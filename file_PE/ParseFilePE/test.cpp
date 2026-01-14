//#include <windows. h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//
//// ==================== Cấu trúc và biến toàn cục ====================
//HANDLE hFile = INVALID_HANDLE_VALUE;
//HANDLE hMapping = NULL;
//LPVOID lpFileBase = NULL;
//
//PIMAGE_DOS_HEADER pDosHeader = NULL;
//PIMAGE_NT_HEADERS pNtHeaders = NULL;
//PIMAGE_FILE_HEADER pFileHeader = NULL;
//PIMAGE_OPTIONAL_HEADER pOptionalHeader = NULL;
//PIMAGE_SECTION_HEADER pSectionHeaders = NULL;
//
//BOOL is64Bit = FALSE;
//
//// ==================== Hàm tiện ích ====================
//void PrintSeparator(const char* title) {
//    printf("\n");
//    printf("================================================================================\n");
//    printf("  %s\n", title);
//    printf("================================================================================\n");
//}
//
//void PrintSubSeparator(const char* title) {
//    printf("\n--- %s ---\n", title);
//}
//
//// Chuyển đổi RVA sang File Offset
//DWORD RvaToFileOffset(DWORD rva) {
//    if (!pSectionHeaders || !pFileHeader) return 0;
//
//    for (int i = 0; i < pFileHeader->NumberOfSections; i++) {
//        DWORD sectionStart = pSectionHeaders[i].VirtualAddress;
//        DWORD sectionEnd = sectionStart + pSectionHeaders[i].Misc.VirtualSize;
//
//        if (rva >= sectionStart && rva < sectionEnd) {
//            return pSectionHeaders[i].PointerToRawData + (rva - sectionStart);
//        }
//    }
//    return rva;
//}
//
//// Lấy con trỏ từ RVA
//LPVOID GetPtrFromRVA(DWORD rva) {
//    if (rva == 0) return NULL;
//    DWORD offset = RvaToFileOffset(rva);
//    return (LPVOID)((BYTE*)lpFileBase + offset);
//}
//
//// ==================== Parse DOS Header ====================
//void ParseDosHeader() {
//    PrintSeparator("DOS HEADER");
//
//    pDosHeader = (PIMAGE_DOS_HEADER)lpFileBase;
//
//    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
//        printf("[ERROR] Invalid DOS signature!\n");
//        return;
//    }
//
//    printf("  e_magic (Magic number)              : 0x%04X (MZ)\n", pDosHeader->e_magic);
//    printf("  e_cblp (Bytes on last page)         : 0x%04X (%d)\n", pDosHeader->e_cblp, pDosHeader->e_cblp);
//    printf("  e_cp (Pages in file)                : 0x%04X (%d)\n", pDosHeader->e_cp, pDosHeader->e_cp);
//    printf("  e_crlc (Relocations)                : 0x%04X (%d)\n", pDosHeader->e_crlc, pDosHeader->e_crlc);
//    printf("  e_cparhdr (Size of header)          : 0x%04X (%d paragraphs)\n", pDosHeader->e_cparhdr, pDosHeader->e_cparhdr);
//    printf("  e_minalloc (Min extra paragraphs)   : 0x%04X (%d)\n", pDosHeader->e_minalloc, pDosHeader->e_minalloc);
//    printf("  e_maxalloc (Max extra paragraphs)   : 0x%04X (%d)\n", pDosHeader->e_maxalloc, pDosHeader->e_maxalloc);
//    printf("  e_ss (Initial SS value)             : 0x%04X\n", pDosHeader->e_ss);
//    printf("  e_sp (Initial SP value)             : 0x%04X\n", pDosHeader->e_sp);
//    printf("  e_csum (Checksum)                   : 0x%04X\n", pDosHeader->e_csum);
//    printf("  e_ip (Initial IP value)             : 0x%04X\n", pDosHeader->e_ip);
//    printf("  e_cs (Initial CS value)             : 0x%04X\n", pDosHeader->e_cs);
//    printf("  e_lfarlc (File addr of reloc table) : 0x%04X\n", pDosHeader->e_lfarlc);
//    printf("  e_ovno (Overlay number)             : 0x%04X\n", pDosHeader->e_ovno);
//    printf("  e_oemid (OEM identifier)            : 0x%04X\n", pDosHeader->e_oemid);
//    printf("  e_oeminfo (OEM information)         : 0x%04X\n", pDosHeader->e_oeminfo);
//    printf("  e_lfanew (File addr of PE header)   : 0x%08lX\n", pDosHeader->e_lfanew);
//}
//
//// ==================== Parse NT Headers ====================
//void ParseNtHeaders() {
//    PrintSeparator("NT HEADERS");
//
//    pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE*)lpFileBase + pDosHeader->e_lfanew);
//
//    if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
//        printf("[ERROR] Invalid NT signature!\n");
//        return;
//    }
//
//    printf("  Signature :  0x%08lX (PE\\0\\0)\n", pNtHeaders->Signature);
//
//    // Kiểm tra 32-bit hay 64-bit
//    is64Bit = (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
//    printf("  Type      : %s\n", is64Bit ? "PE64 (64-bit)" : "PE32 (32-bit)");
//}
//
//// ==================== Parse File Header ====================
//void ParseFileHeader() {
//    PrintSeparator("FILE HEADER (COFF Header)");
//
//    pFileHeader = &pNtHeaders->FileHeader;
//
//    // Machine type
//    const char* machineType = "Unknown";
//    switch (pFileHeader->Machine) {
//    case IMAGE_FILE_MACHINE_I386:  machineType = "Intel 386 (x86)"; break;
//    case IMAGE_FILE_MACHINE_AMD64: machineType = "AMD64 (x64)"; break;
//    case IMAGE_FILE_MACHINE_IA64:  machineType = "Intel Itanium"; break;
//    case IMAGE_FILE_MACHINE_ARM:   machineType = "ARM"; break;
//    case IMAGE_FILE_MACHINE_ARM64: machineType = "ARM64"; break;
//    }
//
//    printf("  Machine                    : 0x%04X (%s)\n", pFileHeader->Machine, machineType);
//    printf("  NumberOfSections           :  0x%04X (%d)\n", pFileHeader->NumberOfSections, pFileHeader->NumberOfSections);
//
//    // Timestamp
//    time_t timestamp = pFileHeader->TimeDateStamp;
//    char timeStr[64];
//    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
//    printf("  TimeDateStamp              : 0x%08lX (%s)\n", pFileHeader->TimeDateStamp, timeStr);
//
//    printf("  PointerToSymbolTable       : 0x%08lX\n", pFileHeader->PointerToSymbolTable);
//    printf("  NumberOfSymbols            :  0x%08lX (%lu)\n", pFileHeader->NumberOfSymbols, pFileHeader->NumberOfSymbols);
//    printf("  SizeOfOptionalHeader       : 0x%04X (%d bytes)\n", pFileHeader->SizeOfOptionalHeader, pFileHeader->SizeOfOptionalHeader);
//
//    // Characteristics
//    printf("  Characteristics            : 0x%04X\n", pFileHeader->Characteristics);
//    if (pFileHeader->Characteristics & IMAGE_FILE_RELOCS_STRIPPED)         printf("    - IMAGE_FILE_RELOCS_STRIPPED\n");
//    if (pFileHeader->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)        printf("    - IMAGE_FILE_EXECUTABLE_IMAGE\n");
//    if (pFileHeader->Characteristics & IMAGE_FILE_LINE_NUMS_STRIPPED)      printf("    - IMAGE_FILE_LINE_NUMS_STRIPPED\n");
//    if (pFileHeader->Characteristics & IMAGE_FILE_LOCAL_SYMS_STRIPPED)     printf("    - IMAGE_FILE_LOCAL_SYMS_STRIPPED\n");
//    if (pFileHeader->Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)     printf("    - IMAGE_FILE_LARGE_ADDRESS_AWARE\n");
//    if (pFileHeader->Characteristics & IMAGE_FILE_32BIT_MACHINE)           printf("    - IMAGE_FILE_32BIT_MACHINE\n");
//    if (pFileHeader->Characteristics & IMAGE_FILE_DEBUG_STRIPPED)          printf("    - IMAGE_FILE_DEBUG_STRIPPED\n");
//    if (pFileHeader->Characteristics & IMAGE_FILE_DLL)                     printf("    - IMAGE_FILE_DLL\n");
//    if (pFileHeader->Characteristics & IMAGE_FILE_SYSTEM)                  printf("    - IMAGE_FILE_SYSTEM\n");
//}
//
//// ==================== Parse Optional Header ====================
//void ParseOptionalHeader() {
//    PrintSeparator("OPTIONAL HEADER");
//
//    pOptionalHeader = &pNtHeaders->OptionalHeader;
//
//    if (is64Bit) {
//        PIMAGE_OPTIONAL_HEADER64 pOpt64 = (PIMAGE_OPTIONAL_HEADER64)pOptionalHeader;
//
//        printf("  Magic                       : 0x%04X (PE32+)\n", pOpt64->Magic);
//        printf("  MajorLinkerVersion          : %d\n", pOpt64->MajorLinkerVersion);
//        printf("  MinorLinkerVersion          : %d\n", pOpt64->MinorLinkerVersion);
//        printf("  SizeOfCode                  : 0x%08lX (%lu bytes)\n", pOpt64->SizeOfCode, pOpt64->SizeOfCode);
//        printf("  SizeOfInitializedData       : 0x%08lX (%lu bytes)\n", pOpt64->SizeOfInitializedData, pOpt64->SizeOfInitializedData);
//        printf("  SizeOfUninitializedData     : 0x%08lX (%lu bytes)\n", pOpt64->SizeOfUninitializedData, pOpt64->SizeOfUninitializedData);
//        printf("  AddressOfEntryPoint         :  0x%08lX\n", pOpt64->AddressOfEntryPoint);
//        printf("  BaseOfCode                  : 0x%08lX\n", pOpt64->BaseOfCode);
//        printf("  ImageBase                   : 0x%016llX\n", pOpt64->ImageBase);
//        printf("  SectionAlignment            : 0x%08lX (%lu bytes)\n", pOpt64->SectionAlignment, pOpt64->SectionAlignment);
//        printf("  FileAlignment               : 0x%08lX (%lu bytes)\n", pOpt64->FileAlignment, pOpt64->FileAlignment);
//        printf("  MajorOperatingSystemVersion : %d\n", pOpt64->MajorOperatingSystemVersion);
//        printf("  MinorOperatingSystemVersion : %d\n", pOpt64->MinorOperatingSystemVersion);
//        printf("  MajorImageVersion           : %d\n", pOpt64->MajorImageVersion);
//        printf("  MinorImageVersion           : %d\n", pOpt64->MinorImageVersion);
//        printf("  MajorSubsystemVersion       : %d\n", pOpt64->MajorSubsystemVersion);
//        printf("  MinorSubsystemVersion       : %d\n", pOpt64->MinorSubsystemVersion);
//        printf("  Win32VersionValue           : 0x%08lX\n", pOpt64->Win32VersionValue);
//        printf("  SizeOfImage                 : 0x%08lX (%lu bytes)\n", pOpt64->SizeOfImage, pOpt64->SizeOfImage);
//        printf("  SizeOfHeaders               : 0x%08lX (%lu bytes)\n", pOpt64->SizeOfHeaders, pOpt64->SizeOfHeaders);
//        printf("  CheckSum                    :  0x%08lX\n", pOpt64->CheckSum);
//
//        const char* subsystem = "Unknown";
//        switch (pOpt64->Subsystem) {
//        case IMAGE_SUBSYSTEM_NATIVE:                subsystem = "Native"; break;
//        case IMAGE_SUBSYSTEM_WINDOWS_GUI:           subsystem = "Windows GUI"; break;
//        case IMAGE_SUBSYSTEM_WINDOWS_CUI:           subsystem = "Windows Console"; break;
//        case IMAGE_SUBSYSTEM_POSIX_CUI:             subsystem = "POSIX Console"; break;
//        case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:        subsystem = "Windows CE GUI"; break;
//        case IMAGE_SUBSYSTEM_EFI_APPLICATION:       subsystem = "EFI Application"; break;
//        }
//        printf("  Subsystem                   : 0x%04X (%s)\n", pOpt64->Subsystem, subsystem);
//
//        printf("  DllCharacteristics          : 0x%04X\n", pOpt64->DllCharacteristics);
//        if (pOpt64->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA)  printf("    - HIGH_ENTROPY_VA\n");
//        if (pOpt64->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)     printf("    - DYNAMIC_BASE (ASLR)\n");
//        if (pOpt64->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY)  printf("    - FORCE_INTEGRITY\n");
//        if (pOpt64->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NX_COMPAT)        printf("    - NX_COMPAT (DEP)\n");
//        if (pOpt64->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_SEH)           printf("    - NO_SEH\n");
//        if (pOpt64->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE) printf("    - TERMINAL_SERVER_AWARE\n");
//
//        printf("  SizeOfStackReserve          : 0x%016llX\n", pOpt64->SizeOfStackReserve);
//        printf("  SizeOfStackCommit           : 0x%016llX\n", pOpt64->SizeOfStackCommit);
//        printf("  SizeOfHeapReserve           : 0x%016llX\n", pOpt64->SizeOfHeapReserve);
//        printf("  SizeOfHeapCommit            : 0x%016llX\n", pOpt64->SizeOfHeapCommit);
//        printf("  LoaderFlags                 : 0x%08lX\n", pOpt64->LoaderFlags);
//        printf("  NumberOfRvaAndSizes         : 0x%08lX (%lu)\n", pOpt64->NumberOfRvaAndSizes, pOpt64->NumberOfRvaAndSizes);
//    }
//    else {
//        PIMAGE_OPTIONAL_HEADER32 pOpt32 = (PIMAGE_OPTIONAL_HEADER32)pOptionalHeader;
//
//        printf("  Magic                       :  0x%04X (PE32)\n", pOpt32->Magic);
//        printf("  MajorLinkerVersion          : %d\n", pOpt32->MajorLinkerVersion);
//        printf("  MinorLinkerVersion          : %d\n", pOpt32->MinorLinkerVersion);
//        printf("  SizeOfCode                  : 0x%08lX (%lu bytes)\n", pOpt32->SizeOfCode, pOpt32->SizeOfCode);
//        printf("  SizeOfInitializedData       : 0x%08lX (%lu bytes)\n", pOpt32->SizeOfInitializedData, pOpt32->SizeOfInitializedData);
//        printf("  SizeOfUninitializedData     : 0x%08lX (%lu bytes)\n", pOpt32->SizeOfUninitializedData, pOpt32->SizeOfUninitializedData);
//        printf("  AddressOfEntryPoint         :  0x%08lX\n", pOpt32->AddressOfEntryPoint);
//        printf("  BaseOfCode                  : 0x%08lX\n", pOpt32->BaseOfCode);
//        printf("  BaseOfData                  :  0x%08lX\n", pOpt32->BaseOfData);
//        printf("  ImageBase                   : 0x%08lX\n", pOpt32->ImageBase);
//        printf("  SectionAlignment            : 0x%08lX (%lu bytes)\n", pOpt32->SectionAlignment, pOpt32->SectionAlignment);
//        printf("  FileAlignment               : 0x%08lX (%lu bytes)\n", pOpt32->FileAlignment, pOpt32->FileAlignment);
//        printf("  MajorOperatingSystemVersion : %d\n", pOpt32->MajorOperatingSystemVersion);
//        printf("  MinorOperatingSystemVersion : %d\n", pOpt32->MinorOperatingSystemVersion);
//        printf("  MajorImageVersion           : %d\n", pOpt32->MajorImageVersion);
//        printf("  MinorImageVersion           : %d\n", pOpt32->MinorImageVersion);
//        printf("  MajorSubsystemVersion       : %d\n", pOpt32->MajorSubsystemVersion);
//        printf("  MinorSubsystemVersion       : %d\n", pOpt32->MinorSubsystemVersion);
//        printf("  Win32VersionValue           : 0x%08lX\n", pOpt32->Win32VersionValue);
//        printf("  SizeOfImage                 :  0x%08lX (%lu bytes)\n", pOpt32->SizeOfImage, pOpt32->SizeOfImage);
//        printf("  SizeOfHeaders               :  0x%08lX (%lu bytes)\n", pOpt32->SizeOfHeaders, pOpt32->SizeOfHeaders);
//        printf("  CheckSum                    : 0x%08lX\n", pOpt32->CheckSum);
//
//        const char* subsystem = "Unknown";
//        switch (pOpt32->Subsystem) {
//        case IMAGE_SUBSYSTEM_NATIVE:                subsystem = "Native"; break;
//        case IMAGE_SUBSYSTEM_WINDOWS_GUI:           subsystem = "Windows GUI"; break;
//        case IMAGE_SUBSYSTEM_WINDOWS_CUI:           subsystem = "Windows Console"; break;
//        case IMAGE_SUBSYSTEM_POSIX_CUI:              subsystem = "POSIX Console"; break;
//        case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:        subsystem = "Windows CE GUI"; break;
//        }
//        printf("  Subsystem                   : 0x%04X (%s)\n", pOpt32->Subsystem, subsystem);
//
//        printf("  DllCharacteristics          :  0x%04X\n", pOpt32->DllCharacteristics);
//        if (pOpt32->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)     printf("    - DYNAMIC_BASE (ASLR)\n");
//        if (pOpt32->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY)  printf("    - FORCE_INTEGRITY\n");
//        if (pOpt32->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NX_COMPAT)        printf("    - NX_COMPAT (DEP)\n");
//        if (pOpt32->DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_SEH)           printf("    - NO_SEH\n");
//
//        printf("  SizeOfStackReserve          : 0x%08lX\n", pOpt32->SizeOfStackReserve);
//        printf("  SizeOfStackCommit           : 0x%08lX\n", pOpt32->SizeOfStackCommit);
//        printf("  SizeOfHeapReserve           : 0x%08lX\n", pOpt32->SizeOfHeapReserve);
//        printf("  SizeOfHeapCommit            : 0x%08lX\n", pOpt32->SizeOfHeapCommit);
//        printf("  LoaderFlags                 :  0x%08lX\n", pOpt32->LoaderFlags);
//        printf("  NumberOfRvaAndSizes         : 0x%08lX (%lu)\n", pOpt32->NumberOfRvaAndSizes, pOpt32->NumberOfRvaAndSizes);
//    }
//}
//
//// ==================== Parse Data Directories ====================
//void ParseDataDirectories() {
//    PrintSeparator("DATA DIRECTORIES");
//
//    const char* dirNames[] = {
//        "Export Table",
//        "Import Table",
//        "Resource Table",
//        "Exception Table",
//        "Certificate Table",
//        "Base Relocation Table",
//        "Debug",
//        "Architecture",
//        "Global Ptr",
//        "TLS Table",
//        "Load Config Table",
//        "Bound Import",
//        "IAT",
//        "Delay Import Descriptor",
//        "CLR Runtime Header",
//        "Reserved"
//    };
//
//    PIMAGE_DATA_DIRECTORY pDataDir;
//    DWORD numDirs;
//
//    if (is64Bit) {
//        PIMAGE_OPTIONAL_HEADER64 pOpt64 = (PIMAGE_OPTIONAL_HEADER64)pOptionalHeader;
//        pDataDir = pOpt64->DataDirectory;
//        numDirs = pOpt64->NumberOfRvaAndSizes;
//    }
//    else {
//        PIMAGE_OPTIONAL_HEADER32 pOpt32 = (PIMAGE_OPTIONAL_HEADER32)pOptionalHeader;
//        pDataDir = pOpt32->DataDirectory;
//        numDirs = pOpt32->NumberOfRvaAndSizes;
//    }
//
//    printf("  %-4s %-28s %-12s %-12s\n", "No.", "Name", "RVA", "Size");
//    printf("  %-4s %-28s %-12s %-12s\n", "----", "----------------------------", "------------", "------------");
//
//    for (DWORD i = 0; i < numDirs && i < 16; i++) {
//        printf("  [%2lu] %-28s 0x%08lX   0x%08lX\n",
//            i, dirNames[i],
//            pDataDir[i].VirtualAddress,
//            pDataDir[i].Size);
//    }
//}
//
//// ==================== Parse Section Headers ====================
//void ParseSectionHeaders() {
//    PrintSeparator("SECTION HEADERS");
//
//    pSectionHeaders = IMAGE_FIRST_SECTION(pNtHeaders);
//
//    printf("  %-8s %-10s %-10s %-10s %-10s %-10s %-12s\n",
//        "Name", "VirtSize", "VirtAddr", "RawSize", "RawAddr", "Relocs", "Characteristics");
//    printf("  %-8s %-10s %-10s %-10s %-10s %-10s %-12s\n",
//        "--------", "----------", "----------", "----------", "----------", "----------", "------------");
//
//    for (int i = 0; i < pFileHeader->NumberOfSections; i++) {
//        char name[9] = { 0 };
//        memcpy(name, pSectionHeaders[i].Name, 8);
//
//        printf("  %-8s 0x%08lX 0x%08lX 0x%08lX 0x%08lX 0x%08X 0x%08lX\n",
//            name,
//            pSectionHeaders[i].Misc.VirtualSize,
//            pSectionHeaders[i].VirtualAddress,
//            pSectionHeaders[i].SizeOfRawData,
//            pSectionHeaders[i].PointerToRawData,
//            pSectionHeaders[i].NumberOfRelocations,
//            pSectionHeaders[i].Characteristics);
//
//        // Hiển thị characteristics
//        DWORD chars = pSectionHeaders[i].Characteristics;
//        printf("           Flags: ");
//        if (chars & IMAGE_SCN_CNT_CODE)               printf("CODE ");
//        if (chars & IMAGE_SCN_CNT_INITIALIZED_DATA)   printf("INIT_DATA ");
//        if (chars & IMAGE_SCN_CNT_UNINITIALIZED_DATA) printf("UNINIT_DATA ");
//        if (chars & IMAGE_SCN_MEM_EXECUTE)            printf("EXEC ");
//        if (chars & IMAGE_SCN_MEM_READ)               printf("READ ");
//        if (chars & IMAGE_SCN_MEM_WRITE)              printf("WRITE ");
//        if (chars & IMAGE_SCN_MEM_DISCARDABLE)        printf("DISCARD ");
//        printf("\n");
//    }
//}
//
//// ==================== Parse Export Directory ====================
//void ParseExportDirectory() {
//    PrintSeparator("EXPORT DIRECTORY");
//
//    PIMAGE_DATA_DIRECTORY pDataDir;
//    if (is64Bit) {
//        pDataDir = ((PIMAGE_OPTIONAL_HEADER64)pOptionalHeader)->DataDirectory;
//    }
//    else {
//        pDataDir = ((PIMAGE_OPTIONAL_HEADER32)pOptionalHeader)->DataDirectory;
//    }
//
//    if (pDataDir[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0) {
//        printf("  No Export Directory found.\n");
//        return;
//    }
//
//    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)GetPtrFromRVA(
//        pDataDir[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
//
//    if (!pExportDir) {
//        printf("  Unable to read Export Directory.\n");
//        return;
//    }
//
//    char* dllName = (char*)GetPtrFromRVA(pExportDir->Name);
//
//    printf("  DLL Name                  : %s\n", dllName ? dllName : "N/A");
//    printf("  Characteristics           : 0x%08lX\n", pExportDir->Characteristics);
//    printf("  TimeDateStamp             : 0x%08lX\n", pExportDir->TimeDateStamp);
//    printf("  MajorVersion              : %d\n", pExportDir->MajorVersion);
//    printf("  MinorVersion              : %d\n", pExportDir->MinorVersion);
//    printf("  Base                      : %lu\n", pExportDir->Base);
//    printf("  NumberOfFunctions         : %lu\n", pExportDir->NumberOfFunctions);
//    printf("  NumberOfNames             : %lu\n", pExportDir->NumberOfNames);
//    printf("  AddressOfFunctions        : 0x%08lX\n", pExportDir->AddressOfFunctions);
//    printf("  AddressOfNames            : 0x%08lX\n", pExportDir->AddressOfNames);
//    printf("  AddressOfNameOrdinals     : 0x%08lX\n", pExportDir->AddressOfNameOrdinals);
//
//    // Hiển thị danh sách các hàm export
//    if (pExportDir->NumberOfNames > 0) {
//        PrintSubSeparator("Exported Functions");
//
//        DWORD* pNames = (DWORD*)GetPtrFromRVA(pExportDir->AddressOfNames);
//        WORD* pOrdinals = (WORD*)GetPtrFromRVA(pExportDir->AddressOfNameOrdinals);
//        DWORD* pFunctions = (DWORD*)GetPtrFromRVA(pExportDir->AddressOfFunctions);
//
//        printf("  %-6s %-12s %s\n", "Ord", "RVA", "Name");
//        printf("  %-6s %-12s %s\n", "------", "------------", "--------------------");
//
//        DWORD maxDisplay = min(pExportDir->NumberOfNames, (DWORD)50);
//        for (DWORD i = 0; i < maxDisplay; i++) {
//            char* funcName = (char*)GetPtrFromRVA(pNames[i]);
//            WORD ordinal = pOrdinals[i] + (WORD)pExportDir->Base;
//            DWORD funcRva = pFunctions[pOrdinals[i]];
//
//            printf("  %-6d 0x%08lX   %s\n", ordinal, funcRva, funcName ? funcName : "N/A");
//        }
//
//        if (pExportDir->NumberOfNames > 50) {
//            printf("  ... and %lu more functions\n", pExportDir->NumberOfNames - 50);
//        }
//    }
//}
//
//// ==================== Parse Import Directory ====================
//void ParseImportDirectory() {
//    PrintSeparator("IMPORT DIRECTORY");
//
//    PIMAGE_DATA_DIRECTORY pDataDir;
//    if (is64Bit) {
//        pDataDir = ((PIMAGE_OPTIONAL_HEADER64)pOptionalHeader)->DataDirectory;
//    }
//    else {
//        pDataDir = ((PIMAGE_OPTIONAL_HEADER32)pOptionalHeader)->DataDirectory;
//    }
//
//    if (pDataDir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress == 0) {
//        printf("  No Import Directory found.\n");
//        return;
//    }
//
//    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)GetPtrFromRVA(
//        pDataDir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
//
//    if (!pImportDesc) {
//        printf("  Unable to read Import Directory.\n");
//        return;
//    }
//
//    int dllCount = 0;
//
//    while (pImportDesc->Name != 0) {
//        char* dllName = (char*)GetPtrFromRVA(pImportDesc->Name);
//
//        printf("\n  [%d] DLL:  %s\n", dllCount + 1, dllName ? dllName : "N/A");
//        printf("      OriginalFirstThunk :  0x%08lX\n", pImportDesc->OriginalFirstThunk);
//        printf("      TimeDateStamp      : 0x%08lX\n", pImportDesc->TimeDateStamp);
//        printf("      ForwarderChain     : 0x%08lX\n", pImportDesc->ForwarderChain);
//        printf("      Name RVA           : 0x%08lX\n", pImportDesc->Name);
//        printf("      FirstThunk (IAT)   : 0x%08lX\n", pImportDesc->FirstThunk);
//
//        // Hiển thị các hàm import
//        DWORD thunkRva = pImportDesc->OriginalFirstThunk ?
//            pImportDesc->OriginalFirstThunk : pImportDesc->FirstThunk;
//
//        if (is64Bit) {
//            PIMAGE_THUNK_DATA64 pThunk = (PIMAGE_THUNK_DATA64)GetPtrFromRVA(thunkRva);
//            int funcCount = 0;
//
//            printf("      Imported Functions:\n");
//            while (pThunk && pThunk->u1.AddressOfData != 0 && funcCount < 20) {
//                if (pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64) {
//                    printf("        - Ordinal:  %llu\n", IMAGE_ORDINAL64(pThunk->u1.Ordinal));
//                }
//                else {
//                    PIMAGE_IMPORT_BY_NAME pImportName =
//                        (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA((DWORD)pThunk->u1.AddressOfData);
//                    if (pImportName) {
//                        printf("        - %s (Hint: %d)\n", pImportName->Name, pImportName->Hint);
//                    }
//                }
//                pThunk++;
//                funcCount++;
//            }
//
//            // Đếm tổng số hàm
//            PIMAGE_THUNK_DATA64 pCount = (PIMAGE_THUNK_DATA64)GetPtrFromRVA(thunkRva);
//            int total = 0;
//            while (pCount && pCount->u1.AddressOfData != 0) {
//                total++;
//                pCount++;
//            }
//            if (total > 20) {
//                printf("        ... and %d more functions\n", total - 20);
//            }
//        }
//        else {
//            PIMAGE_THUNK_DATA32 pThunk = (PIMAGE_THUNK_DATA32)GetPtrFromRVA(thunkRva);
//            int funcCount = 0;
//
//            printf("      Imported Functions:\n");
//            while (pThunk && pThunk->u1.AddressOfData != 0 && funcCount < 20) {
//                if (pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG32) {
//                    printf("        - Ordinal: %lu\n", IMAGE_ORDINAL32(pThunk->u1.Ordinal));
//                }
//                else {
//                    PIMAGE_IMPORT_BY_NAME pImportName =
//                        (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(pThunk->u1.AddressOfData);
//                    if (pImportName) {
//                        printf("        - %s (Hint: %d)\n", pImportName->Name, pImportName->Hint);
//                    }
//                }
//                pThunk++;
//                funcCount++;
//            }
//
//            // Đếm tổng số hàm
//            PIMAGE_THUNK_DATA32 pCount = (PIMAGE_THUNK_DATA32)GetPtrFromRVA(thunkRva);
//            int total = 0;
//            while (pCount && pCount->u1.AddressOfData != 0) {
//                total++;
//                pCount++;
//            }
//            if (total > 20) {
//                printf("        ... and %d more functions\n", total - 20);
//            }
//        }
//
//        pImportDesc++;
//        dllCount++;
//    }
//
//    printf("\n  Total DLLs imported: %d\n", dllCount);
//}
//
//// ==================== Parse Resource Directory ====================
//void ParseResourceDirectoryRecursive(PIMAGE_RESOURCE_DIRECTORY pResDir,
//    PIMAGE_RESOURCE_DIRECTORY pBaseResDir,
//    int level,
//    const char* typeName) {
//    if (!pResDir || level > 3) return;
//
//    char indent[32];
//    memset(indent, ' ', level * 4);
//    indent[level * 4] = '\0';
//
//    int numEntries = pResDir->NumberOfNamedEntries + pResDir->NumberOfIdEntries;
//    PIMAGE_RESOURCE_DIRECTORY_ENTRY pEntry =
//        (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResDir + 1);
//
//    const char* resourceTypes[] = {
//        "??? ", "CURSOR", "BITMAP", "ICON", "MENU", "DIALOG", "STRING",
//        "FONTDIR", "FONT", "ACCELERATOR", "RCDATA", "MESSAGETABLE",
//        "GROUP_CURSOR", "??? ", "GROUP_ICON", "??? ", "VERSION", "DLGINCLUDE",
//        "??? ", "PLUGPLAY", "VXD", "ANICURSOR", "ANIICON", "HTML", "MANIFEST"
//    };
//
//    for (int i = 0; i < numEntries && i < 50; i++) {
//        char name[256] = { 0 };
//
//        if (pEntry[i].NameIsString) {
//            PIMAGE_RESOURCE_DIR_STRING_U pName =
//                (PIMAGE_RESOURCE_DIR_STRING_U)((BYTE*)pBaseResDir + pEntry[i].NameOffset);
//            WideCharToMultiByte(CP_ACP, 0, pName->NameString, pName->Length,
//                name, sizeof(name) - 1, NULL, NULL);
//        }
//        else {
//            if (level == 0 && pEntry[i].Id < 25) {
//                strcpy(name, resourceTypes[pEntry[i].Id]);
//            }
//            else {
//                sprintf(name, "ID:  %d", pEntry[i].Id);
//            }
//        }
//
//        if (pEntry[i].DataIsDirectory) {
//            printf("%s[DIR] %s\n", indent, name);
//            PIMAGE_RESOURCE_DIRECTORY pSubDir =
//                (PIMAGE_RESOURCE_DIRECTORY)((BYTE*)pBaseResDir + pEntry[i].OffsetToDirectory);
//            ParseResourceDirectoryRecursive(pSubDir, pBaseResDir, level + 1, name);
//        }
//        else {
//            PIMAGE_RESOURCE_DATA_ENTRY pDataEntry =
//                (PIMAGE_RESOURCE_DATA_ENTRY)((BYTE*)pBaseResDir + pEntry[i].OffsetToData);
//            printf("%s[DATA] %s - RVA: 0x%08lX, Size: %lu bytes\n",
//                indent, name, pDataEntry->OffsetToData, pDataEntry->Size);
//        }
//    }
//}
//
//void ParseResourceDirectory() {
//    PrintSeparator("RESOURCE DIRECTORY");
//
//    PIMAGE_DATA_DIRECTORY pDataDir;
//    if (is64Bit) {
//        pDataDir = ((PIMAGE_OPTIONAL_HEADER64)pOptionalHeader)->DataDirectory;
//    }
//    else {
//        pDataDir = ((PIMAGE_OPTIONAL_HEADER32)pOptionalHeader)->DataDirectory;
//    }
//
//    if (pDataDir[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress == 0) {
//        printf("  No Resource Directory found.\n");
//        return;
//    }
//
//    PIMAGE_RESOURCE_DIRECTORY pResDir = (PIMAGE_RESOURCE_DIRECTORY)GetPtrFromRVA(
//        pDataDir[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
//
//    if (!pResDir) {
//        printf("  Unable to read Resource Directory.\n");
//        return;
//    }
//
//    printf("  Characteristics       : 0x%08lX\n", pResDir->Characteristics);
//    printf("  TimeDateStamp         : 0x%08lX\n", pResDir->TimeDateStamp);
//    printf("  MajorVersion          : %d\n", pResDir->MajorVersion);
//    printf("  MinorVersion          :  %d\n", pResDir->MinorVersion);
//    printf("  NumberOfNamedEntries  : %d\n", pResDir->NumberOfNamedEntries);
//    printf("  NumberOfIdEntries     : %d\n", pResDir->NumberOfIdEntries);
//
//    PrintSubSeparator("Resource Tree");
//    ParseResourceDirectoryRecursive(pResDir, pResDir, 0, NULL);
//}
//
//// ==================== Parse Relocation Directory ====================
//void ParseRelocationDirectory() {
//    PrintSeparator("RELOCATION DIRECTORY (Base Relocations)");
//
//    PIMAGE_DATA_DIRECTORY pDataDir;
//    if (is64Bit) {
//        pDataDir = ((PIMAGE_OPTIONAL_HEADER64)pOptionalHeader)->DataDirectory;
//    }
//    else {
//        pDataDir = ((PIMAGE_OPTIONAL_HEADER32)pOptionalHeader)->DataDirectory;
//    }
//
//    if (pDataDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress == 0) {
//        printf("  No Relocation Directory found.\n");
//        return;
//    }
//
//    PIMAGE_BASE_RELOCATION pReloc = (PIMAGE_BASE_RELOCATION)GetPtrFromRVA(
//        pDataDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
//
//    if (!pReloc) {
//        printf("  Unable to read Relocation Directory.\n");
//        return;
//    }
//
//    DWORD totalSize = pDataDir[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
//    DWORD processed = 0;
//    int blockCount = 0;
//
//    printf("  Total Relocation Size: %lu bytes\n\n", totalSize);
//
//    const char* relocTypes[] = {
//        "ABSOLUTE", "HIGH", "LOW", "HIGHLOW", "HIGHADJ",
//        "MIPS_JMPADDR", "??? ", "??? ", "???", "MIPS_JMPADDR16",
//        "DIR64"
//    };
//
//    while (processed < totalSize && pReloc->SizeOfBlock > 0) {
//        int numEntries = (pReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
//
//        printf("  [Block %d] Page RVA: 0x%08lX, Block Size: %lu, Entries: %d\n",
//            blockCount + 1, pReloc->VirtualAddress, pReloc->SizeOfBlock, numEntries);
//
//        WORD* pEntry = (WORD*)((BYTE*)pReloc + sizeof(IMAGE_BASE_RELOCATION));
//
//        // Hiển thị một số entry đầu tiên
//        int displayCount = min(numEntries, 10);
//        for (int i = 0; i < displayCount; i++) {
//            WORD type = pEntry[i] >> 12;
//            WORD offset = pEntry[i] & 0x0FFF;
//            DWORD rva = pReloc->VirtualAddress + offset;
//
//            const char* typeName = (type < 11) ? relocTypes[type] : "UNKNOWN";
//            printf("      [%4d] Type: %-12s Offset: 0x%03X -> RVA: 0x%08lX\n",
//                i, typeName, offset, rva);
//        }
//
//        if (numEntries > 10) {
//            printf("      ...  and %d more entries\n", numEntries - 10);
//        }
//        printf("\n");
//
//        processed += pReloc->SizeOfBlock;
//        pReloc = (PIMAGE_BASE_RELOCATION)((BYTE*)pReloc + pReloc->SizeOfBlock);
//        blockCount++;
//
//        if (blockCount >= 20) {
//            printf("  ...  and more blocks (showing first 20)\n");
//            break;
//        }
//    }
//
//    printf("  Total Relocation Blocks: %d\n", blockCount);
//}
//
//// ==================== Main Function ====================
//BOOL OpenPEFile(const char* filename) {
//    hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ,
//        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//
//    if (hFile == INVALID_HANDLE_VALUE) {
//        printf("[ERROR] Cannot open file: %s (Error:  %lu)\n", filename, GetLastError());
//        return FALSE;
//    }
//
//    hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
//    if (!hMapping) {
//        printf("[ERROR] Cannot create file mapping (Error: %lu)\n", GetLastError());
//        CloseHandle(hFile);
//        return FALSE;
//    }
//
//    lpFileBase = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
//    if (!lpFileBase) {
//        printf("[ERROR] Cannot map view of file (Error:  %lu)\n", GetLastError());
//        CloseHandle(hMapping);
//        CloseHandle(hFile);
//        return FALSE;
//    }
//
//    return TRUE;
//}
//
//void ClosePEFile() {
//    if (lpFileBase) UnmapViewOfFile(lpFileBase);
//    if (hMapping) CloseHandle(hMapping);
//    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
//}
//
//void PrintBanner() {
//    printf("\n");
//    printf(" ____  _____   ____                          \n");
//    printf("|  _ \\| ____| |  _ \\ __ _ _ __ ___  ___ _ __ \n");
//    printf("| |_) |  _|   | |_) / _` | '__/ __|/ _ \\ '__|\n");
//    printf("|  __/| |___  |  __/ (_| | |  \\__ \\  __/ |   \n");
//    printf("|_|   |_____| |_|   \\__,_|_|  |___/\\___|_|   \n");
//    printf("\n");
//    printf("PE File Analyzer - Similar to CFF Explorer\n");
//    printf("================================================================================\n");
//}
//
//int main(int argc, char* argv[]) {
//    PrintBanner();
//
//    char filename[MAX_PATH];
//
//    if (argc < 2) {
//        printf("Enter PE file path: ");
//        if (fgets(filename, sizeof(filename), stdin)) {
//            // Loại bỏ newline
//            size_t len = strlen(filename);
//            if (len > 0 && filename[len - 1] == '\n') {
//                filename[len - 1] = '\0';
//            }
//        }
//    }
//    else {
//        strcpy(filename, argv[1]);
//    }
//
//    printf("Analyzing:  %s\n", filename);
//
//    if (!OpenPEFile(filename)) {
//        return 1;
//    }
//
//    // Lấy thông tin file    
//    LARGE_INTEGER fileSize;
//    GetFileSizeEx(hFile, &fileSize);
//    printf("File Size: %lld bytes\n", fileSize.QuadPart);
//
//    // Parse tất cả headers
//    ParseDosHeader();
//
//    if (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
//        ParseNtHeaders();
//
//        if (pNtHeaders->Signature == IMAGE_NT_SIGNATURE) {
//            ParseFileHeader();
//            ParseOptionalHeader();
//            ParseDataDirectories();
//            ParseSectionHeaders();
//            ParseExportDirectory();
//            ParseImportDirectory();
//            ParseResourceDirectory();
//            ParseRelocationDirectory();
//        }
//    }
//
//    ClosePEFile();
//
//    printf("\n================================================================================\n");
//    printf("Analysis Complete!\n");
//    printf("================================================================================\n");
//
//    return 0;
//}