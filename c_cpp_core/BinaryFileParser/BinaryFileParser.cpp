#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

#pragma pack(push, 1)
struct FILE_HDR
{
	char magic[4];
	int version;
	int dataSize;
};
#pragma pack(pop)


void WriteBinaryFile(const char* filename, const char* payload)
{
	std::ofstream file(filename, std::ios::binary);
	if (!file)
	{
		std::cerr << "Error creating file!" << std::endl;
		return;
	}

	// Prepare header
	FILE_HDR header;
	memcpy(header.magic, "DATA", 4);
	header.version = 1;
	size_t len = strlen(payload);
	header.dataSize = (int)(len + 1); // include null terminator

	// Write header
	file.write(reinterpret_cast<const char*>(&header), sizeof(header));

	// Write data
	file.write(payload, header.dataSize);

	file.close();
	std::cout << "[Write] Success: " << filename
		<< " | Payload: \"" << payload << "\" (" << header.dataSize << " bytes)" << std::endl;
}


void ReadBinaryFile(const char* filename)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file)
	{
		std::cerr << "[Read] Error opening file!" << std::endl;
		return;
	}

	// Read header
	FILE_HDR header;
	file.read(reinterpret_cast<char*>(&header), sizeof(header));

	if (file.gcount() != sizeof(header)) {
		std::cerr << "[Read] Error file too short for header!" << std::endl;
		return;
	}

	// Validate header by magic
	if (strncmp(header.magic, "DATA", 4) != 0)
	{
		std::cerr << "[Read] Invalid file format!" << std::endl;
		return;
	}

	std::cout << "\n[Read] File Header Valid!\n";
	std::cout << " - Magic:    " << std::string(header.magic, sizeof(header.magic)) << std::endl;
	std::cout << " - Version:  " << header.version << std::endl;
	std::cout << " - DataSize: " << header.dataSize << " bytes" << std::endl;

	// Read data
	if (header.dataSize > 0)
	{
		std::vector<char> data(header.dataSize);
		file.read(data.data(), header.dataSize);
		std::cout << "[Read] Content: " << std::string(data.data(), header.dataSize) << std::endl;
		
	}
	file.close();
}


int main()
{
	WriteBinaryFile("test.bin", "This is test.bin");
	ReadBinaryFile("test.bin");
	std::cout << std::endl;
	WriteBinaryFile("long.bin", "This is long.binnnnnnnnnnn");
	ReadBinaryFile("long.bin");
	system("pause");
	return 0;
}