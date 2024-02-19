#pragma once
#include "Common.h"

// 单例
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInit;
	}

	// 从Central Cache获取一定数量的对象给Thread Cache
	// start, end 输出型参数，带回地址
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t alignSize);

	// 获取一个非空的span
	Span* GetOneSpan(SpanList& list, size_t size);

	// 将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start, size_t alignSize);
private:
	CentralCache()
	{}

	CentralCache(const CentralCache&) = delete;
private:
	SpanList _spanLists[NFREELIST];
	static CentralCache _sInit;
};