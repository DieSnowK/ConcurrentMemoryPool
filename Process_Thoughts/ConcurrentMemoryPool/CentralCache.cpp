#include "CentralCache.h"
#include "PageCache.h"

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
	span->_useCount += actualNum;

	_spanLists[index]._mtx.unlock();

	return actualNum;
}

// 先从SpanList中遍历寻找符合要求的Span，若没有，则从PageCache获取
Span* CentralCache::GetOneSpan(SpanList& list, size_t alignSize)
{
	// 查看当前的spanList中是否有还未分配对象的span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freeList)
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	// 先把Central Cache的桶锁解掉，如果其他线程释放内存对象回来，不会阻塞
	list._mtx.unlock();

	// 至此，说明没有空闲的span了，需要从PageCache获取
	// NewSpan中递归锁的一种解决方案|
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeAlignMap::MovePageNum(alignSize)); // 这里出问题了
	span->_isUse = true;
	span->_objSize = alignSize;
	PageCache::GetInstance()->_pageMtx.unlock();
	// 不需要立即续上Central Cache的桶锁,因为k页span只有当前线程能拿到，其他线程拿不到

	// 获取span之后，需要切分span，不需要加锁
	// 计算span的大块内存起始地址和大块内存的大小(字节数)
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes; 

	// 把大块内存切成自由链表链接起来
	// 尾插比较好，物理上是连续的，连续的CPU缓存利用率比较高
	// 1.先切一块下来去做头，方便尾插
	span->_freeList = start;
	start += alignSize;
	void* tail = span->_freeList;

	while (start < end)
	{
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += alignSize;
	}

	NextObj(tail) = nullptr; // 将tail指向nullptr，查出bug了√

	// 切好span以后，需要把span挂到桶里面去的时候，再加锁
	list._mtx.lock();
	// 大部分情况下，新的span拿完都还有剩，所以将span入SpanList
	list.PushFront(span);											

	return span;
}

void CentralCache::ReleaseListToSpans(void* start, size_t alignSize)
{
	size_t index = SizeAlignMap::Index(alignSize);

	_spanLists[index]._mtx.lock();

	while (start)
	{
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->MapObjToSpan(start);
		NextObj(start) = span->_freeList;
		span->_freeList = start;
		start = next;

		span->_useCount--;

		// span切分出去的小块内存都回来了
		// 这个span可以回收给PageCache了，PageCache可以尝试去做前后页的合并
		if (span->_useCount == 0) 
		{
			_spanLists[index].Erase(span);
			span->_freeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;

			// 这里已经去操作PageCache了，可以先解桶锁，以便线程归还内存/申请内存
			_spanLists[index]._mtx.unlock();

			// 操作PageCache，外面加锁比里面相对好控制一些
			PageCache::GetInstance()->_pageMtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			PageCache::GetInstance()->_pageMtx.unlock();

			_spanLists[index]._mtx.lock();
		}
	}

	_spanLists[index]._mtx.unlock();
}

