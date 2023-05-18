#include <Windows.h>
#include <stdio.h>

#define COUNT_THREADS 4
#define MEMORY_SIZE 10

typedef struct Node
{
	void* pointer;
	unsigned int size;
}Node;

CRITICAL_SECTION section;

byte memory_space[MEMORY_SIZE];
HANDLE hArray[COUNT_THREADS];
Node memory_map[COUNT_THREADS];
DWORD hIndex = 0;

void* allocate_memory(unsigned int requer_size)
{
	EnterCriticalSection(&section);
	void* pointer = (void*)memory_space;

	if (memory_map[COUNT_THREADS - 1].pointer != NULL)
	{
		LeaveCriticalSection(&section);
		return NULL;
	}

	unsigned int mIndex;

	for (mIndex = 0; mIndex < COUNT_THREADS; ++mIndex)
	{
		if (memory_map[mIndex].pointer == NULL)
		{
			if ((unsigned int)memory_space + MEMORY_SIZE - (unsigned int)pointer >= requer_size)
				break;
			LeaveCriticalSection(&section);
			return NULL;
		}
		else if ((unsigned int)memory_map[mIndex].pointer - (unsigned int)pointer >= requer_size)
		{
			for (unsigned int j = COUNT_THREADS - 1, k = COUNT_THREADS - 2; j > mIndex; --j, --k)
			{
				memory_map[j].pointer = memory_map[k].pointer;
				memory_map[j].size = memory_map[k].size;
			}
			break;
		}
		(byte*)pointer = (byte*)(memory_map[mIndex].pointer) + memory_map[mIndex].size;
	}
	memory_map[mIndex].pointer = pointer;
	memory_map[mIndex].size = requer_size;
	LeaveCriticalSection(&section);
	return pointer;
}

void free_memory(void* pointer)
{
	EnterCriticalSection(&section);
	if (pointer)
	{
		for (unsigned int i = 0; i < COUNT_THREADS; ++i)
		{
			if (memory_map[i].pointer == pointer)
			{
				unsigned int lastIndes = i;
				for (unsigned int j = i + 1; j < COUNT_THREADS; ++lastIndes, ++j)
					memory_map[lastIndes] = memory_map[j];
				memory_map[lastIndes].pointer = NULL;
				memory_map[lastIndes].size = 0;
			}
		}
	}
	LeaveCriticalSection(&section);
}

DWORD WINAPI thread_fonction(PVOID param)
{
	srand(time(NULL));
	void* pointer = NULL;
	unsigned int require_memory = (rand() % 5) + 2;
	while (pointer == NULL)
	{
		pointer = allocate_memory(require_memory);
		Sleep(1000);
	}

	wprintf(L"Pointer %p alloc %d \n", pointer, require_memory);

	for (byte* index = (byte*)pointer; index < (byte*)pointer + require_memory; ++index)
		*index = (byte)require_memory;
	Sleep(((rand() % 3) + 1) * 1000);
	for (byte* index = (byte*)pointer; index < (byte*)pointer + require_memory; ++index)
		*index = 0;

	free_memory(pointer);
	wprintf(L"Pointer %p free\n", pointer);
	ExitThread(0);
}

void add_thread1()
{
	if (hIndex == COUNT_THREADS)
	{
		DWORD endThread;
		endThread = WaitForMultipleObjects(hIndex, hArray, FALSE, INFINITE);
		CloseHandle(hArray[endThread]);
		wprintf(L"Thread %d %d stop\n", (int)hArray[endThread], endThread);
		DWORD lastIndex = COUNT_THREADS - 1;
		for (lastIndex = COUNT_THREADS - 1; lastIndex > endThread; --lastIndex)
		{
			if (hArray[lastIndex] != NULL)
			{
				hArray[endThread] = hArray[lastIndex];
				break;
			}
		}
		hArray[lastIndex] = NULL;
		--hIndex;
	}
	hArray[hIndex] = CreateThread(NULL, 0, thread_fonction, hIndex, 0, 0);
	wprintf(L"Thread %d %d start\n", (int)hArray[hIndex], hIndex);
	++hIndex;
}

int main(int argc, char* args[])
{
	srand(time(NULL));
	for (int i = 0; i < COUNT_THREADS; ++i)
	{
		hArray[i] = NULL;
		memory_map[i].pointer = NULL;
		memory_map[i].size = 0;
	}

	printf("%p\n\n", memory_space);
	void* pointer;
	InitializeCriticalSection(&section);
	for (int i = 0; i < 10; ++i)
	{
		Sleep(((rand() % 2) + 1) * 1000);
		add_thread1();
	}
	WaitForMultipleObjects(hIndex, hArray, TRUE, INFINITE);
	DeleteCriticalSection(&section);
	for (DWORD i = 0; i < COUNT_THREADS; ++i)
	{
		if (hArray[i] != NULL)
			CloseHandle(hArray[i]);
	}
	return 0;
}
