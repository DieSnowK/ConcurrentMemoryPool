#include "ThreadCache.h"

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
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize)
{
	return nullptr;
}
