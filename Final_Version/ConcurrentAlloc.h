#pragma once
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

// 为什么需要这个函数？
// 总不能让每个线程来了之后都自己去创建一个ThreadCache吧:P
static void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)
	{
		size_t alignSize = SizeAlignMap::RoundUp(size);
		size_t kPage = alignSize >> PAGE_SHIFT;

		PageCache::GetInstance()->_pageMtx.lock();
		Span* span = PageCache::GetInstance()->NewSpan(kPage);
		span->_objSize = size;
		PageCache::GetInstance()->_pageMtx.unlock();

		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		if (pTLSThreadCache == nullptr)
		{
			static ObjectPool<ThreadCache> tcPool;
			pTLSThreadCache = tcPool.New();
		}

#ifdef DEBUG
		cout << std::this_thread::get_id() << " " << pTLSThreadCache << endl;
#endif

		return pTLSThreadCache->Allocate(size);
	}
}

static void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstance()->MapObjToSpan(ptr); // 这里只能在if外面，不然会造成死锁
	size_t size = span->_objSize;

	if (size > MAX_BYTES)
	{

		PageCache::GetInstance()->_pageMtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_pageMtx.unlock();
	}
	else
	{
		assert(pTLSThreadCache && ptr);
		pTLSThreadCache->Deallocate(ptr, size);
	}
}