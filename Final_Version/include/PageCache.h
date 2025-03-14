#pragma once
#include "Common.h"
#include "ObjectPool.h"
#include "PageMap.h"

// TODO x64 尚未支持
#ifdef _WIN64
	typedef TCMalloc_PageMap3<64 - PAGE_SHIFT> PageMAP;
#elif _WIN32
	typedef TCMalloc_PageMap2<32 - PAGE_SHIFT> PageMAP;
#else // Linux
	#ifdef __x86_64__
		typedef TCMalloc_PageMap3<64 - PAGE_SHIFT> PageMAP;
	#else
		typedef TCMalloc_PageMap2<32 - PAGE_SHIFT> PageMAP;
	#endif
#endif

// 单例
class PageCache
{
public:
	static PageCache *GetInstance()
	{
		return &_sInit;
	}

	// 获取一个k页的span
	Span *NewSpan(size_t k);

	// 获取PAGE_ID到Span*的映射
	Span *MapObjToSpan(void *obj);

	// 释放空闲span回到PageCache，并合并相邻的span
	void ReleaseSpanToPageCache(Span *span);

private:
	PageCache()
	{}

	PageCache(const PageCache &) = delete;

private:
	SpanList _spanList[NPAGES];
	ObjectPool<Span> _spanPool;
	PageMAP _idSpanMap;

	static PageCache _sInit;

public:
	// 需要加一把大锁，不能使用桶锁
	// 因为可能两个线程同时获得了一个span，然后切分
	// 此时桶锁插入会有线程安全问题
	// 并且此时桶锁反而会影响效率
	std::mutex _pageMtx;
};