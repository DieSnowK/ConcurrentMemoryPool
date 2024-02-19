// 说明
// 使用定长内存池替代malloc，否则CMP替代malloc，但是里面又调用malloc，死循环了:P
#pragma once
#include <iostream>
#include "Common.h"

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