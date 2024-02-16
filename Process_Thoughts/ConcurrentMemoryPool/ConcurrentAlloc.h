#pragma once
#include "ThreadCache.h"

// 为什么需要这个函数？
// 总不能让每个线程来了之后都自己去创建一个ThreadCache吧:P
static void* ConcurrentAlloc(size_t size)
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}

#ifdef DEBUG
	cout << std::this_thread::get_id() << " " << pTLSThreadCache << endl;
#endif

	return pTLSThreadCache->Allocate(size);
}

static void ConcurrentFree(void* ptr, size_t size)
{
	assert(pTLSThreadCache && ptr);

	pTLSThreadCache->Deallocate(ptr, size);
}