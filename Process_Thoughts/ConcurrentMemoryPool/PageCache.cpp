#include "PageCache.h"

PageCache PageCache::_sInit;

Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0 && k < NPAGES);

	// 先检查第k个桶有没有span
	if (!_spanList[k].Empty())
	{
		return _spanList[k].PopFront();
	}
	
	// 检查后面的桶有没有span，如果有，则可以切分
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanList[i].Empty())
		{
			// 切分成一个k页的span和一个n-k页的span
			// k页的span返回给Central Cache，n-k页的span挂到n-k号桶上去
			Span* nSpan = _spanList[i].PopFront();
			Span* kSpan = new Span;
			
			// 在nSpan的头部切一个k页下来
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;
			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanList[nSpan->_n].PushFront(nSpan);

			return kSpan;
		}
	}

	// 走到这里Page Cache从没有符合要求的Span，则需要向堆要内存，且直接要一个最大的Page
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigSpan->_n = NPAGES - 1;

	// 这里操作神之一笔，复用前面的代码
	_spanList[bigSpan->_n].PushFront(bigSpan);
	return NewSpan(k);
}
