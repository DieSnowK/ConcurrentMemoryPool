#pragma once
#include <iostream>
#include <thread>
#include <cassert>
using std::cout;
using std::endl;

//#define DEBUG

static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREELIST = 208; // 哈希桶的个数

// static使NextObj()只在当前.cpp文件中可见，因为多个文件都会包含Common.h
static inline void*& NextObj(void* obj)
{
	return *(void**)obj;
}

// 管理切分好的小对象的自由链表
class FreeList
{
public:
	void Push(void* obj)
	{
		assert(obj);

		// 头插
		NextObj(obj) = _freeList; //*(void**)obj = _freeList;
		_freeList = obj;
	}

	void* Pop()
	{
		assert(_freeList);

		// 头删
		void* obj = _freeList;
		_freeList = NextObj(obj);

		return obj;
	}

	bool Empty()
	{
		return _freeList == nullptr;
	}
private:
	void* _freeList = nullptr;

};

// 思考为什么不都按8Bytes对齐？
// 需要32768个8Bytes对齐的自由链表，浪费内存

// 整体控制在最多10%左右的内碎片浪费
// [1,128]					8byte对齐	    freelist[0,16)
// [128+1,1024]				16byte对齐	    freelist[16,72)
// [1024+1,8*1024]			128byte对齐	    freelist[72,128)
// [8*1024+1,64*1024]		1024byte对齐     freelist[128,184)
// [64*1024+1,256*1024]		8*1024byte对齐   freelist[184,208)

// 计算对象大小的对齐映射规则
class SizeAlignMap
{
public:
	// v1.0
	//static inline size_t _RoundUp(size_t size, size_t alignNum)
	//{
	//	size_t alignSize = size;
	//	if (size % 8 != 0)
	//	{
	//		alignSize = (size / alignNum + 1) * alignNum; // 向上对齐，所以+1
	//	}
	//	else
	//	{
	//		alignSize = size;
	//	}

	// v2.0
	static inline size_t _RoundUp(size_t size, size_t alignNum)
	{
		return (size + alignNum - 1) & ~(alignNum - 1);
	}


	// 计算向上对齐是多少
	static inline size_t RoundUp(size_t size) 
	{
		assert(size <= MAX_BYTES);

		if (size <= 128)
		{
			return _RoundUp(size, 8);
		}
		else if (size <= 1024)
		{
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024)
		{
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024)
		{
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024)
		{
			return _RoundUp(size, 8 * 1024);
		}
		else
		{
			// 按理来说不会走到这里
			assert(false);
			return -1;
		}
	}

	// v1.0
	//static inline size_t _Index(size_t size, size_t alignNum)
	//{
	//	if (size % alignNum == 0)
	//	{
	//		return size / alignNum - 1;
	//	}
	//	else
	//	{
	//		return size / alignNum;
	//	}
	//}

	static inline size_t _Index(size_t size, size_t align_shift)
	{
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	// 计算映射的哪一个自由链表桶
	static inline size_t Index(size_t size)
	{
		assert(size <= MAX_BYTES);
		static const size_t FrontNum[4] = {16, 56, 56, 56};

		if (size <= 128)
		{
			return _Index(size, 3);
		}
		else if (size <= 1024)
		{
			return _Index(size, 4) + FrontNum[0];
		}
		else if (size <= 8 * 1024)
		{
			return _Index(size, 7) + FrontNum[0] + FrontNum[1];
		}
		else if (size <= 64 * 1024)
		{
			return _Index(size, 10) + FrontNum[0] + FrontNum[1] + FrontNum[2];
		}
		else if (size <= 256 * 1024)
		{
			return _Index(size, 13) + FrontNum[0] + FrontNum[1] + FrontNum[2] + FrontNum[3];
		}
		else
		{
			// 按理来说不会走到这里
			assert(false);
			return -1;
		}
	}
private:
	
};