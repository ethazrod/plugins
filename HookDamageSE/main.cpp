#include "skse64/PluginAPI.h"
#include "skse64_common/skse_version.h"
#include "skse64_common/BranchTrampoline.h"
#include "skse64/GameRTTI.h"

#include "Addresses.h"
#include "HookDamage.h"
#include "Hooks.h"

#include <shlobj.h>

IDebugLog	gLog;
UInt32 g_skseVersion;
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;
SKSEMessagingInterface	* g_messaging;

void SKSEMessageHandler(SKSEMessagingInterface::Message* msg)
{
	if (msg->type == SKSEMessagingInterface::kMessage_DataLoaded)
	{
		g_messaging->Dispatch(g_pluginHandle, HookDamage::kType_Register, HookDamage::RegisterEvent, HookDamage::kHookDamageVersion, 0);
	}
}

extern "C"
{

	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		SInt32	logLevel = IDebugLog::kLevel_DebugMessage;
		gLog.SetLogLevel((IDebugLog::LogLevel)logLevel);

		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\HookDamage.log");

		_MESSAGE("HookDamage");

		g_skseVersion = skse->skseVersion;

		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "HookDamage";
		info->version = 1;

		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor)
		{
			_MESSAGE("loaded in editor, marking as incompatible");
			return false;
		}

		g_messaging = (SKSEMessagingInterface *)skse->QueryInterface(kInterface_Messaging);
		if (!g_messaging)
		{
			_MESSAGE("couldn't get messaging interface");
			return false;
		}
		if (g_messaging->interfaceVersion < 1)
		{
			_MESSAGE("messaging interface too old (%d expected %d)", g_messaging->interfaceVersion, 1);
			return false;
		}

		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
		_MESSAGE("HookDamage Loaded");

		if (!Addresses::Init())
			return false;

		if (!g_branchTrampoline.Create(1024 * 64))
		{
			_ERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
			return false;
		}

		if (!g_localTrampoline.Create(1024 * 64, nullptr))
		{
			_ERROR("couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
			return false;
		}

		g_messaging->RegisterListener(g_pluginHandle, "SKSE", SKSEMessageHandler);
		Hooks::Init();

		return true;
	}

};
