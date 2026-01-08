#include <windows.h>

#include <iostream>
#include <iomanip>

#undef max

std::wstring GetServiceStateString(DWORD state)
{
	switch (state)
	{
		case SERVICE_STOPPED:  return L"STOPPED";
		case SERVICE_START_PENDING:  return L"START_PENDING";
		case SERVICE_STOP_PENDING: return L"STOP_PENDING";
		case SERVICE_RUNNING:   return L"RUNNING";
		case SERVICE_CONTINUE_PENDING: return L"CONTINUE_PENDING";
		case SERVICE_PAUSE_PENDING:   return L"PAUSE_PENDING";
		case SERVICE_PAUSED:  return L"PAUSED";
		default: return L"UNKNOWN";
	}
}

void ListServices()
{
	SC_HANDLE scManager = OpenSCManagerW(
		NULL,                          // [in, optional] LPCWSTR lpMachineName: name of the target computer
		NULL,                          // [in, optional] LPCWSTR lpDatabaseName: name of the service control manager database
		SC_MANAGER_ENUMERATE_SERVICE   // [in] dwDesiredAccess: access to the service control manager
	);

	if (scManager == NULL)
	{
		std::cerr << "Loi: khong the mo Service Control Manager. Ma loi: " << GetLastError() << std::endl;
		return;
	}
	DWORD bytesNeeded = 0;
	DWORD servicesReturned = 0;
	DWORD resumeHandle = 0;

	EnumServicesStatusExW(
		scManager,            // [in] SC_HANDLE hSCManager:  handle to the service control manager database
		SC_ENUM_PROCESS_INFO, // [in] SC_ENUM_TYPE InfoLevel:  the type of service information to be returned
		SERVICE_WIN32,        // [in] DWORD dwServiceType:  the type of services to be enumerated
		SERVICE_STATE_ALL,    // [in] DWORD dwServiceState:  the state of the services to be enumerated
		NULL,                 // [out, optional] LPBYTE lpServices:  a pointer to the buffer that receives the service status information
		0,                    // [in] DWORD cbBufSize:  the size of the buffer pointed to by the lpServices parameter
		&bytesNeeded,         // [out] LPDWORD pcbBytesNeeded:  a pointer to a variable that receives the number of bytes needed to store all the service status information
		&servicesReturned,    // [out] LPDWORD lpServicesReturned:  a pointer to a variable that receives the number of services returned
		&resumeHandle,        // [in, out, optional] LPDWORD lpResumeHandle:  a pointer to a variable that contains a value used to continue an existing service enumeration
		NULL                  // [in, optional] LPCWSTR pszGroupName:  the name of the service group to be enumerated
	);

	BYTE* buffer = new BYTE[bytesNeeded];
	if (buffer == NULL)
	{
		CloseServiceHandle(scManager);
		std::cerr << "Loi: khong the cap phat bo nho." << std::endl;
		return;
	}

	if (!EnumServicesStatusExW(
		scManager,
		SC_ENUM_PROCESS_INFO,
		SERVICE_WIN32,
		SERVICE_STATE_ALL,
		buffer,
		bytesNeeded,
		&bytesNeeded,
		&servicesReturned,
		&resumeHandle,
		NULL
	))
	{
		delete[] buffer;
		CloseServiceHandle(scManager);
		std::cerr << "Loi: khong the liet ke services. Ma loi: " << GetLastError() << std::endl;
		return;
	}

	ENUM_SERVICE_STATUS_PROCESS* services = (ENUM_SERVICE_STATUS_PROCESS*)buffer;

	std::cout << std::left << std::setw(5) << "ID"
		<< std::left << std::setw(50) << "Service Name"
		<< std::left << std::setw(85) << "Display Name"
		<< std::left << std::setw(10) << "State"
		<< std::endl;

	for (DWORD i = 0; i < servicesReturned; i++)
	{
		std::wcout << std::left << std::setw(5) << i
			<< std::left << std::setw(50) << services[i].lpServiceName
			<< std::left << std::setw(85) << services[i].lpDisplayName
			<< std::left << std::setw(10) << GetServiceStateString(services[i].ServiceStatusProcess.dwCurrentState)
			<< std::endl;
	}
}

bool StartServiceByName(const std::wstring& serviceName)
{
	SC_HANDLE scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
	
	if (scManager == NULL)
	{
		std::wcerr << L"Loi: khong the mo Service Control Manager. Ma loi: " << GetLastError() << std::endl;
		return false;
	}

	SC_HANDLE service = OpenServiceW(scManager, serviceName.c_str(), SERVICE_START | SERVICE_QUERY_STATUS);

	if (service == NULL)
	{
		std::wcerr << L"Loi: Khong the mo Service " << serviceName << L". Ma loi: " << GetLastError() << std::endl;
		CloseServiceHandle(scManager);
		return false;
	}

		
	SERVICE_STATUS_PROCESS status;
	DWORD bytesNeeded;
	if (
		!QueryServiceStatusEx(
			service, 
			SC_STATUS_PROCESS_INFO, 
			(LPBYTE)&status,
			sizeof(SERVICE_STATUS_PROCESS),
			&bytesNeeded
		)
	)
	{
		std::wcerr << L"Loi: Khong the lay trang thai cua Service " << serviceName << L". Ma loi: " << GetLastError() << std::endl;
		CloseServiceHandle(service);
		CloseServiceHandle(scManager);
		return false;
	}

	if (status.dwCurrentState == SERVICE_RUNNING)
	{
		std::wcout << L"Service " << serviceName << L" da dang chay." << std::endl;
		CloseServiceHandle(service);
		CloseServiceHandle(scManager);
		return true;
	}

	if (!StartService(service, 0, NULL))
	{
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_ALREADY_RUNNING)
		{
			std::wcout << L"Service " << serviceName << L" da dang chay." << std::endl;
		}
		else
		{
			std::wcerr << L"Loi: Khong the khoi dong Service " << serviceName << L". Ma loi: " << error << std::endl;
			CloseServiceHandle(service);
			CloseServiceHandle(scManager);
			return false;
		}
	}

	// Wait until the service is running
	std::wcout << L"Dang khoi dong Service " << serviceName << L"..." << std::endl;
	for (int i = 0; i < 30; i++)
	{
		Sleep(1000);
		if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(status), &bytesNeeded))
		{
			break;
		}
		if (status.dwCurrentState == SERVICE_RUNNING) {
			std::wcout << L"[OK] Service '" << serviceName << L"' da duoc start thanh cong!" << std::endl;
			CloseServiceHandle(service);
			CloseServiceHandle(scManager);
			return true;
		}
	}
	CloseServiceHandle(service);
	CloseServiceHandle(scManager);
	return true;
}

bool StopServiceByName(const std::wstring& serviceName)
{
	SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (scManager == NULL)
	{
		std::wcerr << L"Loi: Khong the mo Service Control Manager. Ma loi: " << GetLastError() << std::endl;
		return false;
	}

	SC_HANDLE service = OpenService(scManager, serviceName.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
	if (service == NULL)
	{
		std::wcerr << L"Loi: Khong the mo Service " << serviceName << L". Ma loi: " << GetLastError() << std::endl;
		CloseServiceHandle(scManager);
		return false;
	}

	SERVICE_STATUS_PROCESS status;
	DWORD bytesNeeded;
	if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded))
	{
		std::wcerr << L"Loi: Khong the lay trang thai cua Service " << serviceName << L". Ma loi: " << GetLastError() << std::endl;
		CloseServiceHandle(service);
		CloseServiceHandle(scManager);
		return false;
	}

	if (status.dwCurrentState == SERVICE_STOPPED)
	{
		std::wcout << L"Service " << serviceName << L" da bi dung." << std::endl;
		CloseServiceHandle(service);
		CloseServiceHandle(scManager);
		return true;
	}

	SERVICE_STATUS stopStatus;
	if (!ControlService(service, SERVICE_CONTROL_STOP, &stopStatus))
	{
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_NOT_ACTIVE)
		{
			std::wcout << L"Service " << serviceName << L" da bi dung." << std::endl;
		}
		else
		{
			std::wcerr << L"Loi: Khong the dung Service " << serviceName << L". Ma loi: " << error << std::endl;
			CloseServiceHandle(service);
			CloseServiceHandle(scManager);
			return false;
		}
	}

	// Wait until the service is stopped
	std::wcout << L"Dang dung Service " << serviceName << L"..." << std::endl;
	for (int i = 0; i < 30; i++)
	{
		Sleep(1000);
		if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(status), &bytesNeeded))
		{
			break;
		}
		if (status.dwCurrentState == SERVICE_STOPPED) {
			std::wcout << L"[OK] Service '" << serviceName << L"' da duoc dung thanh cong!" << std::endl;
			CloseServiceHandle(service);
			CloseServiceHandle(scManager);
			return true;
		}
	}

	CloseServiceHandle(service);
	CloseServiceHandle(scManager);
	return true;
}

int main()
{
	int choice;
	while (true)
	{
		std::cout << "\n=== MINI SERVICE CONTROLLER ===" << std::endl;
		std::cout << "1. List All Services" << std::endl;
		std::cout << "2. Start Service by Name" << std::endl;
		std::cout << "3. Stop Service by Name" << std::endl;
		std::cout << "0. Exit" << std::endl << std::endl;
		std::cout << "Your choice: ";
		std::cin >> choice;

		if (std::cin.fail())
		{
			std::cout << "Lua chon khong hop le! Vui long nhap so." << std::endl;
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			system("pause");
			system("cls");
			continue;
		}

		switch (choice)
		{
			case 1:
			{
				ListServices();
				break;
			}
			case 2:
			{
				std::string serviceName;
				std::cout << "Input Service Name to start: ";
				std::cin >> serviceName;
				StartServiceByName(std::wstring(serviceName.begin(), serviceName.end()));
				break;
			}
			case 3:
			{
				std::string serviceName;
				std::cout << "Input Service Name to stop: ";
				std::cin >> serviceName;
				StopServiceByName(std::wstring(serviceName.begin(), serviceName.end()));
				break;
			}
			case 0:
			{
				return 0;
			}
			default:
			{
				std::cout << "Your choice is invalid! Please try again." << std::endl;
			}
		}
		system("pause");
		system("cls");
	}
	return 0;
}