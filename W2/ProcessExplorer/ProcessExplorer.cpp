#include <iostream>
#include <windows.h>    // Windows API
#include <tlhelp32.h>   // Tool Help Library
#include <psapi.h>      // get process memory info, path
#include <iomanip>      // setw, setprecision   
#include <limits.h>
#include <string>
#include <algorithm>
#include <cctype>

#undef max

using namespace std;

double BytesToMB(SIZE_T bytes)
{
    return (double)bytes / (1024 * 1024);
}

std::string WCharToString(const WCHAR* wstr)
{
    if (!wstr) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);
    if (!strTo.empty() && strTo.back() == '\0') strTo.pop_back();
    return strTo;
}

std::string ToLowerString(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return str;
}

void GetProcessInfo(DWORD processID, std::string& path, double& ramUsageMB)
{
    path = "N/A (Access Denied)";
    ramUsageMB = 0.0;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

    if (hProcess == NULL)
    {
        return;
    }

    char filename[MAX_PATH];
    if (GetModuleFileNameExA(hProcess, NULL, filename, MAX_PATH))
    {
        path = filename;
    }

    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
    {
        ramUsageMB = BytesToMB(pmc.WorkingSetSize);
    }
    CloseHandle(hProcess);
}

void ListProcesses(std::string filterName = "")
{
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        std::cout << "Khong the tao snapshot. Loi: " << GetLastError() << std::endl;
        return;
    }
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    std::string filterLower = ToLowerString(filterName);

    std::cout << std::left << std::setw(8) << "PID"
        << std::left << std::setw(45) << "Process Name"
        << std::left << std::setw(10) << "RAM (MB)"
        << "Process Path" << std::endl;

    std::cout << std::string(100, '-') << std::endl;

    if (Process32First(hProcessSnap, &pe32))
    {
        do
        {
            std::string processName = WCharToString(pe32.szExeFile);

            if (!filterName.empty())
            {
                std::string processNameLower = ToLowerString(processName);
                if (processNameLower.find(filterLower) == std::string::npos) continue; // if not found, skip
            }

            std::string path;
            double ram;
            GetProcessInfo(pe32.th32ProcessID, path, ram);

            std::cout << std::left << std::setw(8) << pe32.th32ProcessID;
            std::wcout << std::left << std::setw(45) << pe32.szExeFile;
            std::cout << std::left << std::setw(10) << std::fixed << std::setprecision(2) << ram
                << path << std::endl;

        } while (Process32Next(hProcessSnap, &pe32));
    }
    else
    {
        std::cerr << "Khong lay duoc thn tin tien trinh dau tien. " << std::endl;
    }
    CloseHandle(hProcessSnap);
}

void KillProcessByPID(DWORD& pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);

    if (hProcess == NULL)
    {
        cout << "Khong the mo tien trinh voi PID " << pid << ". Loi: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return;
    }

    if (TerminateProcess(hProcess, 0))
    {
        cout << "Tien trinh voi PID " << pid << " da bi ket thuc." << endl;
    }
    else
    {
        cout << "Khong the ket thuc tien trinh voi PID " << pid << ". Loi: " << GetLastError() << std::endl;
    }
    CloseHandle(hProcess);
}

string GetThreadState(HANDLE hThread)
{
    DWORD exitCode;
    if (GetExitCodeThread(hThread, &exitCode))
    {
        if (exitCode != STILL_ACTIVE) return "Terminated";

        DWORD suspendCount = SuspendThread(hThread);
        if (suspendCount == (DWORD)-1)
        {
            return "Running";
        }
        else
        {
            ResumeThread(hThread);
            if (suspendCount > 0) return "Syspended";
            else return "Running";
        }
    }
    return "Unknown";
}

double GetThreadCPUUsage(HANDLE hThread) {
    FILETIME creationTime, exitTime, kernelTime, userTime;

    if (!GetThreadTimes(hThread, &creationTime, &exitTime, &kernelTime, &userTime)) {
        return -1.0;
    }

    // Chuyển đổi FILETIME sang ULONGLONG
    ULONGLONG kernelTimeValue = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    ULONGLONG userTimeValue = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

    // Tổng CPU time (tính bằng 100-nanosecond)
    ULONGLONG totalCPUTime = kernelTimeValue + userTimeValue;

    // Chuyển đổi sang giây (1 giây = 10,000,000 đơn vị 100-nanosecond)
    double cpuTimeSec = totalCPUTime / 10000000.0;

    return cpuTimeSec;
}

void ListThreadsOfProcess(DWORD pid)
{
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
    {
        std::cout << "Khong the tao snapshot luong. Loi: " << GetLastError() << std::endl;
        return;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    std::cout << std::left << std::setw(8) << "TID"
        << std::left << std::setw(15) << "CPU Time Usage"
        << "State" << std::endl;

    std::cout << std::string(100, '-') << std::endl;


    if (!Thread32First(hThreadSnap, &te32))
    {
        std::cerr << "Khong lay duoc thong tin luong dau tien. " << std::endl;
        CloseHandle(hThreadSnap);
        return;
    }

    do
    {
        if (te32.th32OwnerProcessID == pid)
        {
            HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);

            string state = "Unknown";
            double cpuTime = -1.0;

            if (hThread != NULL)
            {
                state = GetThreadState(hThread);
                cpuTime = GetThreadCPUUsage(hThread);
                CloseHandle(hThread);
            }
            else
            {
                state = "Access Denied";
            }

            std::cout << std::left << std::setw(8) << te32.th32ThreadID
                << std::left << std::setw(15) << cpuTime
                << state << std::endl;

        }
    } while (Thread32Next(hThreadSnap, &te32));
    CloseHandle(hThreadSnap);
}

int main()
{
    int choice;
    DWORD pid;

    while (true) {
        cout << "\n=== MINI PROCESS EXPLORER ===" << endl;
        cout << "1. List All Process" << endl;
        cout << "2. Search Process By Name" << endl;
        cout << "3. Kill Process" << endl;
        cout << "4. List Threads of a Process (by PID)" << endl;
        cout << "0. Exit" << endl << endl;
        cout << "Your choice: ";
        cin >> choice;

        if (cin.fail())
        {
            cout << "Lua chon khong hop le! Vui long nhap so." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            system("pause");
            system("cls");
            continue;
        }

        switch (choice) {
            case 1:
            {
                ListProcesses();
                break;
            }
            case 2:
            {
                std::string keyword;
                cout << "Input process name: ";
                cin >> keyword;
                ListProcesses(keyword);
                break;
            }
            case 3:
            {
                cout << "Input Process PID to kill: ";
                cin >> pid;
                if (cin.fail())
                {
                    cout << "PID phai la mot so nguyen!" << endl;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }
                KillProcessByPID(pid);
                break;
            }
            case 4:
            {
                cout << "Input Process PID to list thread: ";
                cin >> pid;
                if (cin.fail())
                {
                    cout << "PID phai la mot so nguyen!" << endl;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }
                ListThreadsOfProcess(pid);
                break;
            }
            case 0:
            {
                return 0;
            }
            default:
            {
                cout << "Your choice not valid!" << endl;
            }
        }
        system("pause");
        system("cls");
    }
    return 0;
}