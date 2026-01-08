#include "PerformanceMonitor.h"
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

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "advapi32.lib")

SERVICE_STATUS serviceStatus = { 0 };
SERVICE_STATUS_HANDLE serviceStatusHandle = NULL;
HANDLE stopEvent = NULL;
bool isRunning = false;

// PDH query handles for CPU monitoring
static PDH_HQUERY cpuQuery = NULL;
static PDH_HCOUNTER cpuTotal = NULL;
static bool cpuInitialized = false;

// Service Main Entry Point
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    // Register service control handler
    serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    if (!serviceStatusHandle)
    {
        WriteErrorLog(L"RegisterServiceCtrlHandler failed");
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

    // Initialize PDH for CPU monitoring
    PdhOpenQuery(NULL, NULL, &cpuQuery);
    PdhAddEnglishCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
    PdhCollectQueryData(cpuQuery);
    cpuInitialized = true;

    // Create log directory if not exists
    CreateDirectory(L"C:\\ProgramData\\PerformanceMonitor", NULL);

    // Service is running
    UpdateServiceStatus(SERVICE_RUNNING);
    isRunning = true;

    WriteErrorLog(L"Service started successfully");

    // Main service loop
    ServiceWorkerThread(NULL);

    // Cleanup
    if (cpuQuery)
    {
        PdhCloseQuery(cpuQuery);
    }
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
    while (isRunning)
    {
        // Get current time
        SYSTEMTIME st;
        GetLocalTime(&st);

        // Calculate milliseconds until next minute (at second 00)
        DWORD waitUntilNextMinute = (60 - st.wSecond) * 1000 - st.wMilliseconds;

        // If we're very close to the minute boundary (within 100ms), wait for next cycle
        if (waitUntilNextMinute < 100)
        {
            waitUntilNextMinute += 60000;
        }

        // Wait until the first second of next minute
        DWORD waitResult = WaitForSingleObject(stopEvent, waitUntilNextMinute);
        if (waitResult == WAIT_OBJECT_0)
        {
            return;
        }

        // Log performance
        LogSystemPerformance();

        // Rotate log if needed
        RotateLogIfNeeded();
    }
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

// Write error log
void WriteErrorLog(const wchar_t* message)
{
    std::wofstream logFile(LOG_FILE_PATH, std::ios::app);
    if (logFile.is_open())
    {
        logFile << L"[" << GetCurrentTimestamp() << L"] " << message << std::endl;
        logFile.close();
    }
}

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

// Get CPU usage
bool GetCpuUsage(double& cpuUsage)
{
    if (!cpuInitialized || !cpuQuery)
    {
        return false;
    }

    PDH_FMT_COUNTERVALUE counterVal;
    PdhCollectQueryData(cpuQuery);

    Sleep(100);
    PdhCollectQueryData(cpuQuery);

    PDH_STATUS status = PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    if (status == ERROR_SUCCESS)
    {
        cpuUsage = counterVal.doubleValue;
        return true;
    }
    return false;
}

// Get memory usage
bool GetMemoryUsage(double& memUsage, DWORDLONG& totalMem, DWORDLONG& freeMem)
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (GlobalMemoryStatusEx(&memInfo))
    {
        totalMem = memInfo.ullTotalPhys / (1024 * 1024); // Convert to MB
        freeMem = memInfo.ullAvailPhys / (1024 * 1024);
        memUsage = memInfo.dwMemoryLoad;
        return true;
    }
    return false;
}

// Get disk usage (C: drive)
bool GetDiskUsage(double& diskUsage, DWORDLONG& totalDisk, DWORDLONG& freeDisk)
{
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;

    if (GetDiskFreeSpaceEx(L"C:\\", &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
    {
        totalDisk = totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024); // GB
        freeDisk = totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024); // GB
        diskUsage = ((double)(totalNumberOfBytes.QuadPart - totalNumberOfFreeBytes.QuadPart) /
            (double)totalNumberOfBytes.QuadPart) * 100.0;
        return true;
    }
    return false;
}

// Log system performance
void LogSystemPerformance()
{
    double cpuUsage = 0.0, memUsage = 0.0, diskUsage = 0.0;
    DWORDLONG totalMem = 0, freeMem = 0, totalDisk = 0, freeDisk = 0;

    bool cpuOk = GetCpuUsage(cpuUsage);
    bool memOk = GetMemoryUsage(memUsage, totalMem, freeMem);
    bool diskOk = GetDiskUsage(diskUsage, totalDisk, freeDisk);

    std::wofstream logFile(LOG_FILE_PATH, std::ios::app);
    if (logFile.is_open())
    {
        logFile << L"[" << GetCurrentTimestamp() << L"] ";

        if (cpuOk)
        {
            logFile << L"CPU Usage: " << std::fixed << std::setprecision(2) << cpuUsage << L"%. ";
        }
        else
        {
            logFile << L"CPU Usage: N/A. ";
        }

        if (memOk)
        {
            logFile << L"Memory Usage: " << std::fixed << std::setprecision(2)
                << memUsage << L"% ("
                << (totalMem - freeMem) << L" MB / " << totalMem << L" MB). ";
        }
        else
        {
            logFile << L"Memory Usage: N/A. ";
        }

        if (diskOk)
        {
            logFile << L"Disk Usage (C:): " << std::fixed << std::setprecision(2)
                << diskUsage << L"% ("
                << (totalDisk - freeDisk) << L" GB / " << totalDisk << L" GB). ";
        }
        else
        {
            logFile << L"Disk Usage:  N/A. ";
        }
        logFile << std::endl;
        logFile.close();
    }
}

// Get file size
ULONGLONG GetFileSize(const std::wstring& filePath)
{
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesEx(filePath.c_str(), GetFileExInfoStandard, &fileInfo))
    {
        ULARGE_INTEGER size;
        size.HighPart = fileInfo.nFileSizeHigh;
        size.LowPart = fileInfo.nFileSizeLow;
        return size.QuadPart;
    }
    return 0;
}

// Check if log file is old
bool IsLogFileOld()
{
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesEx(LOG_FILE_PATH, GetFileExInfoStandard, &fileInfo)) {
        SYSTEMTIME stUTC, stLocal;
        FileTimeToSystemTime(&fileInfo.ftCreationTime, &stUTC);
        SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

        SYSTEMTIME stNow;
        GetLocalTime(&stNow);

        // Calculate days difference
        FILETIME ftLocal, ftNow;
        SystemTimeToFileTime(&stLocal, &ftLocal);
        SystemTimeToFileTime(&stNow, &ftNow);

        ULARGE_INTEGER ulLocal, ulNow;
        ulLocal.LowPart = ftLocal.dwLowDateTime;
        ulLocal.HighPart = ftLocal.dwHighDateTime;
        ulNow.LowPart = ftNow.dwLowDateTime;
        ulNow.HighPart = ftNow.dwHighDateTime;

        ULONGLONG diffSeconds = (ulNow.QuadPart - ulLocal.QuadPart) / 10000000ULL;
        int daysDiff = (int)(diffSeconds / 86400);

        return daysDiff >= LOG_MAX_AGE_DAYS;
    }
    return false;
}

// Delete old log file
void DeleteOldLogFile()
{
    std::wstring backupPath = L"C:\\ProgramData\\PerformanceMonitor\\performance_old.log";
    // Rename current log to backup
    DeleteFile(backupPath.c_str());
    MoveFile(LOG_FILE_PATH, backupPath.c_str());
    WriteErrorLog(L"Log rotated - old log archived");
}

// Rotate log if needed
void RotateLogIfNeeded()
{
    ULONGLONG fileSize = GetFileSize(LOG_FILE_PATH);

    // Check size (1MB limit)
    if (fileSize >= LOG_MAX_SIZE)
    {
        DeleteOldLogFile();
        WriteErrorLog(L"Log rotated due to size limit (1MB exceeded)");
        return;
    }

    // Check age (5 days limit)
    if (IsLogFileOld())
    {
        DeleteOldLogFile();
        WriteErrorLog(L"Log rotated due to age limit (5 days exceeded)");
        return;
    }
}