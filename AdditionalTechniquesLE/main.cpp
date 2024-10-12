#include <SKSE.h>
#include <SKSE/PluginAPI.h>
#include <SKSE/DebugLog.h>
#include <SKSE/GameRTTI.h>
#include <SKSE/HookUtil.h>

#include "iniSettings.h"
#include "TechniquesManager.h"
#include "TechniquesMenu.h"

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
		static BSFixedString menuName = "TechniquesMenu";

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

class AdditionalTechniquesPlugin : public SKSEPlugin
{
public:
	AdditionalTechniquesPlugin()
	{
	}

	virtual bool InitInstance() override
	{
		if (!Requires(kSKSEVersion_1_7_1))
			return false;

		SetName("AdditionalTechniques");
		SetVersion(1);

		return true;
	}

	virtual bool OnLoad() override
	{
		SKSEPlugin::OnLoad();
		const SKSEMessagingInterface *mes = GetInterface(SKSEMessagingInterface::Version_1);
		if (!mes)
			return false;

		mes->RegisterListener("SKSE", TechniquesManager::TechniquesMessageHandler);

		return true;
	}

	virtual void OnModLoaded() override
	{
		ini.Load();
		TechniquesManager::Init();
		if (((!ini.DisableHomingMissile && ini.DisplayMarkerHM) || (!ini.DisableHomingArrow && ini.DisplayMarkerHA)) && TechniquesMenu::Register())
			MenuOpenCloseEventSource::InitHook();
	}

} thePlugin;
