#include <iostream>
#include <cstring>

struct TestStruct
{
	char buffer[8];
	int value;
};


void UnsafeCopy(char* dst, const char* src)
{
	while (*src != '\0')
	{
		*dst = *src;
		dst++;
		src++;
	}
	*dst = '\0';
}


void SafeCopy(char* dst, size_t dstSize, const char* src)
{
	if (dstSize == 0) return;

	size_t count = 0;
	while (*src != '\0' && count < dstSize - 1)
	{
		*dst = *src;
		dst++;
		src++;
		count++;
	}
	*dst = '\0';
}


int main()
{
	const char* longString = "12345678";
	
	// Unsafe copy
	TestStruct unsafeTest;
	unsafeTest.value = 0x11223344;

	printf("Address buffer: %p\n", (void*)unsafeTest.buffer);
	printf("Address value:  %p\n", (void*)&unsafeTest.value);
	printf("Original value: 0x%X\n", unsafeTest.value);

	std::cout << "Unsafe Copy:" << std::endl;
	UnsafeCopy(unsafeTest.buffer, longString);
	printf("Buffer content: %s\n", unsafeTest.buffer);
	printf("New Value:      0x%X  <-- BI GHI DE (BUFFER OVERFLOW)!\n\n", unsafeTest.value);

	// Safe copy
	TestStruct safeTest;
	safeTest.value = 0x11223344;

	printf("Address buffer: %p\n", (void*)safeTest.buffer);
	printf("Address value:  %p\n", (void*)&safeTest.value);
	printf("Original value: 0x%X\n", safeTest.value);

	std::cout << "Safe Copy:" << std::endl;
	SafeCopy(safeTest.buffer, sizeof(safeTest.buffer), longString);
	printf("Buffer content: %s\n", safeTest.buffer);
	printf("New Value:      0x%X  <-- KO BI GHI DE !\n\n", safeTest.value);

	system("pause");
	return 0;
}