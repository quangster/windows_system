#include <windows.h>
#include <iostream>
#include <string>

const LPCWSTR PIPE_NAME = L"\\\\.\\pipe\\ScanPipeEndpoint";
const int BUFFER_SIZE = 1024;

int main(int argc, char* argv[])
{
	std::wstring filePath;
	if (argc > 1)
	{
		std::string temp(argv[1]);
		filePath = std::wstring(temp.begin(), temp.end());
	}
	else
	{
		std::wcout << L"Enter file path to scan: ";
		std::getline(std::wcin, filePath);
	}

	HANDLE hPipe;
	while (true)
	{
		hPipe = CreateFile(
			PIPE_NAME,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);
		if (hPipe != INVALID_HANDLE_VALUE) break;

		if (GetLastError() == ERROR_PIPE_BUSY)
		{
			if (!WaitNamedPipe(PIPE_NAME, 20000))
			{
				std::wcout << L"Could not open pipe: 20 second wait timed out." << std::endl;
				return -1;
			}
		}
		else
		{
			std::wcout << L"Could not connect to Service. Make sure Service is running" << std::endl;;
			return -1;
		}
	}

	DWORD mode = PIPE_READMODE_MESSAGE;
	SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);

	DWORD bytesWritten;

	if (!WriteFile(
		hPipe,
		filePath.c_str(),
		(filePath.size() + 1) * sizeof(wchar_t),
		&bytesWritten,
		NULL))
	{
		std::wcout << L"WriteFile to pipe failed. GLE=" << GetLastError() << std::endl;
		CloseHandle(hPipe);
		return -1;
	}

	wchar_t buffer[BUFFER_SIZE];
	DWORD bytesRead;

	if (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL))
	{
		std::wcout << L"Server Response: " << buffer << std::endl;
	}
	else
	{
		std::wcout << L"ReadFile failed. GLE=" << GetLastError() << std::endl;
	}
	CloseHandle(hPipe);
	std::wcout << L"Press Enter to exit...";
	std::cin.get();
	return 0;
}