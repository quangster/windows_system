#include <cstdio>
#include <cstdlib>
#include <cstdint>


void func_static()
{
	static int s = 0;
	s++;
	printf("Static Variable:     %p, Value: %d\n", (void*)&s, s);
}


struct MemoryBlock {
	uintptr_t addr;
	const char* name;
	const char* segment;
};


int g_init = 100;
int g_uninit;


int main()
{
	int* heap_var_malloc = (int*)malloc(sizeof(int));
	int* heap_var_new = new int(10);

	int local_var_1 = 1;
	int local_var_2 = 2;

	static int static_in_main = 999;

	// Text Segment
	printf("[TEXT Segment]\n");
	printf("Address of main():        %p\n", (void*)&main);
	printf("Address of func_static(): %p\n", (void*)&func_static);
	long text_diff = (uintptr_t)&func_static - (uintptr_t)&main;
	printf("Distance (main() -> func_static()) %ld bytes\n\n", text_diff);


	// Data & BSS Segment
	printf("[DATA / BSS Segment]\n");
	printf("Global Init:         %p\n", (void*)&g_init);
	printf("Static in main:      %p\n", (void*)&static_in_main);
	printf("Global Uninit:       %p\n", (void*)&g_uninit);
	for (int i = 0; i < 5; i++)
	{
		func_static();
	}
	long data_bss_diff = (uintptr_t)&g_uninit - (uintptr_t)&g_init;
	long data_static_diff = (uintptr_t)&static_in_main - (uintptr_t)&g_init;
	printf("Distance (g_init -> g_uninit): %ld bytes\n", data_bss_diff);
	printf("Distance (g_init -> static_in_main): %ld bytes\n\n", data_static_diff);


	// Heap
	printf("[HEAP Segment]\n");
	printf("Malloc Address:      %p\n", (void*)heap_var_malloc);
	printf("New Address:         %p\n", (void*)heap_var_new);
	long heap_diff = (uintptr_t)heap_var_new - (uintptr_t)heap_var_malloc;
	printf("Distance (malloc -> new): %ld bytes\n\n", heap_diff);


	// Stack
	printf("[STACK Segment]\n");
	printf("Local Var 1:         %p\n", (void*)&local_var_1);
	printf("Local Var 2:         %p\n", (void*)&local_var_2);
	long stack_diff = (long)((uintptr_t)&local_var_2 - (uintptr_t)&local_var_1);
	printf("Distance (local_var_1 -> local_var_2): %ld bytes\n", stack_diff);

	// Compare Segments
	MemoryBlock blocks[] = {
		{ (uintptr_t)&main,            "main()",           "TEXT"  },
		{ (uintptr_t)&g_init,          "g_init",           "DATA"  },
		{ (uintptr_t)&g_uninit,        "g_uninit",         "BSS"   },
		{ (uintptr_t)heap_var_malloc,  "heap_var_malloc",  "HEAP"  },
		{ (uintptr_t)&local_var_1,     "local_var_1",      "STACK" }
	};

	// sort addresses
	int n = 5;
	for (int i = 0; i < n - 1; i++) {
		for (int j = 0; j < n - i - 1; j++) {
			if (blocks[j].addr > blocks[j + 1].addr) {
				MemoryBlock temp = blocks[j];
				blocks[j] = blocks[j + 1];
				blocks[j + 1] = temp;
			}
		}
	}

	printf("\n=== MEMORY LAYOUT (Low to High) ===\n\n");
	printf("%-5s %-20s %-18s %-10s %s\n", "No.", "Variable", "Address", "Segment", "Distance to prev");
	printf("--------------------------------------------------------------------------------\n");

	for (int i = 0; i < n; i++) {
		printf("%-5d %-20s %p   %-10s",
			i + 1,
			blocks[i].name,
			(void*)blocks[i].addr,
			blocks[i].segment);

		if (i > 0) {
			unsigned long long diff = blocks[i].addr - blocks[i - 1].addr;

			if (diff > 1024ULL * 1024 * 1024) {
				printf("   (+%.2f GB)", diff / (1024.0 * 1024.0 * 1024.0));
			}
			else if (diff > 1024ULL * 1024) {
				printf("   (+%.2f MB)", diff / (1024.0 * 1024.0));
			}
			else if (diff > 1024) {
				printf("   (+%.2f KB)", diff / 1024.0);
			}
			else {
				printf("   (+%llu bytes)", diff);
			}
		}
		printf("\n");
	}


	// Clean Dynamic Memory
	free(heap_var_malloc);
	delete heap_var_new;

	system("pause");
	return 0;
}