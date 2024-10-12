#pragma once
#include "skse64_common/BranchTrampoline.h"
#include "skse64_common/SafeWrite.h"

template<typename T>
T WriteVtbl(uintptr_t addr, uintptr_t dst)
{
	T oldTgt = *(T*)addr;
	SafeWrite64(addr, dst);
	return oldTgt;
}

uintptr_t Write5BranchEx(uintptr_t src, uintptr_t dst)
{
	int32_t* p = (int32_t*)(src + 1);
	uintptr_t oldTgt = src + 5 + *p;
	return g_branchTrampoline.Write5Branch(src, dst) ? oldTgt : 0;
}

uintptr_t Write5CallEx(uintptr_t src, uintptr_t dst)
{
	int32_t* p = (int32_t*)(src + 1);
	uintptr_t oldTgt = src + 5 + *p;
	return g_branchTrampoline.Write5Call(src, dst) ? oldTgt : 0;
}

uintptr_t Write6BranchEx(uintptr_t src, uintptr_t dst)
{
	int32_t* p = (int32_t*)(src + 2);
	uintptr_t oldTgt = src + 6 + *p;
	return g_branchTrampoline.Write6Branch(src, dst) ? oldTgt : 0;
}

uintptr_t Write6CallEx(uintptr_t src, uintptr_t dst)
{
	int32_t* p = (int32_t*)(src + 2);
	uintptr_t oldTgt = src + 6 + *p;
	return g_branchTrampoline.Write6Call(src, dst) ? oldTgt : 0;
}
