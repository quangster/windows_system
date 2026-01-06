#include <windows.h>    // Win32 API
#include <wincrypt.h>   // Microsoft Cryptographic API (CAPI), hash functions

#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>

std::string BytesToHex(const std::vector<BYTE>& data) 
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (BYTE b : data) {
        ss << std::setw(2) << (int)b;
    }
    return ss.str();
}

std::string CalculateFileHash(const std::string& filepath, ALG_ID algId) 
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BYTE buffer[4096];
    DWORD bytesRead = 0;
    std::string result = "";

    try 
    {
        // 1. Open file
        hFile = CreateFileA(
            filepath.c_str(),            // [in] LPCSTR lpFileName: name of file
			GENERIC_READ,                // [in] DWORD dwDesiredAccess: access mode (GENERIC_READ, GENERIC_WRITE)
			FILE_SHARE_READ,             // [in] DWORD dwShareMode: share mode (FILE_SHARE_DELETE, FILE_SHARE_READ, FILE_SHARE_WRITE)
			NULL,                        // [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes: pointer to security attributes structure
			OPEN_EXISTING,               // [in] DWORD dwCreationDisposition: action to take on files that exist, and which action to take when files do not exist
			FILE_FLAG_SEQUENTIAL_SCAN,   // [in] DWORD dwFlagsAndAttributes: file or device attributes and flags
            NULL
        );

        if (hFile == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open file: " + filepath);

		// 2. Get cryptographic context
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) 
        {
			// If the key container does not exist, create a new one
            if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET))
                throw std::runtime_error("Error CryptAcquireContext");
        }

		// 3. Create hash object
        if (!CryptCreateHash(hProv, algId, 0, 0, &hHash)) throw std::runtime_error("Error CryptCreateHash");

		// 4. Read file and hash data
        while (ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) \
        {
            if (!CryptHashData(hHash, buffer, bytesRead, 0)) throw std::runtime_error("Error CryptHashData");
        }

		// 5. Get hash size
        DWORD hashLen = 0;
        DWORD dataLen = sizeof(DWORD);
        if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashLen, &dataLen, 0)) throw std::runtime_error("Error get hash size");

		// 6. Get hash value
        std::vector<BYTE> hashValue(hashLen);
        if (!CryptGetHashParam(hHash, HP_HASHVAL, hashValue.data(), &hashLen, 0)) throw std::runtime_error("Error get hash value");

        result = BytesToHex(hashValue);
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << " (Ma loi: " << GetLastError() << ")" << std::endl;
    }

    if (hHash) CryptDestroyHash(hHash);
    if (hProv) CryptReleaseContext(hProv, 0);
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    return result;
}

int main() 
{
    std::string file1 = "test.txt";
    std::string file2 = "test_copy.txt";

    std::cout << "--- CHUONG TRINH KIEM TRA TOAN VEN FILE ---\n\n";

    // MD5
    std::cout << "[MD5] Dang tinh toan...\n";
    std::string md5_1 = CalculateFileHash(file1, CALG_MD5);
    std::string md5_2 = CalculateFileHash(file2, CALG_MD5);

    std::cout << "File 1 MD5: " << (md5_1.empty() ? "Loi" : md5_1) << "\n";
    std::cout << "File 2 MD5: " << (md5_2.empty() ? "Loi" : md5_2) << "\n\n";

    // SHA256
    std::cout << "[SHA256] Dang tinh toan...\n";
    std::string sha256_1 = CalculateFileHash(file1, CALG_SHA_256);
    std::string sha256_2 = CalculateFileHash(file2, CALG_SHA_256);

    std::cout << "File 1 SHA256: " << (sha256_1.empty() ? "Loi" : sha256_1) << "\n";
    std::cout << "File 2 SHA256: " << (sha256_2.empty() ? "Loi" : sha256_2) << "\n\n";

    // Compare
    std::cout << "--- KET QUA SO SANH ---\n";
    if (sha256_1 == sha256_2 && !sha256_1.empty())
    {
        std::cout << "=> Hai file GIONG NHAU hoan toan.\n";
    }
    else
    {
        std::cout << "=> Hai file KHAC NHAU.\n";
    }
	system("pause");

    /*
    * Lenh Windows de doi chieu:
    * Get-FileHash "C:\Users\quang\Workspaces\windows_system\W1\x64\Debug\test.txt" -Algorithm MD5
    * Get-FileHash "C:\Users\quang\Workspaces\windows_system\W1\x64\Debug\test.txt" -Algorithm SHA256
    */

    return 0;
}