#include <windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

// Hàm lấy thời gian hiện tại dạng chuỗi
std::string GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    localtime_s(&tm_now, &time_t_now);

    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Hàm log thông tin
void Log(const std::string& message) {
    std::cout << "[" << GetCurrentTimeString() << "] " << message << std::endl;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "========================================" << std::endl;
    std::cout << "   PARENT PROCESS - CREATE CHILD DEMO  " << std::endl;
    std::cout << "========================================" << std::endl;

    Log("Parent Process Started");
    Log("Parent PID: " + std::to_string(GetCurrentProcessId()));

    // ===== THIẾT LẬP PIPE ĐỂ REDIRECT STDOUT =====
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;  // Cho phép handle được kế thừa
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;

    // Tạo pipe để redirect stdout của child process
    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
        Log("ERROR: Failed to create pipe.  Error code: " + std::to_string(GetLastError()));
        return 1;
    }

    // Đảm bảo read handle không được kế thừa bởi child process
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    Log("Pipe created successfully for STDOUT redirection");

    // ===== CHUẨN BỊ THAM SỐ CHO CHILD PROCESS =====
    std::wstring childPath = L"ChildProcess.exe";
    std::wstring parameters = L"Hello_From_Parent 12345 TestParam";
    std::wstring commandLine = childPath + L" " + parameters;

    // Cần buffer có thể ghi được cho CreateProcessW
    wchar_t cmdLine[512];
    wcscpy_s(cmdLine, commandLine.c_str());

    Log("Command line: ChildProcess.exe Hello_From_Parent 12345 TestParam");

    // ===== THIẾT LẬP STARTUP INFO =====
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;  // Redirect stdout
    si.hStdError = hWritePipe;   // Redirect stderr
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.wShowWindow = SW_HIDE;    // Ẩn cửa sổ console của child

    ZeroMemory(&pi, sizeof(pi));

    // ===== GHI NHẬN THỜI GIAN BẮT ĐẦU =====
    auto startTime = std::chrono::high_resolution_clock::now();
    Log("Creating child process...");

    // ===== TẠO CHILD PROCESS =====
    BOOL success = CreateProcessW(
        NULL,           // Tên ứng dụng (NULL = lấy từ command line)
        cmdLine,        // Command line với tham số
        NULL,           // Process security attributes
        NULL,           // Thread security attributes
        TRUE,           // Inherit handles (quan trọng cho pipe)
        CREATE_NEW_CONSOLE, // Creation flags
        NULL,           // Environment (NULL = inherit)
        NULL,           // Current directory (NULL = inherit)
        &si,            // Startup info
        &pi             // Process information
    );

    if (!success) {
        DWORD error = GetLastError();
        Log("ERROR: Failed to create process. Error code: " + std::to_string(error));
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return 1;
    }

    Log("Child process created successfully!");
    Log("Child PID: " + std::to_string(pi.dwProcessId));
    Log("Child Thread ID: " + std::to_string(pi.dwThreadId));

    // Đóng write end của pipe trong parent (child sẽ dùng nó)
    CloseHandle(hWritePipe);

    // ===== ĐỌC OUTPUT TỪ CHILD PROCESS =====
    std::cout << "\n--- Child Process Output (Redirected) ---" << std::endl;

    char buffer[4096];
    DWORD bytesRead;
    std::string childOutput;

    // Đọc output từ child process
    while (true) {
        BOOL readSuccess = ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
        if (!readSuccess || bytesRead == 0) {
            break;
        }
        buffer[bytesRead] = '\0';
        childOutput += buffer;
        std::cout << buffer;
    }

    std::cout << "--- End of Child Output ---\n" << std::endl;

    // ===== CHỜ CHILD PROCESS KẾT THÚC =====
    Log("Waiting for child process to complete...");
    WaitForSingleObject(pi.hProcess, INFINITE);

    // ===== GHI NHẬN THỜI GIAN KẾT THÚC =====
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // ===== LẤY EXIT CODE =====
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // ===== LOG KẾT QUẢ =====
    std::cout << "========================================" << std::endl;
    std::cout << "         EXECUTION SUMMARY             " << std::endl;
    std::cout << "========================================" << std::endl;
    Log("Child process completed");
    Log("Exit Code: " + std::to_string(exitCode));
    Log("Execution Time: " + std::to_string(duration.count()) + " ms");

    // ===== CLEANUP =====
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    Log("Parent process finished");

    std::cout << "\nPress Enter to exit... ";
    std::cin.get();

    return 0;
}