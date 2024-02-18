#pragma once
#include "Common.h"

// 单例
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInit;
	}

	// 获取一个k页的span
	Span* NewSpan(size_t k);
private:
	PageCache()
	{}

	PageCache(const PageCache&) = delete;
private:
	SpanList _spanList[NPAGES];
	static PageCache _sInit;
public:
	// 需要加一把大锁，不能使用桶锁
	// 因为可能两个线程同时获得了一个span，然后切分
	// 此时桶锁插入会有线程安全问题
	// 并且此时桶锁反而会影响效率
	std::mutex _pageMtx;
};