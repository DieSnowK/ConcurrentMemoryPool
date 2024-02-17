#include "CentralCache.h"

CentralCache CentralCache::_sInit;

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t alignSize)
{
	size_t index = SizeAlignMap::Index(alignSize);
	_spanLists[index]._mtx.lock();

	Span* span = GetOneSpan(_spanLists[index], alignSize);
	assert(span);
	assert(span->_freeList);

	// 从span中获取batchNum个对象
	// 如果不够batchNum个，有多少拿多少
	start = end = span->_freeList;
	size_t actualNum = 1;
	for (size_t i = 0; i < batchNum - 1 && NextObj(end); i++)
	{
		end = NextObj(end);
		actualNum++;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	_spanLists[index]._mtx.unlock();

	return actualNum;
}

Span* CentralCache::GetOneSpan(SpanList& list, size_t alignSize)
{
	return nullptr;
}
