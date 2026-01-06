#include <windows.h>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <commdlg.h>

struct FileInfo
{
	std::wstring name;
	double sizeMB;
	std::wstring created;
	std::wstring modified;
};

std::wstring FileTimeToString(const FILETIME& ft) 
{
	SYSTEMTIME stUTC, stLocal;
	FileTimeToSystemTime(&ft, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
	wchar_t buffer[256];
	swprintf_s(buffer, 256, L"%04d-%02d-%02d %02d:%02d:%02d",
		stLocal.wYear, stLocal.wMonth, stLocal.wDay,
		stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
	return std::wstring(buffer);
}

std::wstring OpenSaveDialog() 
{
	OPENFILENAMEW ofn;
	wchar_t szFile[260];

	ZeroMemory(szFile, sizeof(szFile));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetConsoleWindow();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"CSV Files (*.csv)\0*.csv\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = L"Chon noi luu file CSV";
	ofn.lpstrDefExt = L"csv";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	if (GetSaveFileNameW(&ofn) == TRUE) {
		return std::wstring(ofn.lpstrFile);
	}
	return L"";
}

std::string WStringToString(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

int main()
{
	std::wstring pathInput, extFilter, exit;
	while (true)
	{ 
		// Input
		std::wcout << "Input folder path: ";
		std::getline(std::wcin, pathInput);

		std::wcout << "Input file extension filter (e.g., .txt), or press Enter for all files: ";
		std::getline(std::wcin, extFilter);

		// Prepare search path
		if (pathInput.back() != L'\\')
		{
			pathInput += L"\\";
		}
		pathInput += L"*";

		WIN32_FIND_DATAW findData;
		HANDLE hFind = FindFirstFileW(pathInput.c_str(), &findData);

		if (hFind == INVALID_HANDLE_VALUE)
		{
			std::wcerr << "Error: Unable to open directory. (Error code: " << GetLastError() << ")\n";
			return 1;
		}

		std::cout << std::left << std::setw(85) << "File name"
			<< std::left << std::setw(20) << "Size (MB)"
			<< std::left << std::setw(25) << "Created time"
			<< "Modified time" << std::endl;

		std::vector<FileInfo> foundFiles;
		
		do
		{
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::wstring fileName = findData.cFileName;
				if (extFilter.empty() || fileName.substr(fileName.find_last_of(L'.')) == extFilter)
				{
					FileInfo info;
					info.name = fileName;
					long long sizeBytes = (static_cast<long long>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
					info.sizeMB = static_cast<double>(sizeBytes) / (1024.0 * 1024.0);
					info.created = FileTimeToString(findData.ftCreationTime);
					info.modified = FileTimeToString(findData.ftLastWriteTime);

					std::wcout << std::left << std::setw(85) << info.name
						<< std::left << std::setw(20) << std::fixed << std::setprecision(4) << info.sizeMB
						<< std::left << std::setw(25) << info.created
						<< info.modified << std::endl;
					foundFiles.push_back(info);
				}
			}
		}
		while (FindNextFileW(hFind, &findData) != 0);
		
		if (!foundFiles.empty())
		{
			std::wcout << L"\nSave results to CSV file? (y/n): ";
			wchar_t choice;
			std::wcin >> choice;

			if (choice == L'y' || choice == L'Y')
			{
				std::wstring savePath = OpenSaveDialog();

				if (!savePath.empty()) {
					std::ofstream csvFile;
					csvFile.open(savePath);

					if (csvFile.is_open()) {
						unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
						csvFile.write((char*)bom, 3);

						csvFile << "File name,Size (MB),Created time,Modified time\n";

						for (const auto& file : foundFiles) {
							csvFile << WStringToString(file.name) << ","
								<< file.sizeMB << ","
								<< WStringToString(file.created) << ","
								<< WStringToString(file.modified) << "\n";
						}
						csvFile.close();
						std::wcout << L"Da luu thanh cong tai: " << savePath << L"\n";
					}
					else {
						std::wcout << L"Loi: Khong the ghi file!\n";
					}
				}
				else {
					std::wcout << L"Da huy luu file.\n";
				}
			}
		}

		std::wcout << "Enter 'exit' to quit: ";
		std::getline(std::wcin, exit);
		if (exit == L"exit")
		{
			break;
		}
		system("cls");
	}
	return 0;
}