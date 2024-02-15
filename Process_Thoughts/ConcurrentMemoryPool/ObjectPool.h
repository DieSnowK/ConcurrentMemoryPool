#pragma once
#include <iostream>

#ifdef _WIN32
	#include <Windows.h>
#elif
	// Linux下的头文件
#endif

// 直接去堆上按页申请空间，彻底摆脱malloc(主要)，并非为了提高效率
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// Linux下brk mmap等
#endif

	if (ptr == nullptr)
	{
		throw std::bad_alloc();
	}

	return ptr;
}

// 定长内存池v1.0
//template<size_t N>
//class ObjectPool
//{};

// 因为每个对象大小是固定的，所以也可以这样设计
template<class T>
class ObjectPool
{
public:
	T* New()
	{
		T* obj = nullptr;
		// 优先用已经还回来的内存块
		if (_freeList)
		{
			obj = (T*)_freeList;
			_freeList = *(void**)_freeList;
		}
		else
		{
			// 剩余字节数少于一个对象大小，则重新申请大空间
			if (_remainBytes < sizeof(T))
			{
				_remainBytes = 128 * 1024;
				_memory = (char*)SystemAlloc(_remainBytes >> 13);
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}

			obj = (T*)_memory;

			// 可能T对象大小小于一个指针，此时若只使用一个T的大小，则存不下一个地址
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T); 
			_memory += sizeof(T);
			_remainBytes -= sizeof(T);
		}

		// 定位new，显式调用T的构造函数初始化
		// new (address) type
		new(obj)T;

		return obj;
	}

	void Delete(T* obj)
	{
		// 显式调用析构函数清理对象
		obj->~T();

		// 头插
		//*(int*)obj = _freeList; // 这里不能这样用，因为x32和x64情况下，指针大小不同
		*(void**)obj = _freeList;
		_freeList = obj;

	}
private:
	char* _memory = nullptr; // 指向大块内存的指针
	void* _freeList = nullptr; // 还回来的内存的自由链表的头指针
	size_t _remainBytes = 0; // 大块内存在切分过程中剩余字节数，用于防止越界
};