#pragma once
#include "Common.h"


class ThreadCache
{
public:
	// 申请和释放内存/对象
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	// 从CentralCache获取内存/对象
	void* FetchFromCentralCache(size_t index, size_t alignSize);

	// 释放对象时，链表过长，回收内存回到Central Cache
	void ListTooLong(FreeList& list, size_t size);

private:
	FreeList _freeLists[NFREELIST];
};

// 实现无锁访问 -- TLS
// 通过TLS，每个线程无锁地获取自己专属的ThreadCache对象
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;