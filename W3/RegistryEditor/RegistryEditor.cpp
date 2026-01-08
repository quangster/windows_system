#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

struct RegistryInfo
{
	HKEY rootKey;
	wstring subKey;
	wstring valueName;
	string rootKeyName;
};

string ToNarrow(const wstring& wstr) {
	if (wstr.empty()) return "";
	int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	string str(size - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
	return str;
}

bool GetRootKey(const string& name, HKEY& rootKey, string& rootKeyName)
{
	rootKeyName = "";
	if (name == "HKEY_LOCAL_MACHINE" || name == "HKLM")
	{
		rootKey = HKEY_LOCAL_MACHINE;
		rootKeyName = "HKEY_LOCAL_MACHINE";
	}
	else if (name == "HKEY_CURRENT_USER" || name == "HKCU")
	{
		rootKey = HKEY_CURRENT_USER;
		rootKeyName = "HKEY_CURRENT_USER";
	}
	else if (name == "HKEY_CLASSES_ROOT" || name == "HKCR")
	{
		rootKey = HKEY_CLASSES_ROOT;
		rootKeyName = "HKEY_CLASSES_ROOT";
	}
	else if (name == "HKEY_USERS" || name == "HKU")
	{
		rootKey = HKEY_USERS;
		rootKeyName = "HKEY_USERS";
	}
	else if (name == "HKEY_CURRENT_CONFIG" || name == "HKCC")
	{
		rootKey = HKEY_CURRENT_CONFIG;
		rootKeyName = "HKEY_CURRENT_CONFIG";
	}
	if (rootKeyName.empty())
	{
		return false;
	}
	return true;
}

bool ParseRegistryPath(const string& fullPath, RegistryInfo& info)
{
	string path = fullPath;

	// Remove "Computer\" prefix if present
	if (path.substr(0, 9) == "Computer\\")
	{
		path = path.substr(9);
	}

	// Find the first backslash to separate root key and subkey
	size_t firstSlash = path.find('\\');
	if (firstSlash == string::npos)
	{
		return false; // Invalid format
	}

	// Extract root key string
	string rootKeyStr = path.substr(0, firstSlash);

	// Get the root key handle
	if (!GetRootKey(rootKeyStr, info.rootKey, info.rootKeyName))
	{
		return false; // Invalid root key
	}

	// Extract subkey and value name
	info.subKey = wstring(path.begin() + firstSlash + 1, path.end());

	return true;
}

string GetTypeName(DWORD type)
{
	switch (type) {
	case REG_NONE:              return "REG_NONE";
	case REG_SZ:                return "REG_SZ";
	case REG_EXPAND_SZ:         return "REG_EXPAND_SZ";
	case REG_BINARY:            return "REG_BINARY";
	case REG_DWORD:             return "REG_DWORD";
	case REG_DWORD_BIG_ENDIAN:  return "REG_DWORD_BIG_ENDIAN";
	case REG_LINK:              return "REG_LINK";
	case REG_MULTI_SZ:          return "REG_MULTI_SZ";
	case REG_QWORD:             return "REG_QWORD";
	default:                    return "UNKNOWN (" + to_string(type) + ")";
	}
}

void PrintData(const DWORD& type, const BYTE* data, const DWORD& size) {
	switch (type) {
	case REG_SZ:
	case REG_EXPAND_SZ:
		wcout << (const wchar_t*)data;
		break;

	case REG_DWORD: {
		DWORD value = *(const DWORD*)data;
		cout << value << " (0x" << hex << value << dec << ")";
		break;
	}

	case REG_QWORD: {
		ULONGLONG value = *(const ULONGLONG*)data;
		cout << value << " (0x" << hex << value << dec << ")";
		break;
	}

	case REG_BINARY: {
		for (DWORD i = 0; i < size; i++) {
			cout << hex << setw(2) << setfill('0') << (int)data[i] << " ";
			if ((i + 1) % 16 == 0 && i + 1 < size) {
				cout << endl << "                ";
			}
		}
		cout << dec;
		break;
	}

	case REG_MULTI_SZ: {
		cout << endl;
		wchar_t* ptr = (wchar_t*)data;
		int index = 1;
		while (*ptr) {
			wcout << L"                [" << index++ << L"] " << ptr << endl;
			ptr += wcslen(ptr) + 1;
		}
		break;
	}

	default:
		cout << "(Cannot display - " << size << " bytes)";
	}
}

bool ReadRegistryValue(const string& keyPath, const string& valueName, DWORD& outType, vector<BYTE>& outData)
{
	RegistryInfo info;
	if (!ParseRegistryPath(keyPath, info))
	{
		cout << "Duong dan Registry khong hop le." << endl;
		return false;
	}

	info.valueName = wstring(valueName.begin(), valueName.end());

	// Open the registry key
	HKEY hKey;
	if (RegOpenKeyExW(info.rootKey, info.subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		cout << "[ERROR] Khong the mo Key!" << endl;
		return false;
	}

	// Query the value
	DWORD dataSize = 0;
	RegQueryValueExW(hKey, info.valueName.c_str(), nullptr, &outType, nullptr, &dataSize);
	//RegCloseKey(hKey);
	outData.resize(dataSize);
	RegQueryValueExW(hKey, info.valueName.c_str(), nullptr, &outType, outData.data(), &dataSize);
	RegCloseKey(hKey);

	// Print results
	cout << endl;
	cout << "================================================" << endl;
	cout << "  REGISTRY VALUE INFORMATION" << endl;
	cout << "================================================" << endl;
	cout << "  Key Path:     " << info.rootKeyName << "\\" << ToNarrow(info.subKey) << endl;
	cout << "  Value Name:   " << valueName << endl;
	cout << "  Type:         " << GetTypeName(outType) << endl;
	cout << "  Data:         ";
	PrintData(outType, outData.data(), dataSize);
	cout << endl;
	cout << "================================================" << endl;

	RegCloseKey(hKey);
	return true;
}

bool InputNewValue(const DWORD& TYPE, vector<BYTE>& newData)
{
	switch (TYPE)
	{
	case REG_SZ:
	case REG_EXPAND_SZ:
	{
		cout << "Nhap chuoi moi: ";
		string input;
		getline(cin, input);

		wstring wInput = wstring(input.begin(), input.end());
		size_t byteSize = (wInput.length() + 1) * sizeof(wchar_t);
		newData.resize(byteSize);
		memcpy(newData.data(), wInput.c_str(), byteSize);
		return true;
	}
	case REG_DWORD:
	{
		cout << "Nhap so moi (0 - 4294967295): ";
		string input;
		getline(cin, input);
		try
		{
			DWORD value;
			if (input.length() > 2 && input.substr(0, 2) == "0x")
			{
				value = stoul(input, nullptr, 16);
			}
			else
			{
				value = stoul(input);
			}
			return true;
		}
		catch (...)
		{
			cout << "[ERROR] Gia tri khong hop le!" << endl;
			return false;
		}
	}
	case REG_QWORD:
	{
		cout << "Nhap so moi (64-bit): ";
		string input;
		getline(cin, input);

		try
		{
			ULONGLONG value;
			if (input.length() > 2 && input.substr(0, 2) == "0x")
			{
				value = stoull(input, nullptr, 16);
			}
			else
			{
				value = stoull(input);
			}

			newData.resize(sizeof(ULONGLONG));
			memcpy(newData.data(), &value, sizeof(ULONGLONG));

			cout << "[INFO] Se ghi: " << value << endl;
			return true;
		}
		catch (...)
		{
			cout << "[ERROR] Gia tri khong hop le!" << endl;
			return false;
		}
	}
	default:
		cout << "[ERROR] Khong ho tro nhap gia tri cho kieu nay!" << endl;
		return false;
	}
}

bool WriteRegistryValue(const string& keyPath, const string& valueName, const DWORD& type, const vector<BYTE>& data)
{
	RegistryInfo info;
	if (!ParseRegistryPath(keyPath, info))
	{
		cout << "[ERROR] Duong dan Registry khong hop le." << endl;
		return false;
	}
	info.valueName = wstring(valueName.begin(), valueName.end());
	// Open the registry key
	HKEY hKey;
	LONG result = RegOpenKeyExW(info.rootKey, info.subKey.c_str(), 0, KEY_SET_VALUE, &hKey);
	if (result != ERROR_SUCCESS)
	{
		cout << "[ERROR] Khong the mo Key de ghi!. Ma loi: " << result << endl;
		return false;
	}
	// Set the value
	result = RegSetValueExW(hKey, info.valueName.c_str(), 0, type, data.data(), static_cast<DWORD>(data.size()));
	if (result != ERROR_SUCCESS)
	{
		cout << "[ERROR] Ghi gia tri that bai!. Ma loi: " << result << endl;
		RegCloseKey(hKey);
		return false;
	}
	cout << "[SUCCESS] Gia tri da duoc cap nhat!" << endl;
	RegCloseKey(hKey);
	return true;
}

void DoRead()
{
	string keyPath, valueName;
	cout << "Nhap Key Path: ";
	getline(cin, keyPath);
	cout << "Nhap Value Name: ";
	getline(cin, valueName);

	DWORD type;
	vector<BYTE> data;

	ReadRegistryValue(keyPath, valueName, type, data);
}

void DoModify()
{
	string keyPath, valueName;
	cout << "Nhap Key Path: ";
	getline(cin, keyPath);
	cout << "Nhap Value Name: ";
	getline(cin, valueName);

	DWORD type;
	vector<BYTE> data;
	if (!ReadRegistryValue(keyPath, valueName, type, data))
	{
		return;
	}

	if (type != REG_SZ && type != REG_EXPAND_SZ &&
		type != REG_DWORD && type != REG_QWORD && type != REG_BINARY) {
		cout << "\n[ERROR] Khong ho tro sua kieu " << GetTypeName(type) << endl;
		cout << "Chi ho tro: REG_SZ, REG_DWORD, REG_QWORD, REG_BINARY" << endl;
		return;
	}

	cout << "\n[NHAP GIA TRI MOI]";
	vector<BYTE> newData;

	if (!InputNewValue(type, newData))
	{
		return;
	}

	cout << "\nXac nhan thay doi?  (Y/N): ";
	char confirm;
	cin >> confirm;
	cin.ignore();

	if (toupper(confirm) != 'Y') {
		cout << "[HUY] Khong thay doi." << endl;
		return;
	}

	if (WriteRegistryValue(keyPath, valueName, type, newData))
	{
		cout << "\n[DONE] Da cap nhat gia tri!" << endl;

		// Hiển thị giá trị mới
		cout << "  Gia tri cu:   ";
		PrintData(type, data.data(), (DWORD)data.size());
		cout << endl;
		cout << "  Gia tri moi:  ";
		PrintData(type, newData.data(), (DWORD)newData.size());
		cout << endl;
	}
	else
	{
		cout << "[ERROR] Khong the cap nhat!" << endl;
	}
}

void DoDelete()
{
	return;
}

int main()
{
	int choice;
	string keyPath, valueName;
	do
	{
		cout << "\n========================================" << endl;
		cout << "      REGISTRY EDITOR TOOL" << endl;
		cout << "========================================" << endl;
		cout << "  1. Xem gia tri cua Key" << endl;
		cout << "  2. Thay doi gia tri cua Key" << endl;
		cout << "  3. Xoa gia tri cua Key" << endl;
		cout << "  0. Thoat" << endl;
		cout << "========================================" << endl;
		cout << "Chon chuc nang (0-3): ";

		cin >> choice;
		cin.ignore();

		switch (choice)
		{
		case 1: DoRead(); break;
		case 2: DoModify(); break;
		case 3: DoDelete(); break;
		case 0: cout << "Tam biet!" << endl; return 0;
		default: cout << "Lua chon khong hop le. Vui long thu lai." << endl;
		}
		system("pause");
		system("cls");
	} while (choice != 0);

	return 0;
}

