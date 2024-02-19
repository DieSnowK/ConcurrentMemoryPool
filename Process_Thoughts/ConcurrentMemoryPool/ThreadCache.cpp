#include "ThreadCache.h"
#include "CentralCache.h"

// 调用构造和析构？// TODO

void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);
	size_t alignSize = SizeAlignMap::RoundUp(size);
	size_t index = SizeAlignMap::Index(size);

	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		// 从CentralCache获取内存
		return FetchFromCentralCache(index, alignSize);
	}
}

// 后续需要处理size，free是不需要size的
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(size <= MAX_BYTES);
	assert(ptr);
	
	// 找出映射的自由链表桶，头插进去
	size_t index = SizeAlignMap::Index(size);
	_freeLists[index].Push(ptr);

	// 当链表长度大于一次批量申请的内存时，就开始还回一段给Central Cache
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize)
{
	// 慢开始反馈调节算法
	// 1.最开始不会一次性向Central Cache要太多，因为可能要太多用不完
	// 2.如果不断有alignSize大小内存的需求，那么batchNum就会不断增长，直到上限
	// 3.alignSize越大，一次性向Central Cache要的batchNum就越小
	// 4.alignSize越小，一次性向Central Cache要的batchNum就越大	
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeAlignMap::MoveObjNum(alignSize));
	if (batchNum == _freeLists[index].MaxSize())
	{
		_freeLists[index].MaxSize() += 2;
	}

	void* start = nullptr;
	void* end = nullptr;

	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, alignSize);
	assert(actualNum > 0);
	
	if (actualNum == 1)
	{
		assert(start == end);
		return start;
	}
	else
	{
		// 返回头上的一个，剩下的挂在FreeList下
		_freeLists[index].PushRange(NextObj(start), end, actualNum - 1);
		return start;
	}
}

void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;

	list.PopRange(start, end, list.MaxSize());

	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}

