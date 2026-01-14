#include <windows.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <io.h>
#include <fcntl.h>

bool IsPEFile(const std::wstring& filePath)
{
	HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Error: Cannot open file" << std::endl;
		return false;
	}

	DWORD bytesRead;
	IMAGE_DOS_HEADER dosHeader;

	// Read DOS header
	if (!ReadFile(hFile, &dosHeader, sizeof(IMAGE_DOS_HEADER), &bytesRead, NULL) || bytesRead != sizeof(IMAGE_DOS_HEADER))
	{
		CloseHandle(hFile);
		return false;
	}
	// Verify DOS signature
	if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
	{
		CloseHandle(hFile);
		return false;
	}
	// Move to PE header
	if (SetFilePointer(hFile, dosHeader.e_lfanew, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		CloseHandle(hFile);
		return false;
	}
	// Read PE signature
	DWORD peSignature;
	if (!ReadFile(hFile, &peSignature, sizeof(DWORD), &bytesRead, NULL) || bytesRead != sizeof(DWORD))
	{
		CloseHandle(hFile);
		return false;
	}
	CloseHandle(hFile);
	return peSignature == IMAGE_NT_SIGNATURE;
}

DWORD RvaToOffset(const PIMAGE_NT_HEADERS pNtHeaders, DWORD rva)
{
	PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);
	for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++)
	{
		if (rva >= pSectionHeader[i].VirtualAddress &&
			rva < (pSectionHeader[i].VirtualAddress + pSectionHeader[i].Misc.VirtualSize))
		{
			return (rva - pSectionHeader[i].VirtualAddress) + pSectionHeader[i].PointerToRawData;
		}
	}
	return 0;
}

std::wstring ToWString(const char* localString, int length = -1) {
	if (!localString || localString[0] == '\0') return L"";

	int size_needed = MultiByteToWideChar(CP_ACP, 0, localString, length, NULL, 0);
	if (size_needed <= 0) return L"";

	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, localString, length, &wstrTo[0], size_needed);

	if (!wstrTo.empty() && wstrTo.back() == L'\0') {
		wstrTo.pop_back();
	}
	return wstrTo;
}

void ParsePEFile(const std::wstring& filePath)
{
	std::wcout << L"Parsing PE file: " << filePath << std::endl;
	
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hMapping = NULL;
	LPVOID lpFileBase = NULL;
	LARGE_INTEGER fileSize;

	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeaders = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionalHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeaders = NULL;
	
	// Open the file
	hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
						NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	lpFileBase = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);

	// Get file size
	GetFileSizeEx(hFile, &fileSize);
	std::wcout << L"File Size: " << fileSize.QuadPart << L" bytes" << std::endl;

	// Parse DOS Header
	std::wcout << L"------DOS HEADER------\n";
	pDosHeader = (PIMAGE_DOS_HEADER)lpFileBase;
	std::wcout << std::hex << std::uppercase << std::setfill(L'0');
	std::wcout << L"  e_magic (Magic number)              : 0x" << std::setw(4) << pDosHeader->e_magic << std::endl;
	std::wcout << L"  e_cblp (Bytes on last page)         : 0x" << std::setw(4) << pDosHeader->e_cblp << std::endl;
	std::wcout << L"  e_cp (Pages in file)                : 0x" << std::setw(4) << pDosHeader->e_cp << std::endl;
	std::wcout << L"  e_crlc (Relocations)                : 0x" << std::setw(4) << pDosHeader->e_crlc << std::endl;
	std::wcout << L"  e_cparhdr (Size of header)          : 0x" << std::setw(4) << pDosHeader->e_cparhdr << std::endl;
	std::wcout << L"  e_minalloc (Min extra paragraphs)   : 0x" << std::setw(4) << pDosHeader->e_minalloc << std::endl;
	std::wcout << L"  e_maxalloc (Max extra paragraphs)   : 0x" << std::setw(4) << pDosHeader->e_maxalloc << std::endl;
	std::wcout << L"  e_ss (Initial SS value)             : 0x" << std::setw(4) << pDosHeader->e_ss << std::endl;
	std::wcout << L"  e_sp (Initial SP value)             : 0x" << std::setw(4) << pDosHeader->e_sp << std::endl;
	std::wcout << L"  e_csum (Checksum)                   : 0x" << std::setw(4) << pDosHeader->e_csum << std::endl;
	std::wcout << L"  e_ip (Initial IP value)             : 0x" << std::setw(4) << pDosHeader->e_ip << std::endl;
	std::wcout << L"  e_cs (Initial CS value)             : 0x" << std::setw(4) << pDosHeader->e_cs << std::endl;
	std::wcout << L"  e_lfarlc (File addr of reloc table) : 0x" << std::setw(4) << pDosHeader->e_lfarlc << std::endl;
	std::wcout << L"  e_ovno (Overlay number)             : 0x" << std::setw(4) << pDosHeader->e_ovno << std::endl;
	std::wcout << L"  e_oemid (OEM identifier)            : 0x" << std::setw(4) << pDosHeader->e_oemid << std::endl;
	std::wcout << L"  e_oeminfo (OEM information)         : 0x" << std::setw(4) << pDosHeader->e_oeminfo << std::endl;
	std::wcout << L"  e_lfanew (File addr of PE header)   : 0x" << std::setw(8) << pDosHeader->e_lfanew << std::endl;

	// Parse NT Headers
	std::wcout << L"------NT Headers------\n";
	pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)lpFileBase + pDosHeader->e_lfanew);
	std::wcout << L"  Signature                           : 0x" << std::setw(8) << pNtHeaders->Signature << std::endl;

	std::wcout << L"------------FILE HEADER------------\n";
	pFileHeader = &pNtHeaders->FileHeader;
	std::wcout << L"  Machine                             : 0x" << std::setw(4) << pFileHeader->Machine << std::endl;
	std::wcout << L"  Number of Sections                  : 0x" << std::setw(4) << pFileHeader->NumberOfSections << std::endl;
	std::wcout << L"  TimeDateStamp                       : 0x" << std::setw(8) << pFileHeader->TimeDateStamp << std::endl;
	std::wcout << L"  PointerToSymbolTable                : 0x" << std::setw(8) << pFileHeader->PointerToSymbolTable << std::endl;
	std::wcout << L"  NumberOfSymbols                     : 0x" << std::setw(8) << pFileHeader->NumberOfSymbols << std::endl;
	std::wcout << L"  SizeOfOptionalHeader                : 0x" << std::setw(4) << pFileHeader->SizeOfOptionalHeader << std::endl;
	std::wcout << L"  Characteristics                     : 0x" << std::setw(4) << pFileHeader->Characteristics << std::endl;

	std::wcout << L"------------OPTIONAL HEADER------------\n";
	pOptionalHeader = &pNtHeaders->OptionalHeader;
	std::wcout << L"  Magic                               : 0x" << std::setw(4) << pOptionalHeader->Magic << std::endl;
	std::wcout << L"  MajorLinkerVersion                  : 0x" << std::setw(2) << (UINT)pOptionalHeader->MajorLinkerVersion << std::endl;
	std::wcout << L"  MinorLinkerVersion                  : 0x" << std::setw(2) << (UINT)pOptionalHeader->MinorLinkerVersion << std::endl;
	std::wcout << L"  SizeOfCode                          : 0x" << std::setw(8) << pOptionalHeader->SizeOfCode << std::endl;
	std::wcout << L"  SizeOfInitializedData               : 0x" << std::setw(8) << pOptionalHeader->SizeOfInitializedData << std::endl;
	std::wcout << L"  SizeOfUninitializedData             : 0x" << std::setw(8) << pOptionalHeader->SizeOfUninitializedData << std::endl;
	std::wcout << L"  AddressOfEntryPoint                 : 0x" << std::setw(8) << pOptionalHeader->AddressOfEntryPoint << std::endl;
	std::wcout << L"  BaseOfCode                          : 0x" << std::setw(8) << pOptionalHeader->BaseOfCode << std::endl;

	std::wcout << L"  ImageBase                           : 0x" << std::setw(sizeof(pOptionalHeader->ImageBase) * 2) << pOptionalHeader->ImageBase << std::endl;
	std::wcout << L"  SectionAlignment                    : 0x" << std::setw(8) << pOptionalHeader->SectionAlignment << std::endl;
	std::wcout << L"  FileAlignment                       : 0x" << std::setw(8) << pOptionalHeader->FileAlignment << std::endl;
	std::wcout << L"  MajorOperatingSystemVersion         : 0x" << std::setw(4) << pOptionalHeader->MajorOperatingSystemVersion << std::endl;
	std::wcout << L"  MinorOperatingSystemVersion         : 0x" << std::setw(4) << pOptionalHeader->MinorOperatingSystemVersion << std::endl;
	std::wcout << L"  MajorImageVersion                   : 0x" << std::setw(4) << pOptionalHeader->MajorImageVersion << std::endl;
	std::wcout << L"  MinorImageVersion                   : 0x" << std::setw(4) << pOptionalHeader->MinorImageVersion << std::endl;
	std::wcout << L"  MajorSubsystemVersion               : 0x" << std::setw(4) << pOptionalHeader->MajorSubsystemVersion << std::endl;
	std::wcout << L"  MinorSubsystemVersion               : 0x" << std::setw(4) << pOptionalHeader->MinorSubsystemVersion << std::endl;
	std::wcout << L"  Win32VersionValue                   : 0x" << std::setw(8) << pOptionalHeader->Win32VersionValue << std::endl;
	std::wcout << L"  SizeOfImage                         : 0x" << std::setw(8) << pOptionalHeader->SizeOfImage << std::endl;
	std::wcout << L"  SizeOfHeaders                       : 0x" << std::setw(8) << pOptionalHeader->SizeOfHeaders << std::endl;
	std::wcout << L"  CheckSum                            : 0x" << std::setw(8) << pOptionalHeader->CheckSum << std::endl;
	std::wcout << L"  Subsystem                           : 0x" << std::setw(4) << pOptionalHeader->Subsystem << std::endl;
	std::wcout << L"  DllCharacteristics                  : 0x" << std::setw(4) << pOptionalHeader->DllCharacteristics << std::endl;
	std::wcout << L"  SizeOfStackReserve                  : 0x" << std::setw(16) << pOptionalHeader->SizeOfStackReserve << std::endl;
	std::wcout << L"  SizeOfStackCommit                   : 0x" << std::setw(16) << pOptionalHeader->SizeOfStackCommit << std::endl;
	std::wcout << L"  SizeOfHeapReserve                   : 0x" << std::setw(16) << pOptionalHeader->SizeOfHeapReserve << std::endl;
	std::wcout << L"  SizeOfHeapCommit                    : 0x" << std::setw(16) << pOptionalHeader->SizeOfHeapCommit << std::endl;
	std::wcout << L"  LoaderFlags                         : 0x" << std::setw(8) << pOptionalHeader->LoaderFlags << std::endl;
	std::wcout << L"  NumberOfRvaAndSizes                 : 0x" << std::setw(8) << pOptionalHeader->NumberOfRvaAndSizes << std::endl;

	std::wcout << L"------------------DATA DIRECTORIES------------------\n";
	const wchar_t* directoryNames[] = {
		L"Export Directory", L"Import Directory", L"Resource Directory", L"Exception Directory",
		L"Security Directory", L"Relocation Directory", L"Debug Directory", L"Architecture Directory",
		L"Reversed", L"TLS Directory", L"Configuration Directory", L"Bound Import Directory",
		L"Import Address Table Directory", L"Delay Import Directory", L".NET MetaData Directory", L"Reserved"
	};

	std::wcout << std::hex << std::uppercase << std::setfill(L' ');
	std::wcout << L"  " << std::left << std::setw(35) << L"Directory Name"
		<< L" | " << std::setw(10) << L"RVA"
		<< L" | " << std::setw(10) << L"Size" << std::endl;
	std::wcout << std::setfill(L'-') << std::setw(63) << L"" << std::setfill(L' ') << std::endl;

	for (int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) 
	{
		std::wcout << L"  " << std::left << std::setfill(L' ') << std::setw(35) << directoryNames[i] << L" : 0x"
			<< std::right << std::setfill(L'0' ) << std::setw(8) << pOptionalHeader->DataDirectory[i].VirtualAddress << L" | 0x"
			<< std::setw(8) << pOptionalHeader->DataDirectory[i].Size << std::endl;
	}

	// Parse Section Headers
	std::wcout << L"-------SECTION HEADERS-------\n";
	pSectionHeaders = IMAGE_FIRST_SECTION(pNtHeaders);
	std::wcout << std::left << std::setfill(L' ')
		<< L"  " << std::setw(10) << L"Name" << L" |"
		<< L" " << std::setw(10) << L"VirtSize" << L" |"
		<< L" " << std::setw(10) << L"VirtAddr" << L" |"
		<< L" " << std::setw(10) << L"RawSize" << L" |"
		<< L" " << std::setw(10) << L"RawAddr" << L" |"
		<< L" " << std::setw(10) << L"Relocs" << L" |"
		<< L" " << std::setw(8) << L"Characteristics" << L" |"
		<< L" " << std::setw(12) << L"Flags" << std::endl;
	
	std::wcout << std::setfill(L'-') << std::setw(95) << L"" << std::setfill(L' ') << std::endl;

	for (int i = 0; i < pFileHeader->NumberOfSections; i++)
	{
		char name[9] = { 0 };
		memcpy(name, pSectionHeaders[i].Name, 8);

		std::string sName(name);
		std::wstring wName(sName.begin(), sName.end());

		std::wcout << L"  " << std::left << std::setw(10) << wName << L" |";
		std::wcout << std::right << std::hex << std::uppercase << std::setfill(L'0');

		std::wcout << L" 0x" << std::setw(8) << pSectionHeaders[i].Misc.VirtualSize << L" |"
			<< L" 0x" << std::setw(8) << pSectionHeaders[i].VirtualAddress << L" |"
			<< L" 0x" << std::setw(8) << pSectionHeaders[i].SizeOfRawData << L" |"
			<< L" 0x" << std::setw(8) << pSectionHeaders[i].PointerToRawData << L" |"
			<< L" 0x" << std::setw(8) << pSectionHeaders[i].NumberOfRelocations << L" |"
			<< L" 0x" << std::setw(8) << pSectionHeaders[i].Characteristics << L" | ";

		DWORD chars = pSectionHeaders[i].Characteristics;
		std::wcout << std::setfill(L' ');
		if (chars & IMAGE_SCN_CNT_CODE)               std::wcout << L"CODE ";
		if (chars & IMAGE_SCN_CNT_INITIALIZED_DATA)   std::wcout << L"INIT_DATA ";
		if (chars & IMAGE_SCN_CNT_UNINITIALIZED_DATA) std::wcout << L"UNINIT_DATA ";
		if (chars & IMAGE_SCN_MEM_EXECUTE)            std::wcout << L"EXEC ";
		if (chars & IMAGE_SCN_MEM_READ)               std::wcout << L"READ ";
		if (chars & IMAGE_SCN_MEM_WRITE)              std::wcout << L"WRITE ";
		if (chars & IMAGE_SCN_MEM_DISCARDABLE)        std::wcout << L"DISCARD ";
		std::wcout << std::dec << std::endl;
	}
	// Parse Export Directory


	// Parse Import Directory
	std::wcout << "------IMPORT DIRECTORY------\n";
	PIMAGE_DATA_DIRECTORY pDataDir;
	pDataDir = &pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	
	if (pDataDir->VirtualAddress != 0)
	{
		std::wcout << std::left << std::setfill(L' ')
			<< L" " << std::setw(20) << L"DLL Name" << L" |"
			<< L" " << std::setw(10) << L"Functions" << L" |"
			<< L" " << std::setw(10) << L"OFTs" << L" |" 
			<< L" " << std::setw(10) << L"TimeDateStamp" << L" |"
			<< L" " << std::setw(10) << L"ForwarderChain" << L" |"
			<< L" " << std::setw(10) << L"Name RVA" << L" |"
			<< L" " << std::setw(10) << L"FTs (IAT)" << std::endl;

		std::wcout << std::setfill(L'-') << std::setw(120) << L"" << std::setfill(L' ') << std::endl;

		DWORD importOffset = RvaToOffset(pNtHeaders, pDataDir->VirtualAddress);
		PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE*)lpFileBase + importOffset);

		while (pImportDesc->Name != 0)
		{
			DWORD nameOffset = RvaToOffset(pNtHeaders, pImportDesc->Name);
			char* dllName = (char*)((BYTE*)lpFileBase + nameOffset);

			int functionCount = 0;
			DWORD thunkRVA = pImportDesc->OriginalFirstThunk ? pImportDesc->OriginalFirstThunk : pImportDesc->FirstThunk;
			DWORD thunkOffset = RvaToOffset(pNtHeaders, thunkRVA);
			PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((BYTE*)lpFileBase + thunkOffset);

			while (pThunk->u1.AddressOfData != 0) {
				functionCount++;
				pThunk++;
			}

			std::wcout << std::setfill(L' ');
			std::wcout << L" " << std::left << std::setw(20) << ToWString(dllName) << L" |";
			std::wcout << L" " << std::right << std::dec << std::setw(10) << functionCount << L" |";
			std::wcout << std::right << std::hex << std::uppercase << std::setfill(L'0');
			std::wcout << L" 0x" << std::setw(8) << pImportDesc->OriginalFirstThunk << L" |";
			std::wcout << L" 0x" << std::setw(8) << pImportDesc->TimeDateStamp << L" |";
			std::wcout << L" 0x" << std::setw(8) << pImportDesc->ForwarderChain << L" |";
			std::wcout << L" 0x" << std::setw(8) << pImportDesc->Name << L" |";
			std::wcout << L" 0x" << std::setw(8) << pImportDesc->FirstThunk;


			std::wcout << std::endl;
			pImportDesc++;
		}
	}
}

int main()
{
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U16TEXT);

	std::wstring path;
	std::wcout << L"Enter the file path: ";
	std::getline(std::wcin, path);

	if (IsPEFile(path)) 
	{
		std::wcout << L"✅ Đây là file PE (thực thi) hợp lệ." << std::endl;
		ParsePEFile(path);
	}
	else 
	{
		std::wcout << L"❌ Không phải file PE hoặc đường dẫn sai." << std::endl;
	}

	return 0;
}