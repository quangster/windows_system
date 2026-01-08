#include "ScanService.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <psapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <sys/stat.h>
#include <chrono>
#include <thread>

typedef int(*ScanFileFunc)(const char*);
const int BUFFER_SIZE = 1024;

SERVICE_STATUS serviceStatus = { 0 };
SERVICE_STATUS_HANDLE serviceStatusHandle = NULL;
HANDLE stopEvent = NULL;
bool isRunning = false;

// Get current timestamp
std::wstring GetCurrentTimestamp()
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	std::wstringstream ss;
	ss << std::setfill(L'0')
		<< std::setw(4) << st.wYear << L"-"
		<< std::setw(2) << st.wMonth << L"-"
		<< std::setw(2) << st.wDay << L" "
		<< std::setw(2) << st.wHour << L":"
		<< std::setw(2) << st.wMinute << L":"
		<< std::setw(2) << st.wSecond;

	return ss.str();
}

// Write log
void WriteLog(const wchar_t* message)
{
	std::wofstream logFile(LOG_FILE_PATH, std::ios::app);
	if (logFile.is_open())
	{
		logFile << L"[" << GetCurrentTimestamp() << L"] " << message << std::endl;
		logFile.close();
	}
}

// Check if system is free
bool IsSystemFree()
{
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);

	std::wcout << L"[Monitor] Memory Load: " << memInfo.dwMemoryLoad << L"%\n";

	if (memInfo.dwMemoryLoad > 90)
	{
		return false;
	}
	return true;
}

// Update service status
void UpdateServiceStatus(DWORD currentState)
{
	serviceStatus.dwCurrentState = currentState;
	serviceStatus.dwWin32ExitCode = NO_ERROR;
	serviceStatus.dwWaitHint = 0;

	if (currentState == SERVICE_START_PENDING)
	{
		serviceStatus.dwControlsAccepted = 0;
	}
	else
	{
		serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	}

	if (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED)
	{
		serviceStatus.dwCheckPoint = 0;
	}
	else
	{
		serviceStatus.dwCheckPoint++;
	}
	SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

// Service Main Entry Point
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
	serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
	if (!serviceStatusHandle)
	{
		WriteLog(L"RegisterServiceCtrlHandler failed");
		return;
	}

	// Initialize service status
	ZeroMemory(&serviceStatus, sizeof(serviceStatus));
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

	UpdateServiceStatus(SERVICE_START_PENDING);

	// Create stop event
	stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!stopEvent)
	{
		UpdateServiceStatus(SERVICE_STOPPED);
		return;
	}

	// Create log directory if not exists
	CreateDirectory(L"C:\\ProgramData\\ScanService", NULL);

	// Load Scan Engine DLL
	WriteLog(L"Starting Scan Service...");
	HMODULE hDll = LoadLibrary(L"ScanEngineDLL.dll");
	if (!hDll)
	{
		UpdateServiceStatus(SERVICE_STOPPED);
		WriteLog(L"Error: Cannot load ScanEngineDLL.dll. Make sure it is in the same folder.");
		return;
	}
	ScanFileFunc ScanFile = (ScanFileFunc)GetProcAddress(hDll, "CheckFilePathSafety");
	if (!ScanFile)
	{
		FreeLibrary(hDll);
		UpdateServiceStatus(SERVICE_STOPPED);
		WriteLog(L"Error: Cannot find CheckFilePathSafety in DLL.");
		return;
	}
	
	// Service is running
	UpdateServiceStatus(SERVICE_RUNNING);
	isRunning = true;

	WriteLog(L"Service started successfully");

	// Main service loop
	ServiceWorkerThread((void*)ScanFile);

	// Cleanup
	FreeLibrary(hDll);
	CloseHandle(stopEvent);
	UpdateServiceStatus(SERVICE_STOPPED);
}

// Service Control Handler
void WINAPI ServiceCtrlHandler(DWORD ctrl)
{
	switch (ctrl)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		if (serviceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		UpdateServiceStatus(SERVICE_STOP_PENDING);
		isRunning = false;
		SetEvent(stopEvent);
		break;
	default:
		break;
	}
}

// Worker thread
void ServiceWorkerThread(void* param)
{
	ScanFileFunc ScanFile = (ScanFileFunc)param;
	while (isRunning)
	{
		if (WaitForSingleObject(stopEvent, 0) == WAIT_OBJECT_0)
		{
			WriteLog(L"Stop event received. Exiting worker thread...");
			break;
		}

		WriteLog(L"Waiting for client connection...");
		HANDLE hPipe = CreateNamedPipe(
			PIPE_NAME,
			PIPE_ACCESS_DUPLEX,                                      // Read/Write access
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,   // Message mode, blocking
			1,                                                       // Max instances
			BUFFER_SIZE,                                             // Output buffer size
			BUFFER_SIZE,                                             // Input buffer size
			0,                                                       // Timeout
			NULL                                                     // Security attributes
		);

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			WriteLog(L"CreateNamedPipe failed");
			return;
		}

		OVERLAPPED overlapped = { 0 };
		overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		BOOL connected = ConnectNamedPipe(hPipe, &overlapped);
		DWORD lastError = GetLastError();

		if (!connected && lastError == ERROR_IO_PENDING)
		{
			// Chờ với timeout hoặc stop event
			HANDLE handles[2] = { stopEvent, overlapped.hEvent };
			DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, 500); // 1 giây timeout

			if (waitResult == WAIT_OBJECT_0) // Stop event được signal
			{
				WriteLog(L"Stop event received. Canceling pipe operation...");
				CancelIo(hPipe);
				CloseHandle(overlapped.hEvent);
				DisconnectNamedPipe(hPipe);
				CloseHandle(hPipe);
				break;
			}
			else if (waitResult == WAIT_OBJECT_0 + 1) // Client connected
			{
				connected = TRUE;
			}
			else if (waitResult == WAIT_TIMEOUT)
			{
				// Timeout - tiếp tục vòng lặp
				CancelIo(hPipe);
				CloseHandle(overlapped.hEvent);
				DisconnectNamedPipe(hPipe);
				CloseHandle(hPipe);
				continue;
			}
		}
		else if (lastError == ERROR_PIPE_CONNECTED)
		{
			connected = TRUE;
		}

		CloseHandle(overlapped.hEvent);

		if (connected)
		{
			std::wcout << L"Client connected." << std::endl;
			wchar_t buffer[BUFFER_SIZE];
			DWORD bytesRead;
			
			if (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL))
			{
				std::wstring filePath(buffer);
				WriteLog((std::wstring(L"Request scan for: ") + filePath).c_str());
				std::wstring response;
			
				if (IsSystemFree())
				{
					WriteLog(L"System is free. Scanning...");
					char ansiPath[MAX_PATH];
					WideCharToMultiByte(CP_ACP, 0, filePath.c_str(), -1, ansiPath, MAX_PATH, NULL, NULL);
					int result = ScanFile(ansiPath);
					if (result == 0)
					{
						response = L"SAFE: File is safe.";
					}
					else if (result == 1)
					{
						response = L"WARNING: File is on a network drive.";
					}
					else if (result == 2)
					{
						response = L"DANGER: File is on a removable drive.";
					}
					else
					{
						response = L"ERROR: Could not determine file safety.";
					}
					WriteLog((std::wstring(L"Scan result : ") + response).c_str());
				}
				else
				{
					response = L"BUSY: System resources are high. Try again later.";
					WriteLog(L"System is busy. Skipping scan.");
				}
			
				DWORD bytesWritten;
				WriteFile(hPipe, response.c_str(), (response.size() + 1) * sizeof(wchar_t), &bytesWritten, NULL);
			}
		}
			
		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);
	}
	WriteLog(L"Service worker thread stopped.");
}



void InstallService()
{
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!scManager)
	{
		std::wcout << L"Failed to open Service Control Manager.  Error: "
			<< GetLastError() << std::endl;
		return;
	}

	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);

	std::wstring fullPath = std::wstring(L"\"") + path + L"\" run";
	std::wcout << L"Service binary path: " << fullPath << std::endl;

	SC_HANDLE service = CreateService(
		scManager,                      // [in] SC_HANDLE hSCManager: handle to SCM database
		SERVICE_NAME,                   // [in] PCSTR lpServiceName: the name of the service to install
		SERVICE_DISPLAY_NAME,           // [in, optional] PCSTR lpDisplayName: the display name to show in UIs
		SERVICE_ALL_ACCESS,             // [in] DWORD dwDesiredAccess: access to the service
		SERVICE_WIN32_OWN_PROCESS,      // [in] DWORD dwServiceType: the type of service
		SERVICE_AUTO_START,             // [in] DWORD dwStartType: when to start the service 
		SERVICE_ERROR_NORMAL,           // [in] DWORD dwErrorControl: severity of the error if service fails to start
		fullPath.c_str(),               // [in, optional] PCSTR lpBinaryPathName: path to service binary
		NULL, NULL, NULL, NULL, NULL
	);

	if (service)
	{
		// Set service description
		SERVICE_DESCRIPTION sd;
		sd.lpDescription = (LPWSTR)SERVICE_DESC;
		ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &sd);

		// Configure auto-restart on failure
		SERVICE_FAILURE_ACTIONS sfa;
		SC_ACTION actions[3];

		actions[0].Type = SC_ACTION_RESTART;
		actions[0].Delay = 5000; // 5 seconds
		actions[1].Type = SC_ACTION_RESTART;
		actions[1].Delay = 10000; // 10 seconds
		actions[2].Type = SC_ACTION_RESTART;
		actions[2].Delay = 30000; // 30 seconds

		sfa.dwResetPeriod = 86400; // 24 hours
		sfa.lpRebootMsg = NULL;
		sfa.lpCommand = NULL;
		sfa.cActions = 3;
		sfa.lpsaActions = actions;

		ChangeServiceConfig2(service, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);

		std::wcout << L"Service installed successfully!" << std::endl;
		std::wcout << L"Auto-restart enabled on failure." << std::endl;
		std::wcout << L"Use 'ServiceInstaller.exe start' to start the service." << std::endl;

		CloseServiceHandle(service);
	}
	else
	{
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_EXISTS)
		{
			std::wcout << L"Service already exists!" << std::endl;
		}
		else
		{
			std::wcout << L"Failed to create service. Error: " << error << std::endl;
		}
	}
	CloseServiceHandle(scManager);
}

void UninstallService()
{
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!scManager)
	{
		std::wcout << L"Failed to open Service Control Manager.  Error: "
			<< GetLastError() << std::endl;
		return;
	}

	SC_HANDLE service = OpenService(scManager, SERVICE_NAME, SERVICE_STOP | DELETE);
	if (service)
	{
		// Stop service first
		SERVICE_STATUS status;
		ControlService(service, SERVICE_CONTROL_STOP, &status);
		Sleep(1000);

		// Delete service
		if (DeleteService(service))
		{
			std::wcout << L"Service uninstalled successfully!" << std::endl;
		}
		else
		{
			std::wcout << L"Failed to uninstall service. Error: "
				<< GetLastError() << std::endl;
		}

		CloseServiceHandle(service);
	}
	else
	{
		std::wcout << L"Failed to open service. Error: " << GetLastError() << std::endl;
	}

	CloseServiceHandle(scManager);
}

void StartScanService()
{
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!scManager)
	{
		std::wcout << L"Failed to open Service Control Manager. Error: "
			<< GetLastError() << std::endl;
		return;
	}

	SC_HANDLE service = OpenService(scManager, SERVICE_NAME, SERVICE_START);
	if (service)
	{
		if (StartService(service, 0, NULL))
		{
			std::wcout << L"Service started successfully!" << std::endl;
		}
		else
		{
			DWORD error = GetLastError();
			if (error == ERROR_SERVICE_ALREADY_RUNNING)
			{
				std::wcout << L"Service is already running!" << std::endl;
			}
			else
			{
				std::wcout << L"Failed to start service. Error: " << error << std::endl;
			}
		}
		CloseServiceHandle(service);
	}
	else
	{
		std::wcout << L"Failed to open service. Error: " << GetLastError() << std::endl;
	}
	CloseServiceHandle(scManager);
}

void StopScanService()
{
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!scManager)
	{
		std::wcout << L"Failed to open Service Control Manager. Error: "
			<< GetLastError() << std::endl;
		return;
	}

	SC_HANDLE service = OpenService(scManager, SERVICE_NAME, SERVICE_STOP);
	if (service)
	{
		SERVICE_STATUS status;
		if (ControlService(service, SERVICE_CONTROL_STOP, &status))
		{
			std::wcout << L"Service stopped successfully!" << std::endl;
		}
		else
		{
			std::wcout << L"Failed to stop service. Error: "
				<< GetLastError() << std::endl;
		}
		CloseServiceHandle(service);
	}
	else
	{
		std::wcout << L"Failed to open service. Error: " << GetLastError() << std::endl;
	}
	CloseServiceHandle(scManager);
}

void DisplayUsage()
{
	std::wcout << L"Performance Monitor Service Manager\n";
	std::wcout << L"===================================\n\n";
	std::wcout << L"Usage:\n";
	std::wcout << L"  ScanService.exe install    - Install the service\n";
	std::wcout << L"  ScanService.exe uninstall  - Uninstall the service\n";
	std::wcout << L"  ScanService.exe start      - Start the service\n";
	std::wcout << L"  ScanService.exe stop       - Stop the service\n";
	std::wcout << L"  ScanService.exe run        - Run as service (internal)\n\n";
}

int wmain(int argc, wchar_t* argv[])
{
	if (argc < 2)
	{
		DisplayUsage();
		return 0;
	}

	std::wstring command = argv[1];

	if (command == L"install")
	{
		InstallService();
	}
	else if (command == L"uninstall")
	{
		UninstallService();
	}
	else if (command == L"start")
	{
		StartScanService();
	}
	else if (command == L"stop")
	{
		StopScanService();
	}
	else if (command == L"run")
	{
		SERVICE_TABLE_ENTRY serviceTable[] = {
			{ (PWSTR)SERVICE_NAME, ServiceMain },
			{ NULL, NULL }
		};

		if (!StartServiceCtrlDispatcher(serviceTable)) {
			WriteLog(L"StartServiceCtrlDispatcher failed");
		}
	}
	else
	{
		DisplayUsage();
	}

	return 0;
}