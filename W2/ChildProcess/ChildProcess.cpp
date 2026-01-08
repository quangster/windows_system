#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

int wmain(int argc, wchar_t* argv[]) {
    // Thiết lập console hỗ trợ Unicode
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "========================================" << std::endl;
    std::cout << "      CHILD PROCESS STARTED            " << std::endl;
    std::cout << "========================================" << std::endl;

    // Hiển thị thông tin process
    std::cout << "Child PID: " << GetCurrentProcessId() << std::endl;
    std::cout << "Parent PID: " << GetCurrentProcessId() << std::endl;

    // ===== NHẬN VÀ XỬ LÝ THAM SỐ TỪ PARENT =====
    std::cout << "\n--- Parameters Received from Parent ---" << std::endl;
    std::cout << "Total arguments: " << argc << std::endl;

    for (int i = 0; i < argc; i++) {
        std::wcout << L"  argv[" << i << L"]: " << argv[i] << std::endl;
    }

    // ===== THỰC HIỆN CÔNG VIỆC MẪU =====
    std::cout << "\n--- Processing Data ---" << std::endl;

    // Giả lập xử lý dữ liệu
    for (int i = 1; i <= 5; i++) {
        std::cout << "Processing step " << i << "/5..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // ===== XỬ LÝ THAM SỐ CỤ THỂ =====
    if (argc > 1) {
        std::wcout << L"\nFirst parameter value: " << argv[1] << std::endl;

        // Chuyển đổi và xử lý nếu là số
        if (argc > 2) {
            try {
                int numValue = std::stoi(argv[2]);
                std::cout << "Numeric parameter:  " << numValue << std::endl;
                std::cout << "Squared value: " << (numValue * numValue) << std::endl;
            }
            catch (...) {
                std::wcout << L"Second parameter (string): " << argv[2] << std::endl;
            }
        }
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "    CHILD PROCESS COMPLETED            " << std::endl;
    std::cout << "========================================" << std::endl;

    return 42;
}