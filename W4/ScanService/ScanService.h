#ifndef SCAN_SERVICE_H
#define SCAN_SERVICE_H

#include <windows.h>
#include <string>

// Service configuration
#define SERVICE_NAME L"ScanService"
#define SERVICE_DISPLAY_NAME L"File Scan Service"
#define SERVICE_DESC L"Scans files for safety using ScanEngineDLL"
#define PIPE_NAME L"\\\\.\\pipe\\ScanPipeEndpoint"

// Log configuration
#define LOG_FILE_PATH L"C:\\ProgramData\\ScanService\\scanservice.log"
#define LOG_MAX_SIZE (1024 * 1024) // 1MB

// Service functions
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD ctrl);
void ServiceWorkerThread(void* param);
void UpdateServiceStatus(DWORD currentState);
void WriteErrorLog(const wchar_t* message);

// Global variables
extern SERVICE_STATUS serviceStatus;
extern SERVICE_STATUS_HANDLE serviceStatusHandle;
extern HANDLE stopEvent;
extern bool isRunning;

#endif