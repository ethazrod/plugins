#include <SKSE.h>
#include <SKSE/PluginAPI.h>
#include <SKSE/DebugLog.h>
#include <SKSE/GameRTTI.h>
#include <SKSE/HookUtil.h>
#include <SKSE/GameData.h>
#include <SKSE/GameForms.h>
#include <SKSE/GameReferences.h>

#include "iniSettings.h"
#include "MiniMapMenu.h"

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

class FreezeEventHandler : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	typedef EventResult(FreezeEventHandler::*FnReceiveEvent)(MenuOpenCloseEvent *evn, BSTEventSource<MenuOpenCloseEvent> *src);

	static BSTHashMap<UInt32, FnReceiveEvent> ms_handlers;

	UInt32 GetVPtr() const
	{
		return *(UInt32*)this;
	}

	EventResult ReceiveEvent_Hook(MenuOpenCloseEvent *evn, BSTEventSource<MenuOpenCloseEvent> *src)
	{
		static BSFixedString menuName = "MiniMapMenu";

		if (evn->menuName == menuName)
		{
			return kEvent_Continue;
		}

		FnReceiveEvent fn;
		return (ms_handlers.GetAt(GetVPtr(), fn)) ? (this->*fn)(evn, src) : kEvent_Continue;
	}

	void InstallHook()
	{
		UInt32 vptr = GetVPtr();
		FreezeEventHandler::FnReceiveEvent pFn = SafeWrite32(vptr + 4, &FreezeEventHandler::ReceiveEvent_Hook);
		ms_handlers.SetAt(vptr, pFn);
	}
};

BSTHashMap<UInt32, FreezeEventHandler::FnReceiveEvent> FreezeEventHandler::ms_handlers;

class MenuOpenCloseEventSource : public BSTEventSource<MenuOpenCloseEvent>
{
public:
	void ProcessHook()
	{
		lock.Lock();

		BSTEventSink<MenuOpenCloseEvent> *sink;
		UInt32 idx = 0;
		while (eventSinks.GetAt(idx, sink))
		{
			const char * className = GetObjectClassName(sink);

			if (strcmp(className, "class FreezeEventHandler") == 0)
			{
				FreezeEventHandler *freezeEventHandler = static_cast<FreezeEventHandler *>(sink);
				freezeEventHandler->InstallHook();
			}

			++idx;
		}

		lock.Unlock();
	}

	static void InitHook()
	{
		MenuManager *mm = MenuManager::GetSingleton();
		if (mm)
		{
			MenuOpenCloseEventSource *pThis = static_cast<MenuOpenCloseEventSource*>(mm->GetMenuOpenCloseEventSource());
			pThis->ProcessHook();
		}
	}
};

const UInt8 kCommentBytes_PUSH[] = {
	0x68
};

const UInt8 kCommentBytes_CALL[] = {
	0xE8
};

const UInt8 kCommentBytes_UseOriginalObjectsProcessing[] = {
	0x55, 0x73, 0x65, 0x4F, 0x72, 0x69, 0x67, 0x69, 0x6E, 0x61, 0x6C, 0x4F, 0x62, 0x6A,
	0x65, 0x63, 0x74, 0x73, 0x50, 0x72, 0x6F, 0x63, 0x65, 0x73, 0x73, 0x69, 0x6E, 0x67
};

uintptr_t GetCommentAddress1(uintptr_t start, UInt8* kCommentBytes, UInt32 kCommentByteCount)
{
	uintptr_t end = start + 0x00200000;
	start += 0x1000;
	for (; start < end; ++start)
	{
		if (!memcmp((void*)start, kCommentBytes, kCommentByteCount))
		{
			return start;
		}
	}
	return 0;
}

uintptr_t GetCommentAddress2(uintptr_t start, UInt8* kCommentBytes, UInt32 kCommentByteCount)
{
	uintptr_t end = start + 0x00100000;
	start += 0x1000;
	for (; start < end; ++start)
	{
		if (!memcmp((void*)start, kCommentBytes, kCommentByteCount) &&
			!memcmp((void*)(start - 0x6), kCommentBytes_PUSH, sizeof(kCommentBytes_PUSH)) && 
			!memcmp((void*)(start - 0x1), kCommentBytes_PUSH, sizeof(kCommentBytes_PUSH)) && 
			!memcmp((void*)(start + 0x4), kCommentBytes_PUSH, sizeof(kCommentBytes_PUSH)) &&
			!memcmp((void*)(start + 0xB), kCommentBytes_CALL, sizeof(kCommentBytes_CALL)))
		{
			return start;
		}
	}
	return 0;
}

bool IsBinaryCompatible(uintptr_t kCommentAddress, UInt8* kCommentBytes, UInt32 kCommentByteCount)
{
	return !memcmp((void*)kCommentAddress, kCommentBytes, kCommentByteCount);
}

//const UInt8 kCommentBytes436_1[] = {
//	0x0F, 0x84, 0xE1, 0x03, 0x00, 0x00, 0x83, 0x3D
//};
//
//const UInt8 kCommentBytes436_2[] = {
//	0x8B, 0x84, 0x24, 0xB4, 0x01, 0x00, 0x00
//};
//
//const UInt8 kCommentBytes434_1[] = {
//	0x0F, 0x84, 0xC0, 0x03, 0x00, 0x00, 0x83, 0x3D
//};
//
//const UInt8 kCommentBytes434_2[] = {
//	0x8B, 0x8C, 0x24, 0xC0, 0x01, 0x00, 0x00
//};
//
//const UInt8 kCommentBytes414_1[] = {
//	0x75, 0x2F, 0x8B, 0x84, 0x24, 0xC0, 0x01, 0x00, 0x00, 0x8B, 0x8C, 0x24, 0xB0, 0x01,
//	0x00, 0x00, 0x8B, 0x94, 0x24, 0xAC, 0x01, 0x00, 0x00, 0x50, 0x53, 0x56, 0x57,
//	0x51, 0x52, 0x8B, 0xCD, 0xE8
//};
//
//const UInt8 kCommentBytes414_2[] = {
//	0x5B, 0x5E, 0x5F, 0x5D, 0x81, 0xC4, 0x98, 0x01, 0x00, 0x00, 0xC2, 0x18, 0x00, 0x83, 0x3D
//};
//
//const UInt8 kCommentBytes389[] = {
//	0x75, 0x2F, 0x8B, 0x84, 0x24, 0xC0, 0x01, 0x00, 0x00, 0x8B, 0x8C, 0x24, 0xB0, 0x01,
//	0x00, 0x00, 0x8B, 0x94, 0x24, 0xAC, 0x01, 0x00, 0x00, 0x50, 0x53, 0x56, 0x57,
//	0x51, 0x52, 0x8B, 0xCD, 0xE8, 0xD1, 0xDC, 0xFF, 0xFF, 0x5B, 0x5E, 0x5F, 0x5D,
//	0x81, 0xC4, 0x98, 0x01, 0x00, 0x00, 0xC2, 0x18, 0x00, 0x83, 0x3D
//};
//
//const UInt8 kCommentBytes366_1[] = {
//	0x0F, 0x84, 0xB9, 0x03, 0x00, 0x00, 0x83, 0x3D
//};
//
//const UInt8 kCommentBytes366_2[] = {
//	0x8B, 0x84, 0x24, 0x80, 0x01, 0x00, 0x00
//};
//
//const UInt8 kCommentBytes319[] = {
//	0x75, 0x23, 0x8B, 0x44, 0x24, 0x7C, 0x8B, 0x4C, 0x24, 0x6C, 0x8B, 0x54, 0x24, 0x68,
//	0x50, 0x55, 0x53, 0x57, 0x51, 0x52, 0x8B, 0xCE, 0xE8, 0x5A, 0xE1, 0xFF, 0xFF,
//	0x5D, 0x5F, 0x5B, 0x5E, 0x83, 0xC4, 0x54, 0xC2, 0x18, 0x00, 0x83, 0x3D
//};
//
//const UInt8 kCommentBytes315_1[] = {
//	0x75, 0x06, 0x57, 0xE9, 0x2E, 0x02, 0x00, 0x00, 0x8B, 0x4C, 0x24, 0x10
//};
//
//const UInt8 kCommentBytes315_2[] = {
//	0x8B, 0x44, 0x24, 0x34
//};
//
//const UInt8 kCommentBytes304[] = {
//	0x75, 0x23, 0x8B, 0x54, 0x24, 0x3C, 0x8B, 0x44, 0x24, 0x2C, 0x8B, 0x4C, 0x24, 0x28,
//	0x52, 0x57, 0x55, 0x53, 0x50, 0x51, 0x8B, 0xCE, 0xE8, 0x72, 0xE1, 0xFF, 0xFF,
//	0x5B, 0x5F, 0x5D, 0x5E, 0x83, 0xC4, 0x14, 0xC2, 0x18, 0x00, 0x39, 0x82
//};
//
//const UInt8 kCommentBytes292[] = {
//	0x75, 0x23, 0x8B, 0x44, 0x24, 0x3C, 0x8B, 0x4C, 0x24, 0x2C, 0x8B, 0x54, 0x24, 0x28,
//	0x50, 0x57, 0x53, 0x55, 0x51, 0x52, 0x8B, 0xCE, 0xE8, 0x68, 0xDB, 0xFF, 0xFF,
//	0x5F, 0x5B, 0x5E, 0x5D, 0x83, 0xC4, 0x14, 0xC2, 0x18, 0x00, 0x8B, 0x54, 0x24, 0x14
//};
//
//uintptr_t GetCommentAddress(uintptr_t start, UInt8* kCommentBytes, UInt32 kCommentByteCount)
//{
//	uintptr_t end = start + 0x00030000;
//
//	for (; start < end; ++start)
//	{
//		if (!memcmp((void*)start, kCommentBytes, kCommentByteCount))
//		{
//			return start;
//		}
//	}
//	return 0;
//}
//
//bool IsBinaryCompatible(uintptr_t kCommentAddress, UInt8* kCommentBytes, UInt32 kCommentByteCount)
//{
//	return !memcmp((void*)kCommentAddress, kCommentBytes, kCommentByteCount);
//}
//
//static bool IsMiniMap()
//{
//	MiniMapMenu* MiniMapMenu = MiniMapMenu::GetSingleton();
//	if (MiniMapMenu && MiniMapMenu->IsProcessing())
//		return true;
//
//	return false;
//}
//
//static UInt32 kInstall_Base;
//static UInt32 kInstall_retn_original;
//static UInt32 kInstall_retn_enb;
//
//static void __declspec(naked) Hook_ENB436(void)
//{
//	__asm
//	{
//		je		jmp_original
//
//		call	IsMiniMap
//		test	al, al
//		jnz		jmp_original
//
//		jmp		kInstall_retn_enb
//
//jmp_original :
//		jmp		kInstall_retn_original
//	}
//}
//
//static bool TryHook_ENB436(HMODULE base)
//{
//	UInt32 kCommentByteCount_1 = sizeof(kCommentBytes436_1);
//	uintptr_t kCommentAddress_1 = GetCommentAddress((uintptr_t)base, (UInt8*)kCommentBytes436_1, kCommentByteCount_1);
//	if (kCommentAddress_1 && IsBinaryCompatible(kCommentAddress_1, (UInt8*)kCommentBytes436_1, kCommentByteCount_1))
//	{
//		UInt32 kCommentByteCount_2 = sizeof(kCommentBytes436_2);
//		uintptr_t kCommentAddress_2 = kCommentAddress_1 + 0x3E7;
//		if (IsBinaryCompatible(kCommentAddress_2, (UInt8*)kCommentBytes436_2, kCommentByteCount_2))
//		{
//			kInstall_Base = kCommentAddress_1;
//			kInstall_retn_original = kCommentAddress_2;
//			kInstall_retn_enb = kInstall_Base + 0x6;
//			WriteRelJump(kInstall_Base, &Hook_ENB436);
//			return true;
//		}
//	}
//	return false;
//}
//
//static void __declspec(naked) Hook_ENB434(void)
//{
//	__asm
//	{
//		je		jmp_original
//
//		call	IsMiniMap
//		test	al, al
//		jnz		jmp_original
//
//		jmp		kInstall_retn_enb
//
//jmp_original :
//		jmp		kInstall_retn_original
//	}
//}
//
//static bool TryHook_ENB434(HMODULE base)
//{
//	UInt32 kCommentByteCount_1 = sizeof(kCommentBytes434_1);
//	uintptr_t kCommentAddress_1 = GetCommentAddress((uintptr_t)base, (UInt8*)kCommentBytes434_1, kCommentByteCount_1);
//	if (kCommentAddress_1 && IsBinaryCompatible(kCommentAddress_1, (UInt8*)kCommentBytes434_1, kCommentByteCount_1))
//	{
//		UInt32 kCommentByteCount_2 = sizeof(kCommentBytes434_2);
//		uintptr_t kCommentAddress_2 = kCommentAddress_1 + 0x3C6;
//		if (IsBinaryCompatible(kCommentAddress_2, (UInt8*)kCommentBytes434_2, kCommentByteCount_2))
//		{
//			kInstall_Base = kCommentAddress_1;
//			kInstall_retn_original = kCommentAddress_2;
//			kInstall_retn_enb = kInstall_Base + 0x6;
//			WriteRelJump(kInstall_Base, &Hook_ENB434);
//			return true;
//		}
//	}
//	return false;
//}
//
//static void __declspec(naked) Hook_ENB414(void)
//{
//	__asm
//	{
//		jne		jmp_enb
//
//jmp_original :
//		mov		eax, dword ptr[esp + 0x1C0]
//		mov		ecx, dword ptr[esp + 0x1B0]
//		mov		edx, dword ptr[esp + 0x1AC]
//		jmp		kInstall_retn_original
//
//jmp_enb :
//		call	IsMiniMap
//		test	al, al
//		jnz		jmp_original
//
//		jmp		kInstall_retn_enb
//	}
//}
//
//static bool TryHook_ENB414(HMODULE base)
//{
//	UInt32 kCommentByteCount_1 = sizeof(kCommentBytes414_1);
//	uintptr_t kCommentAddress_1 = GetCommentAddress((uintptr_t)base, (UInt8*)kCommentBytes414_1, kCommentByteCount_1);
//	if (kCommentAddress_1 && IsBinaryCompatible(kCommentAddress_1, (UInt8*)kCommentBytes414_1, kCommentByteCount_1))
//	{
//		UInt32 kCommentByteCount_2 = sizeof(kCommentBytes414_2);
//		uintptr_t kCommentAddress_2 = kCommentAddress_1 + 0x24;
//		if (IsBinaryCompatible(kCommentAddress_2, (UInt8*)kCommentBytes414_2, kCommentByteCount_2))
//		{
//			kInstall_Base = kCommentAddress_1;
//			kInstall_retn_original = kInstall_Base + 0x17;
//			kInstall_retn_enb = kInstall_Base + 0x31;
//			WriteRelJump(kInstall_Base, &Hook_ENB414);
//			return true;
//		}
//	}
//	return false;
//
//}
//
//static void __declspec(naked) Hook_ENB389(void)
//{
//	__asm
//	{
//		jne		jmp_enb
//
//jmp_original :
//		mov		eax, dword ptr[esp + 0x1C0]
//		mov		ecx, dword ptr[esp + 0x1B0]
//		mov		edx, dword ptr[esp + 0x1AC]
//		jmp		kInstall_retn_original
//
//jmp_enb :
//		call	IsMiniMap
//		test	al, al
//		jnz		jmp_original
//
//		jmp		kInstall_retn_enb
//	}
//}
//
//static bool TryHook_ENB389(HMODULE base)
//{
//	UInt32 kCommentByteCount = sizeof(kCommentBytes389);
//	uintptr_t kCommentAddress = GetCommentAddress((uintptr_t)base, (UInt8*)kCommentBytes389, kCommentByteCount);
//	if (kCommentAddress && IsBinaryCompatible(kCommentAddress, (UInt8*)kCommentBytes389, kCommentByteCount))
//	{
//		kInstall_Base = kCommentAddress;
//		kInstall_retn_original = kInstall_Base + 0x17;
//		kInstall_retn_enb = kInstall_Base + 0x31;
//		WriteRelJump(kInstall_Base, &Hook_ENB389);
//		return true;
//	}
//	return false;
//}
//
//static void __declspec(naked) Hook_ENB366(void)
//{
//	__asm
//	{
//		je		jmp_original
//
//		call	IsMiniMap
//		test	al, al
//		jnz		jmp_original
//
//		jmp		kInstall_retn_enb
//
//jmp_original :
//		jmp		kInstall_retn_original
//	}
//}
//
//static bool TryHook_ENB366(HMODULE base)
//{
//	UInt32 kCommentByteCount_1 = sizeof(kCommentBytes366_1);
//	uintptr_t kCommentAddress_1 = GetCommentAddress((uintptr_t)base, (UInt8*)kCommentBytes366_1, kCommentByteCount_1);
//	if (kCommentAddress_1 && IsBinaryCompatible(kCommentAddress_1, (UInt8*)kCommentBytes366_1, kCommentByteCount_1))
//	{
//		UInt32 kCommentByteCount_2 = sizeof(kCommentBytes366_2);
//		uintptr_t kCommentAddress_2 = kCommentAddress_1 + 0x3BF;
//		if (IsBinaryCompatible(kCommentAddress_2, (UInt8*)kCommentBytes366_2, kCommentByteCount_2))
//		{
//			kInstall_Base = kCommentAddress_1;
//			kInstall_retn_original = kCommentAddress_2;
//			kInstall_retn_enb = kInstall_Base + 0x6;
//			WriteRelJump(kInstall_Base, &Hook_ENB366);
//			return true;
//		}
//	}
//	return false;
//}
//
//static void __declspec(naked) Hook_ENB319(void)
//{
//	__asm
//	{
//		jne		jmp_enb
//
//jmp_original :
//		mov		eax, dword ptr[esp + 0x7C]
//		mov		ecx, dword ptr[esp + 0x6C]
//		mov		edx, dword ptr[esp + 0x68]
//		jmp		kInstall_retn_original
//
//jmp_enb :
//		call	IsMiniMap
//		test	al, al
//		jnz		jmp_original
//
//		jmp		kInstall_retn_enb
//	}
//}
//
//static bool TryHook_ENB319(HMODULE base)
//{
//	UInt32 kCommentByteCount = sizeof(kCommentBytes319);
//	uintptr_t kCommentAddress = GetCommentAddress((uintptr_t)base, (UInt8*)kCommentBytes319, kCommentByteCount);
//	if (kCommentAddress && IsBinaryCompatible(kCommentAddress, (UInt8*)kCommentBytes319, kCommentByteCount))
//	{
//		kInstall_Base = kCommentAddress;
//		kInstall_retn_original = kInstall_Base + 0xE;
//		kInstall_retn_enb = kInstall_Base + 0x25;
//		WriteRelJump(kInstall_Base, &Hook_ENB319);
//		return true;
//	}
//	return false;
//}
//
//static void __declspec(naked) Hook_ENB315(void)
//{
//	__asm
//	{
//		jne		jmp_enb
//
//jmp_original :
//		push	edi
//		jmp		kInstall_retn_original
//
//jmp_enb :
//		call	IsMiniMap
//		test	al, al
//		jnz		jmp_original
//
//		jmp		kInstall_retn_enb
//	}
//}
//
//static bool TryHook_ENB315(HMODULE base)
//{
//	UInt32 kCommentByteCount_1 = sizeof(kCommentBytes315_1);
//	uintptr_t kCommentAddress_1 = GetCommentAddress((uintptr_t)base, (UInt8*)kCommentBytes315_1, kCommentByteCount_1);
//	if (kCommentAddress_1 && IsBinaryCompatible(kCommentAddress_1, (UInt8*)kCommentBytes315_1, kCommentByteCount_1))
//	{
//		UInt32 kCommentByteCount_2 = sizeof(kCommentBytes315_2);
//		uintptr_t kCommentAddress_2 = kCommentAddress_1 + 0x236;
//		if (IsBinaryCompatible(kCommentAddress_2, (UInt8*)kCommentBytes315_2, kCommentByteCount_2))
//		{
//			kInstall_Base = kCommentAddress_1;
//			kInstall_retn_original = kCommentAddress_2;
//			kInstall_retn_enb = kInstall_Base + 0x8;
//			WriteRelJump(kInstall_Base, &Hook_ENB315);
//			return true;
//		}
//	}
//	return false;
//}
//
//static void __declspec(naked) Hook_ENB304(void)
//{
//	__asm
//	{
//		jne		jmp_enb
//
//jmp_original :
//		mov		edx, dword ptr[esp + 0x3C]
//		mov		eax, dword ptr[esp + 0x2C]
//		mov		ecx, dword ptr[esp + 0x28]
//		jmp		kInstall_retn_original
//
//jmp_enb :
//		call	IsMiniMap
//		test	al, al
//		jnz		jmp_original
//
//		jmp		kInstall_retn_enb
//	}
//}
//
//static bool TryHook_ENB304(HMODULE base)
//{
//	UInt32 kCommentByteCount = sizeof(kCommentBytes304);
//	uintptr_t kCommentAddress = GetCommentAddress((uintptr_t)base, (UInt8*)kCommentBytes304, kCommentByteCount);
//	if (kCommentAddress && IsBinaryCompatible(kCommentAddress, (UInt8*)kCommentBytes304, kCommentByteCount))
//	{
//		kInstall_Base = kCommentAddress;
//		kInstall_retn_original = kInstall_Base + 0xE;
//		kInstall_retn_enb = kInstall_Base + 0x25;
//		WriteRelJump(kInstall_Base, &Hook_ENB304);
//		return true;
//	}
//	return false;
//}
//
//static void __declspec(naked) Hook_ENB292(void)
//{
//	__asm
//	{
//		jne		jmp_enb
//
//jmp_original :
//		mov		eax, dword ptr[esp + 0x3C]
//		mov		ecx, dword ptr[esp + 0x2C]
//		mov		edx, dword ptr[esp + 0x28]
//		jmp		kInstall_retn_original
//
//jmp_enb :
//		call	IsMiniMap
//		test	al, al
//		jnz		jmp_original
//
//		jmp		kInstall_retn_enb
//	}
//}
//
//static bool TryHook_ENB292(HMODULE base)
//{
//	UInt32 kCommentByteCount = sizeof(kCommentBytes292);
//	uintptr_t kCommentAddress = GetCommentAddress((uintptr_t)base, (UInt8*)kCommentBytes292, kCommentByteCount);
//	if (kCommentAddress && IsBinaryCompatible(kCommentAddress, (UInt8*)kCommentBytes292, kCommentByteCount))
//	{
//		kInstall_Base = kCommentAddress;
//		kInstall_retn_original = kInstall_Base + 0xE;
//		kInstall_retn_enb = kInstall_Base + 0x25;
//		WriteRelJump(kInstall_Base, &Hook_ENB292);
//		return true;
//	}
//
//	return false;
//}

static void Hook_004AEB56(UInt32 ecx, UInt32* stack)
{
	TESObjectREFR* ref = (TESObjectREFR*)stack[1];
	MiniMapMenu::AddDoor(ref->formID, ref);
}

static HMODULE GetENBModule()
{
	DWORD	cb = 1000 * sizeof(HMODULE);
	DWORD	cbNeeded = 0;
	HMODULE	enbmodule = NULL;
	HMODULE	hmodules[1000];
	HANDLE	hproc = GetCurrentProcess();
	for (long i = 0; i<1000; i++) hmodules[i] = NULL;
	//find proper library by existance of exported function, because several with the same name may exist
	if (EnumProcessModules(hproc, hmodules, cb, &cbNeeded))
	{
		long	count = cbNeeded / sizeof(HMODULE);
		for (long i = 0; i<count; i++)
		{
			if (hmodules[i] == NULL) break;
			void	*func = (void*)GetProcAddress(hmodules[i], "ENBGetSDKVersion");
			if (func)
			{
				enbmodule = hmodules[i];
				break;
			}
		}
	}

	return enbmodule;
}

static UInt32 FindPointer_ENB(HMODULE base)
{
	UInt32 UseOriginalObjectsProcessing_Count = sizeof(kCommentBytes_UseOriginalObjectsProcessing);
	uintptr_t UseOriginalObjectsProcessing_Address = GetCommentAddress1((uintptr_t)base, (UInt8*)kCommentBytes_UseOriginalObjectsProcessing, UseOriginalObjectsProcessing_Count);
	if (UseOriginalObjectsProcessing_Address && IsBinaryCompatible(UseOriginalObjectsProcessing_Address, (UInt8*)kCommentBytes_UseOriginalObjectsProcessing, UseOriginalObjectsProcessing_Count))
	{
		UInt8 kCommentBytes_LittleEndian[4];
		kCommentBytes_LittleEndian[0] = UseOriginalObjectsProcessing_Address % 0x100;
		kCommentBytes_LittleEndian[1] = UseOriginalObjectsProcessing_Address % 0x10000 / 0x100;
		kCommentBytes_LittleEndian[2] = UseOriginalObjectsProcessing_Address % 0x1000000 / 0x10000;
		kCommentBytes_LittleEndian[3] = UseOriginalObjectsProcessing_Address / 0x1000000;
		UInt32 LittleEndian_Count = sizeof(kCommentBytes_LittleEndian);
		uintptr_t ref_Address = GetCommentAddress2((uintptr_t)base, (UInt8*)kCommentBytes_LittleEndian, LittleEndian_Count);
		if (ref_Address && IsBinaryCompatible(ref_Address, (UInt8*)kCommentBytes_LittleEndian, LittleEndian_Count))
			return *(UInt32*)(ref_Address - 0x5);
	}

	return 0;
}

class MiniMapPlugin : public SKSEPlugin
{
public:
	MiniMapPlugin()
	{
	}

	virtual bool InitInstance() override
	{
		if (!Requires(kSKSEVersion_1_7_3))
		{
			gLog << "ERROR: your skse version is too old." << std::endl;
			return false;
		}

		SetName("MiniMap");
		SetVersion(1);

		return true;
	}

	virtual bool OnLoad() override
	{
		SKSEPlugin::OnLoad();

		//HMODULE enbModule = GetENBModule();
		//if (enbModule)
		//{
		//	typedef long(*_ENBGetVersion)();
		//	_ENBGetVersion fnENBGetVersion = (_ENBGetVersion)GetProcAddress(enbModule, "ENBGetVersion");
		//	if (fnENBGetVersion)
		//	{
		//		long ENBVersion = fnENBGetVersion();
		//		_MESSAGE("ENBVersion  =  %ld", ENBVersion);
		//		bool done = false;
		//		if (ENBVersion >= 436)
		//		{
		//			done = TryHook_ENB436(enbModule);
		//		}
		//		else if (ENBVersion >= 434)
		//		{
		//			done = TryHook_ENB434(enbModule);
		//		}
		//		else if (ENBVersion > 431)
		//		{
		//			done = TryHook_ENB434(enbModule);
		//			if (!done)
		//				done = TryHook_ENB414(enbModule);
		//		}
		//		else if (ENBVersion >= 414)
		//		{
		//			done = TryHook_ENB414(enbModule);
		//		}
		//		else if (ENBVersion >= 389)
		//		{
		//			done = TryHook_ENB389(enbModule);
		//		}
		//		else if (ENBVersion >= 366)
		//		{
		//			done = TryHook_ENB366(enbModule);
		//		}
		//		else if (ENBVersion > 357)
		//		{
		//			done = TryHook_ENB366(enbModule);
		//			if (!done)
		//				done = TryHook_ENB319(enbModule);
		//		}
		//		else if (ENBVersion >= 319)
		//		{
		//			done = TryHook_ENB319(enbModule);
		//		}
		//		else if (ENBVersion > 315)
		//		{
		//			done = TryHook_ENB319(enbModule);
		//			if (!done)
		//				done = TryHook_ENB315(enbModule);
		//		}
		//		else if (ENBVersion == 315)
		//		{
		//			done = TryHook_ENB315(enbModule);
		//		}
		//		else if (ENBVersion >= 304)
		//		{
		//			done = TryHook_ENB304(enbModule);
		//		}
		//		else
		//		{
		//			done = TryHook_ENB304(enbModule);
		//			if (!done)
		//				done = TryHook_ENB292(enbModule);
		//		}

		//		if (!done)
		//			_MESSAGE("couldn't hook address");
		//	}
		//	else
		//	{
		//		_MESSAGE("couldn't find ENBGetVersion");
		//	}
		//}
		//else
		//{
		//	_MESSAGE("couldn't find ENB module");
		//}

		return true;
	}

	virtual void OnModLoaded() override
	{
		if (!ini.Load())
		{
			_MESSAGE("couldn't load ini file");
			return;
		}

		if (ini.OverrideDirectionalAmbient)
		{
			TESDataHandler* dhnd = TESDataHandler::GetSingleton();
			for (auto& wt : dhnd->weather)
			{
				for (int i = 0; i < TESWeather::kNumTimeOfDay; i++)
				{
					wt->directionalAmbient[i].x[0] = 0x00FFFFFF;
					wt->directionalAmbient[i].x[1] = 0x00FFFFFF;
					wt->directionalAmbient[i].y[0] = 0x00FFFFFF;
					wt->directionalAmbient[i].y[1] = 0x00FFFFFF;
					wt->directionalAmbient[i].z[0] = 0x00FFFFFF;
					wt->directionalAmbient[i].z[1] = 0x00FFFFFF;
				}
			}
		}

		UInt32 pointer_ENB = 0;
		if (ini.AvoidEnbProcessing)
		{
			HMODULE enbModule = GetENBModule();
			if (enbModule)
			{
				typedef long(*_ENBGetVersion)();
				_ENBGetVersion fnENBGetVersion = (_ENBGetVersion)GetProcAddress(enbModule, "ENBGetVersion");
				if (fnENBGetVersion)
				{
					long ENBVersion = fnENBGetVersion();
					_MESSAGE("ENBVersion  =  %ld", ENBVersion);
					pointer_ENB = FindPointer_ENB(enbModule);
					if (pointer_ENB == 0)
						_MESSAGE("couldn't find pointer");
				}
				else
				{
					_MESSAGE("couldn't find ENBGetVersion");
				}
			}
			else
			{
				_MESSAGE("couldn't find ENB module");
			}
		}

		HookRelCall(0x004AEB56, Hook_004AEB56);
		if (MiniMapMenu::Register((UInt32*)pointer_ENB))
			MenuOpenCloseEventSource::InitHook();
	}
} thePlugin;
