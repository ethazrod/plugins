#include <SKSE.h>
#include <SKSE/PluginAPI.h>
#include <SKSE/DebugLog.h>
#include <SKSE/GameRTTI.h>
#include <SKSE/SafeWrite.h>

#include "FloatingDamage.h"
#include "HookDamage.h"
#include "iniSettings.h"

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
		static BSFixedString menuName = "Floating Damage";

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

class FloatingDamagePlugin : public SKSEPlugin
{
public:
	FloatingDamagePlugin()
	{
	}

	virtual bool InitInstance() override
	{
		if (!Requires(kSKSEVersion_1_7_3))
		{
			gLog << "ERROR: your skse version is too old." << std::endl;
			return false;
		}

		SetName("FloatingDamage");
		SetVersion(1);

		return true;
	}

	virtual bool OnLoad() override
	{
		SKSEPlugin::OnLoad();

		return true;
	}

	virtual void OnModLoaded() override
	{
		if (!ini.Load())
		{
			_MESSAGE("couldn't load ini file");
			return;
		}

		if (FloatingDamage::Register())
		{
			MenuOpenCloseEventSource::InitHook();
			HookDamage::Init();
		}
	}
} thePlugin;
