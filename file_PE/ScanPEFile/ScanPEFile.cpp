// ScanPEFile.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ScanPEFile.h"
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100
#define MAX_DEPTH 10
#define MAX_PATH_LONG 32767
#define WM_SCAN_COMPLETE (WM_USER + 1)
#define WM_ADD_PE_FILE (WM_USER + 2)
#define WM_UPDATE_STATUS (WM_USER + 3)
#define IDT_STATUS_TIMER 1001
#define STATUS_UPDATE_INTERVAL 200  // Update every 200ms

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// UI Controls
HWND hEditPath = NULL;
HWND hBtnBrowse = NULL;
HWND hListView = NULL;
HWND hBtnScan = NULL;
HWND hBtnStop = NULL;
HWND hBtnClear = NULL;
HWND hStaticStatus = NULL;
HWND g_hWndMain = NULL;

// Threading
HANDLE hScanThread = NULL;
HANDLE hStopEvent = NULL;
HANDLE hMutex = NULL;
volatile BOOL g_bScanning = FALSE;
CRITICAL_SECTION g_cs;

// Statistics
volatile LONG g_nFilesScanned = 0;
volatile LONG g_nPEFilesFound = 0;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// PE File checking function
BOOL IsPEFile(LPCWSTR szFilePath)
{
    HANDLE hFile = CreateFileW(szFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    BOOL bIsPE = FALSE;
    DWORD dwRead = 0;

    // Read DOS header
    IMAGE_DOS_HEADER dosHeader;
    if (ReadFile(hFile, &dosHeader, sizeof(dosHeader), &dwRead, NULL) &&
        dwRead == sizeof(dosHeader))
    {
        // Check MZ signature
        if (dosHeader.e_magic == IMAGE_DOS_SIGNATURE)
        {
            // Validate e_lfanew is reasonable
            if (dosHeader.e_lfanew > 0 && dosHeader.e_lfanew < 0x10000000)
            {
                // Seek to PE header
                if (SetFilePointer(hFile, dosHeader.e_lfanew, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
                {
                    DWORD peSignature = 0;
                    if (ReadFile(hFile, &peSignature, sizeof(peSignature), &dwRead, NULL) &&
                        dwRead == sizeof(peSignature))
                    {
                        // Check PE signature
                        if (peSignature == IMAGE_NT_SIGNATURE)
                        {
                            bIsPE = TRUE;
                        }
                    }
                }
            }
        }
    }

    CloseHandle(hFile);
    return bIsPE;
}

// Add PE file to ListView (thread-safe)
void AddPEFileToListView(LPCWSTR szFilePath)
{
    // Send message to main thread to add item
    WCHAR* pFilePath = _wcsdup(szFilePath);
    if (pFilePath)
    {
        // Use SendMessage with timeout to prevent queue overflow
        if (!PostMessage(g_hWndMain, WM_ADD_PE_FILE, 0, (LPARAM)pFilePath))
        {
            free(pFilePath);  // Free if post failed
        }
    }
}

// Recursive directory scanning function using std::wstring for long paths
void ScanDirectory(const std::wstring& szPath, int nDepth)
{
    if (nDepth > MAX_DEPTH)
        return;

    // Check if stop requested
    if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
        return;

    // Create search pattern
    std::wstring searchPath = szPath;
    if (!searchPath.empty() && searchPath.back() != L'\\')
        searchPath += L"\\";
    searchPath += L"*";

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileExW(searchPath.c_str(), FindExInfoBasic, &findData, 
        FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        // Check if stop requested
        if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
            break;

        // Skip . and ..
        if (findData.cFileName[0] == L'.')
        {
            if (findData.cFileName[1] == L'\0' || 
                (findData.cFileName[1] == L'.' && findData.cFileName[2] == L'\0'))
                continue;
        }

        // Build full path using std::wstring
        std::wstring fullPath = szPath;
        if (!fullPath.empty() && fullPath.back() != L'\\')
            fullPath += L"\\";
        fullPath += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Skip system/hidden directories for performance
            if (!(findData.dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_REPARSE_POINT)))
            {
                // Recursively scan subdirectory
                ScanDirectory(fullPath, nDepth + 1);
            }
        }
        else
        {
            // It's a file - check if PE
            InterlockedIncrement(&g_nFilesScanned);
            
            // Skip very small files (PE files are at least 64 bytes for DOS header)
            ULONGLONG fileSize = ((ULONGLONG)findData.nFileSizeHigh << 32) | findData.nFileSizeLow;
            if (fileSize >= 64)
            {
                if (IsPEFile(fullPath.c_str()))
                {
                    InterlockedIncrement(&g_nPEFilesFound);
                    AddPEFileToListView(fullPath.c_str());
                }
            }
        }

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

// Scan thread function
DWORD WINAPI ScanThreadProc(LPVOID lpParam)
{
    WCHAR* szPath = (WCHAR*)lpParam;

    g_nFilesScanned = 0;
    g_nPEFilesFound = 0;

    std::wstring path(szPath);
    ScanDirectory(path, 0);

    free(szPath);

    // Notify main thread that scan is complete
    PostMessage(g_hWndMain, WM_SCAN_COMPLETE, 0, 0);

    return 0;
}

// Start scanning
void StartScan(HWND hWnd)
{
    if (g_bScanning)
        return;

    WCHAR szPath[MAX_PATH];
    GetWindowTextW(hEditPath, szPath, MAX_PATH);

    if (wcslen(szPath) == 0)
    {
        MessageBoxW(hWnd, L"Please enter a folder path!", L"Error", MB_ICONERROR);
        return;
    }

    // Check if path exists
    DWORD dwAttr = GetFileAttributesW(szPath);
    if (dwAttr == INVALID_FILE_ATTRIBUTES || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        MessageBoxW(hWnd, L"Invalid path or folder does not exist!", L"Error", MB_ICONERROR);
        return;
    }

    // Clear previous results
    ListView_DeleteAllItems(hListView);

    // Reset stop event
    ResetEvent(hStopEvent);

    // Update UI - disable all controls except Stop
    EnableWindow(hBtnScan, FALSE);
    EnableWindow(hBtnStop, TRUE);
    EnableWindow(hBtnClear, FALSE);
    EnableWindow(hEditPath, FALSE);
    EnableWindow(hBtnBrowse, FALSE);

    SetWindowTextW(hStaticStatus, L"Scanning...");

    g_bScanning = TRUE;

    // Start timer for status updates
    SetTimer(hWnd, IDT_STATUS_TIMER, STATUS_UPDATE_INTERVAL, NULL);

    // Start scan thread
    WCHAR* pPath = _wcsdup(szPath);
    hScanThread = CreateThread(NULL, 0, ScanThreadProc, pPath, 0, NULL);
}

// Stop scanning
void StopScan()
{
    if (!g_bScanning)
        return;

    SetEvent(hStopEvent);
    SetWindowTextW(hStaticStatus, L"Stopping...");
}

// Clear results
void ClearResults()
{
    ListView_DeleteAllItems(hListView);
    g_nFilesScanned = 0;
    g_nPEFilesFound = 0;
    SetWindowTextW(hStaticStatus, L"Ready");
}

// Browse folder dialog
void BrowseFolder(HWND hWnd)
{
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hWnd;
    bi.lpszTitle = L"Select folder to scan:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != NULL)
    {
        WCHAR szPath[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, szPath))
        {
            SetWindowTextW(hEditPath, szPath);
        }
        CoTaskMemFree(pidl);
    }
}

// Initialize ListView
void InitListView(HWND hWnd)
{
    LVCOLUMNW lvc = { 0 };
    
    // Column 1: File path
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"PE File Path";
    lvc.cx = 700;
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(hListView, 0, &lvc);

    // Column 2: File name
    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"File Name";
    lvc.cx = 200;
    ListView_InsertColumn(hListView, 1, &lvc);

    // Set extended styles
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
}

// Create UI controls
void CreateControls(HWND hWnd)
{
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Static label for path
    CreateWindowW(L"STATIC", L"Folder Path:",
        WS_CHILD | WS_VISIBLE,
        10, 15, 80, 20, hWnd, (HMENU)IDC_STATIC_PATH, hInst, NULL);

    // Edit control for path
    hEditPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        100, 10, 450, 25, hWnd, (HMENU)IDC_EDIT_PATH, hInst, NULL);

    // Browse button
    hBtnBrowse = CreateWindowW(L"BUTTON", L"Browse...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        560, 10, 80, 25, hWnd, (HMENU)IDC_BTN_BROWSE, hInst, NULL);

    // Scan button
    hBtnScan = CreateWindowW(L"BUTTON", L"Scan",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        650, 10, 70, 25, hWnd, (HMENU)IDC_BTN_SCAN, hInst, NULL);

    // Stop button
    hBtnStop = CreateWindowW(L"BUTTON", L"Stop",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        730, 10, 70, 25, hWnd, (HMENU)IDC_BTN_STOP, hInst, NULL);

    // Clear button
    hBtnClear = CreateWindowW(L"BUTTON", L"Clear",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        810, 10, 70, 25, hWnd, (HMENU)IDC_BTN_CLEAR, hInst, NULL);

    // Static label for result
    CreateWindowW(L"STATIC", L"Results (PE files found):",
        WS_CHILD | WS_VISIBLE,
        10, 45, 200, 20, hWnd, (HMENU)IDC_STATIC_RESULT, hInst, NULL);

    // ListView for results
    hListView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_VSCROLL | WS_HSCROLL,
        10, 70, 870, 400, hWnd, (HMENU)IDC_LISTVIEW_RESULT, hInst, NULL);

    InitListView(hWnd);

    // Status label - positioned below ListView with enough space
    hStaticStatus = CreateWindowW(L"STATIC", L"Ready",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 480, 870, 25, hWnd, (HMENU)IDC_STATIC_STATUS, hInst, NULL);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Create mutex to ensure single instance
    hMutex = CreateMutexW(NULL, TRUE, L"ScanPEFile_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxW(NULL, L"Application is already running!\nCannot run another instance.",
            L"Error", MB_ICONERROR | MB_OK);
        CloseHandle(hMutex);
        return 0;
    }

    // Initialize COM for folder browser
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Create stop event
    hStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    // Initialize critical section
    InitializeCriticalSection(&g_cs);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SCANPEFILE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SCANPEFILE));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Cleanup
    if (hStopEvent)
    {
        SetEvent(hStopEvent);
        if (hScanThread)
        {
            WaitForSingleObject(hScanThread, 5000);
            CloseHandle(hScanThread);
        }
        CloseHandle(hStopEvent);
    }

    DeleteCriticalSection(&g_cs);
    CloseHandle(hMutex);
    CoUninitialize();

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SCANPEFILE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SCANPEFILE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   g_hWndMain = CreateWindowW(szWindowClass, L"PE File Scanner",
       WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
       CW_USEDEFAULT, 0, 910, 560, nullptr, nullptr, hInstance, nullptr);

   if (!g_hWndMain)
   {
      return FALSE;
   }

   CreateControls(g_hWndMain);

   ShowWindow(g_hWndMain, nCmdShow);
   UpdateWindow(g_hWndMain);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDC_BTN_BROWSE:
                BrowseFolder(hWnd);
                break;
            case IDC_BTN_SCAN:
                StartScan(hWnd);
                break;
            case IDC_BTN_STOP:
                StopScan();
                break;
            case IDC_BTN_CLEAR:
                ClearResults();
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_TIMER:
        if (wParam == IDT_STATUS_TIMER && g_bScanning)
        {
            // Update status periodically
            WCHAR szStatus[256];
            swprintf_s(szStatus, 256, L"Scanning... Scanned: %ld files, Found: %ld PE files",
                g_nFilesScanned, g_nPEFilesFound);
            SetWindowTextW(hStaticStatus, szStatus);
        }
        break;

    case WM_ADD_PE_FILE:
        {
            // Add PE file to ListView (called from main thread)
            WCHAR* pFilePath = (WCHAR*)lParam;
            if (pFilePath)
            {
                // Disable redraw for performance
                SendMessage(hListView, WM_SETREDRAW, FALSE, 0);

                LVITEMW lvi = { 0 };
                lvi.mask = LVIF_TEXT;
                lvi.iItem = ListView_GetItemCount(hListView);
                lvi.iSubItem = 0;
                lvi.pszText = pFilePath;
                int index = ListView_InsertItem(hListView, &lvi);

                // Extract filename from path
                WCHAR* pFileName = wcsrchr(pFilePath, L'\\');
                if (pFileName)
                    pFileName++;
                else
                    pFileName = pFilePath;

                ListView_SetItemText(hListView, index, 1, pFileName);

                // Re-enable redraw
                SendMessage(hListView, WM_SETREDRAW, TRUE, 0);

                free(pFilePath);
            }
        }
        break;

    case WM_SCAN_COMPLETE:
        {
            // Stop the timer
            KillTimer(hWnd, IDT_STATUS_TIMER);

            // Scan completed
            g_bScanning = FALSE;

            EnableWindow(hBtnScan, TRUE);
            EnableWindow(hBtnStop, FALSE);
            EnableWindow(hBtnClear, TRUE);
            EnableWindow(hEditPath, TRUE);
            EnableWindow(hBtnBrowse, TRUE);

            if (hScanThread)
            {
                CloseHandle(hScanThread);
                hScanThread = NULL;
            }

            // Force ListView to redraw
            InvalidateRect(hListView, NULL, TRUE);

            WCHAR szStatus[256];
            swprintf_s(szStatus, 256, L"Completed! Scanned: %ld files, Found: %ld PE files",
                g_nFilesScanned, g_nPEFilesFound);
            SetWindowTextW(hStaticStatus, szStatus);
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_CLOSE:
        if (g_bScanning)
        {
            int result = MessageBoxW(hWnd, 
                L"Scanning is in progress. Are you sure you want to exit?",
                L"Confirm", MB_YESNO | MB_ICONQUESTION);
            if (result == IDNO)
                return 0;
            StopScan();
            // Wait a bit for thread to stop
            if (hScanThread)
            {
                WaitForSingleObject(hScanThread, 2000);
            }
        }
        KillTimer(hWnd, IDT_STATUS_TIMER);
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
