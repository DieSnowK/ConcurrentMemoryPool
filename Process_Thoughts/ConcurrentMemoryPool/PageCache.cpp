#include "PageCache.h"

PageCache PageCache::_sInit;

Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0);

	// 大于128Page直接向堆申请
	if (k > NPAGES - 1)
	{
		void* ptr = SystemAlloc(k);
		//Span* span = new Span;
		Span* span = _spanPool.New();
		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;

		_idSpanMap[span->_pageId] = span;

		return span;
	}

	// 先检查第k个桶有没有span
	if (!_spanList[k].Empty())
	{
		Span* kSpan = _spanList[k].PopFront();

		// 此处也要映射id和span，查出bug了√
		for (PAGE_ID i = 0; i < kSpan->_n; i++)
		{
			_idSpanMap[kSpan->_pageId + i] = kSpan;
		}

		return kSpan;
	}
	
	// 检查后面的桶有没有span，如果有，则可以切分
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanList[i].Empty())
		{
			// 切分成一个k页的span和一个n-k页的span
			// k页的span返回给Central Cache，n-k页的span挂到n-k号桶上去
			Span* nSpan = _spanList[i].PopFront();
			//Span* kSpan = new Span;
			Span* kSpan = _spanPool.New();
			
			// 在nSpan的头部切一个k页下来
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;
			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanList[nSpan->_n].PushFront(nSpan);
			// 存储nSpan的首尾页号跟nSpan的映射，方便Page Cache回收内存时进行的合并查找
			// TIPS：存两个时为了方便向前&向后:P
			_idSpanMap[nSpan->_pageId] = nSpan;
			_idSpanMap[nSpan->_pageId + nSpan->_n - 1] = nSpan;

			// 建立id和span的映射，方便Central Cache回收小块内存时，查找对应的span
			for (PAGE_ID i = 0; i < kSpan->_n; i++)
			{
				_idSpanMap[kSpan->_pageId + i] = kSpan;
			}

			return kSpan;
		}
	}

	// 走到这里Page Cache从没有符合要求的Span，则需要向堆要内存，且直接要一个最大的Page
	//Span* bigSpan = new Span;
	Span* bigSpan = _spanPool.New();
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigSpan->_n = NPAGES - 1;

	// 这里操作神之一笔，复用前面的代码
	_spanList[bigSpan->_n].PushFront(bigSpan);
	return NewSpan(k);
}

Span* PageCache::MapObjToSpan(void* obj)
{
	assert(obj);
	
	std::unique_lock<std::mutex> lock(_pageMtx); // RAII锁，出了作用域自动解锁

	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
	auto ret = _idSpanMap.find(id);
	if (ret != _idSpanMap.end())
	{
		return ret->second;
	}
	else
	{
		// 理应不会走到这，因为obj和PAGE_ID应该是映射上的
		assert(false);
		return nullptr;
	}
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{
	// 大于128Page的直接还给堆
	if (span->_n > NPAGES - 1)
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		//delete span;
		_spanPool.Delete(span);

		return;
	}

	// 对span前后的页，尝试进行合并，缓解内存碎片问题
	// 这里添加字段_isUse判断，不可用_useCount判断，会有线程安全问题
	while (1) // 向前
	{
		PAGE_ID prevId = span->_pageId - 1;
		auto ret = _idSpanMap.find(prevId);

		// 前面的页号没有，不进行合并
		if (ret == _idSpanMap.end())
		{
			break;
		}

		Span* prevSpan = ret->second;

		// 前面相邻页的span在使用，不进行合并
		if (prevSpan->_isUse == true)
		{
			break;
		}

		// 合并出超过128页的span没办法管理
		if (prevSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		//delete prevSpan;
		_spanPool.Delete(prevSpan);
	}

	while (1) // 向前
	{
		PAGE_ID nextId = span->_pageId + span->_n;
		auto ret = _idSpanMap.find(nextId);

		// 前面的页号没有，不进行合并
		if (ret == _idSpanMap.end())
		{
			break;
		}

		Span* nextSpan = ret->second;

		// 前面相邻页的span在使用，不进行合并
		if (nextSpan->_isUse == true)
		{
			break;
		}

		// 合并出超过128页的span没办法管理
		if (nextSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		span->_n += nextSpan->_n;

		_spanList[nextSpan->_n].Erase(nextSpan);
		//delete nextSpan;
		_spanPool.Delete(nextSpan);
	}

	_spanList[span->_n].PushFront(span);
	span->_isUse = false;
	_idSpanMap[span->_pageId] = span;
	_idSpanMap[span->_pageId + span->_n - 1] = span;
}
