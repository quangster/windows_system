#include "PerformanceMonitor.h"
#include <iostream>

void InstallService();
void UninstallService();
void StartMonitorService();
void StopMonitorService();
void DisplayUsage();

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
        StartMonitorService();
    }
    else if (command == L"stop")
    {
        StopMonitorService();
    }
    else if (command == L"run")
    {
        SERVICE_TABLE_ENTRY serviceTable[] = {
            { (PWSTR)SERVICE_NAME, ServiceMain },
            { NULL, NULL }
        };

        if (!StartServiceCtrlDispatcher(serviceTable)) {
            WriteErrorLog(L"StartServiceCtrlDispatcher failed");
        }
    }
    else
    {
        DisplayUsage();
    }

    return 0;
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

void StartMonitorService()
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

void StopMonitorService()
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
    std::wcout << L"  LogService.exe install    - Install the service\n";
    std::wcout << L"  LogService.exe uninstall  - Uninstall the service\n";
    std::wcout << L"  LogService.exe start      - Start the service\n";
    std::wcout << L"  LogService.exe stop       - Stop the service\n";
    std::wcout << L"  LogService.exe run        - Run as service (internal)\n\n";
    std::wcout << L"Features:\n";
    std::wcout << L"  - Monitors CPU, RAM, and Disk usage every 60 seconds\n";
    std::wcout << L"  - Auto-restart on failure\n";
    std::wcout << L"  - Log rotation (deletes logs > 1MB or > 5 days old)\n";
    std::wcout << L"  - Log file:  C:\\ProgramData\\PerformanceMonitor\\performance.log\n";
}