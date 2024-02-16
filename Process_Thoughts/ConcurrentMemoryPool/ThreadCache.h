#pragma once
#include "Common.h"


class ThreadCache
{
public:
	// ������ͷ��ڴ�/����
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);

	// ��CentralCache��ȡ�ڴ�/����
	void* FetchFromCentralCache(size_t index, size_t alignSize);

private:
	FreeList _freeLists[NFREELIST];
};

// ʵ���������� -- TLS
// ͨ��TLS��ÿ���߳������ػ�ȡ�Լ�ר����ThreadCache����
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;