#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <windows.h>
#include <string>

// Service configuration
#define SERVICE_NAME L"PerformanceMonitorService"
#define SERVICE_DISPLAY_NAME L"Performance Monitor Service"
#define SERVICE_DESC L"Monitors system performance (CPU, RAM, Disk) every minute"

// Log configuration
#define LOG_FILE_PATH L"C:\\ProgramData\\PerformanceMonitor\\performance.log"
#define LOG_MAX_SIZE (1024 * 1024) // 1MB
#define LOG_MAX_AGE_DAYS 5

// Service functions
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD ctrl);
void ServiceWorkerThread(void* param);
void UpdateServiceStatus(DWORD currentState);
void WriteErrorLog(const wchar_t* message);

// Performance monitoring functions
void LogSystemPerformance();
void RotateLogIfNeeded();
bool GetCpuUsage(double& cpuUsage);
bool GetMemoryUsage(double& memUsage, DWORDLONG& totalMem, DWORDLONG& freeMem);
bool GetDiskUsage(double& diskUsage, DWORDLONG& totalDisk, DWORDLONG& freeDisk);

// Utility functions
std::wstring GetCurrentTimestamp();
bool IsLogFileOld();
ULONGLONG GetFileSize(const std::wstring& filePath);
void DeleteOldLogFile();

// Global variables
extern SERVICE_STATUS serviceStatus;
extern SERVICE_STATUS_HANDLE serviceStatusHandle;
extern HANDLE stopEvent;
extern bool isRunning;

#endif