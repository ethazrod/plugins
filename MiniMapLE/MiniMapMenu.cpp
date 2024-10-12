#include "MiniMapMenu.h"
#include "InputMap.h"
#include "utils.h"
#include "iniSettings.h"

#include <SKSE/DebugLog.h>
#include <SKSE/GameData.h>
#include <SKSE/GameForms.h>
#include <SKSE/GameRTTI.h>
#include <SKSE/GameReferences.h>
#include <SKSE/GameCamera.h>
#include <SKSE/GameExtraData.h>
#include <Skyrim/BSMain/Main.h>

#define _USE_MATH_DEFINES
#include <math.h>

float* units = (float*)0x012E3570; //69.99125f

class LoadedAreaBoundEx : public LoadedAreaBound
{
public:
	struct alignas(16) NiBounds
	{
		NiBound boundsMin;
		NiBound boundsMax;
	};
	DEFINE_MEMBER_FN(Fn005853C0, bool, 0x005853C0, TESObjectCELL*, GridCellArray*, NiBounds*);
};

#define ADDR_UnkCellInfo 0x012E32E8

struct UnkCellInfo
{
	UInt32          unk00;
	UInt32          unk04;
	UInt32          unk08;
	UInt32          unk0C;
	UInt32          unk10;
	UInt32          unk14;
	UInt32          unk18;
	UInt32          unk1C;
	UInt32          unk20;
	UInt32	        unk24;
	BSTArray<UInt32>  actorHandles;	// 28
	BSTArray<UInt32>  objectHandles;	// 34
};
static UnkCellInfo** s_cellInfo = (UnkCellInfo**)ADDR_UnkCellInfo;

bool* unk1BA8454 = (bool*)0x01BA8454;
bool* unk12CF4B4 = (bool*)0x012CF4B4;
bool* unk12CF4B5 = (bool*)0x012CF4B5;
bool* unk1BA72A8 = (bool*)0x01BA72A8;

bool* unk12CFC58 = (bool*)0x012CFC58;

class Unk12E353C
{
public:
	DEFINE_MEMBER_FN(Fn005AB8D0, void, 0x005AB8D0, UInt32 arg1);
	static Unk12E353C * GetSingleton()
	{
		return *((Unk12E353C **)0x012E353C);
	}
};

class ShaderManager
{
public:
	static ShaderManager * GetSingleton()
	{
		return *((ShaderManager **)0x01BA7680);
	}

	UInt32 unk00[(0xAC - 0x00) >> 2];
	UInt32 unkAC;
	UInt16 unkB0;
	UInt16 unkB2;
	UInt32 unkB4[(0x148 - 0xB4) >> 2];
	UInt8  unk148;
};

class AutoOpenMiniMapMenuSink : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	EventResult ReceiveEvent(MenuOpenCloseEvent *evn, BSTEventSource<MenuOpenCloseEvent> *src)
	{
		MenuManager *mm = MenuManager::GetSingleton();
		UIStringHolder *holder = UIStringHolder::GetSingleton();

		if (evn->menuName == holder->loadingMenu && evn->IsClose() && !mm->IsMenuOpen(holder->mainMenu))
		{
			MiniMapMenu *MiniMapMenu = MiniMapMenu::GetSingleton();
			if (!MiniMapMenu)
			{
				UIManager *ui = UIManager::GetSingleton();
				ui->AddMessage("MiniMapMenu", UIMessage::kMessage_Open, nullptr);
			}

			mm->BSTEventSource<MenuOpenCloseEvent>::RemoveEventSink(this);
			delete this;
		}

		return kEvent_Continue;
	}

	static void Register()
	{
		MenuManager *mm = MenuManager::GetSingleton();
		if (mm)
		{
			AutoOpenMiniMapMenuSink *sink = new AutoOpenMiniMapMenuSink();
			if (sink)
			{
				mm->BSTEventSource<MenuOpenCloseEvent>::AddEventSink(sink);
			}
		}
	}
};

MiniMapMenu * MiniMapMenu::ms_pSingleton = nullptr;
BSSpinLock MiniMapMenu::ms_lock;
BSFixedString MiniMapMenu::miniMapName = "Mini_Map";
std::map<UInt32, TESObjectREFR*> MiniMapMenu::doors;
UInt32* MiniMapMenu::useOriginal = nullptr;

MiniMapMenu::MiniMapMenu() : loadTerrain(false), visible(0), bZoom(false), isProcessing(false)
{
	bHide = ini.InitiallyHidden ? true : false;

	memset(cullingProcessMemory, 0, 0x23C);
	cullingProcess = (LocalMapCullingProcess*)cullingProcessMemory;
	cullingProcess->ctor();
	cullingProcessPtr = &cullingProcess->cullingProcess;
	cullingProcess->CreateMapTarget(ini.Width, ini.Height);
	cullingProcess->localMapTexture->renderedTexture[0]->name = miniMapName;

	float minFrustumWidth = ini.MinFrustumWidth;
	float minFrustumRatio = minFrustumWidth * ((float)ini.Height / (float)ini.Width);
	cullingProcess->localMapCamera.InitMinFrustum(minFrustumWidth, minFrustumRatio);
	//NiPoint3 maxBound = g_TES->loadedAreaBound->boundsMax;
	//NiPoint3 minBound = g_TES->loadedAreaBound->boundsMin;
	//cullingProcess->localMapCamera.InitAreaBounds(&maxBound, &minBound);
	cullingProcess->Init();

	const char swfName[] = "MiniMapMenu";

	if (LoadMovie(swfName, GFxMovieView::SM_NoScale, 0.0f))
	{
		_MESSAGE("loaded Inteface/%s.swf", swfName);

		menuDepth = 2;
		flags = kType_DoNotDeleteOnClose | kType_DoNotPreventGameSave | kType_Unk1000 | kType_Unk10000;
	}
}

MiniMapMenu::~MiniMapMenu()
{
}

bool MiniMapMenu::Register(UInt32* pointer_ENB)
{
	useOriginal = pointer_ENB;
	MenuManager *mm = MenuManager::GetSingleton();
	if (!mm)
		return false;

	mm->Register("MiniMapMenu", []() -> IMenu * { return new MiniMapMenu; });

	AutoOpenMiniMapMenuSink::Register();

	return true;
}

void MiniMapMenu::AddDoor(UInt32 formId, TESObjectREFR* ref)
{
	BSSpinLockGuard guard(ms_lock);

	doors.insert(std::make_pair(formId, ref));
}

bool MiniMapMenu::IsProcessing()
{
	return isProcessing;
}

UInt32 MiniMapMenu::ProcessMessage(UIMessage *message)
{
	UInt32 result = kResult_NotProcessed;

	if (view)
	{
		switch (message->type)
		{
		case UIMessage::kMessage_Open:
			OnMenuOpen();
			result = kResult_Processed;
			break;
		case UIMessage::kMessage_Close:
			OnMenuClose();
			result = kResult_Processed;
			break;
		}
	}

	return result;
}

void MiniMapMenu::Render()
{
	BSSpinLockGuard guard(ms_lock);

	GFxValue argDoors;
	view->CreateArray(&argDoors);
	GFxValue argActors;
	view->CreateArray(&argActors);
	GFxValue argQuests;
	view->CreateArray(&argQuests);

	NiCamera* niCamera = (NiCamera*)cullingProcess->localMapCamera.niCamera;

	auto it = doors.begin();
	while (it != doors.end())
	{
		TESForm* form = LookupFormByID(it->first);
		if (!form || form->formType != FormType::Reference || form != it->second)
		{
			doors.erase(it++);
			continue;
		}

		TESObjectREFR* ref = it->second;
		TESObjectCELL* cell = ref->GetParentCell();
		if (!cell || !cell->IsAttached())
		{
			doors.erase(it++);
			continue;
		}

		//ExtraTeleport* xTeleport = static_cast<ExtraTeleport*>(ref->extraData.GetExtraData(ExtraDataType::Teleport));
		//if (!xTeleport || !xTeleport->data)
		//{
		//	doors.erase(it++);
		//	continue;
		//}

		if (!ref->flags.disabled)
		{
			GFxValue id = (double)ref->formID;
			NiPoint3 screenPos;
			NiPoint3 worldPos = ref->pos;
			niCamera->WorldPtToScreenPt3(worldPos, screenPos);
			GFxValue x = (double)ini.Width * screenPos.x;
			GFxValue y = (double)ini.Height * (1 - screenPos.y);
			GFxValue type = (double)Marker_KnownDoor;

			GFxValue value;
			view->CreateObject(&value);
			value.SetMember("id", id);
			value.SetMember("x", x);
			value.SetMember("y", y);
			value.SetMember("type", type);

			argDoors.PushBack(value);
		}

		it++;
	}

	for (auto handle : (*s_cellInfo)->actorHandles)
	{
		if (handle != g_invalidRefHandle)
		{
			TESObjectREFRPtr refPtr;
			if (TESObjectREFR::LookupByHandle(handle, refPtr) && refPtr->formType == FormType::Character && !refPtr->flags.disabled && refPtr != g_thePlayer)
			{
				Actor* actor = static_cast<Actor*>((TESForm*)refPtr);

				GFxValue id = (double)actor->formID;
				NiPoint3 screenPos;
				NiPoint3 worldPos = actor->pos;
				niCamera->WorldPtToScreenPt3(worldPos, screenPos);
				GFxValue x = (double)ini.Width * screenPos.x;
				GFxValue y = (double)ini.Height * (1 - screenPos.y);
				UInt32 actorType = Marker_Actor;
				if (actor->IsDead(true))
					actorType = Marker_Dead;
				else if (actor->IsPlayerTeammate())
					actorType = Marker_Follower;
				else if (g_thePlayer->IsHostileToActor(actor))
					actorType = Marker_Hostile;

				GFxValue type = (double)actorType;

				GFxValue value;
				view->CreateObject(&value);
				value.SetMember("id", id);
				value.SetMember("x", x);
				value.SetMember("y", y);
				value.SetMember("type", type);

				argActors.PushBack(value);
			}
		}
	}

	struct QuestData
	{
		TESQuest::Objective*	objective;
		UInt32					unk04;
		UInt32					unk08;
	};

	BSTArray<QuestData>* questDataList = (BSTArray<QuestData>*)((char*)g_thePlayer + 0x338);
	for (auto& data : *questDataList)
	{
		if (data.unk08 == 1 && data.objective)
		{
			TESQuest* owner = data.objective->owner;
			if (owner)
			{
				for (int i = 0; i < data.objective->unk0C; i++)
				{
					QuestTarget* target = ((QuestTarget**)data.objective->unk08)[i];
					if (target)
					{
						RefHandle handle;
						if (owner->refAliasMap.GetAt(target->aliasID, handle) && handle != g_invalidRefHandle)
						{
							TESObjectREFRPtr ref;
							if (LookupREFRByHandle(handle, ref))
							{
								TESObjectCELL* cell = ref->GetParentCell();
								if (cell && cell->IsAttached() && ref->Is3DLoaded() && !ref->flags.disabled && Utils::QuestTargetConditionMatch(owner, target))
								{
									GFxValue id = (double)ref->formID;
									NiPoint3 screenPos;
									NiPoint3 worldPos = ref->pos;
									niCamera->WorldPtToScreenPt3(worldPos, screenPos);
									GFxValue x = (double)ini.Width * screenPos.x;
									GFxValue y = (double)ini.Height * (1 - screenPos.y);
									GFxValue type = (double)Marker_Quest;

									GFxValue value;
									view->CreateObject(&value);
									value.SetMember("id", id);
									value.SetMember("x", x);
									value.SetMember("y", y);
									value.SetMember("type", type);

									argQuests.PushBack(value);
								}
							}
						}
					}
				}
			}
		}
	}

	double angle = g_thePlayer->rot.z * 180.0 / M_PI;
	double scale = (double)ini.MarkerSize;
	if (bZoom)
		scale *= ini.ZoomPercent2 / ini.ZoomPercent1;

	std::vector<GFxValue> args;
	args.reserve(5);
	args.emplace_back(angle);
	args.emplace_back(scale);
	args.push_back(argDoors);
	args.push_back(argActors);
	args.push_back(argQuests);

	view->Invoke("_root.widget.UpdateWidgets", nullptr, &args[0], args.size());

	MenuManager* mm = MenuManager::GetSingleton();
	PlayerCamera* Camera = PlayerCamera::GetSingleton();
	if (bHide || (mm && (mm->numPauseGame != 0 || mm->numStopCrosshairUpdate != 0)) || (Camera && Camera->cameraState == Camera->cameraStates[PlayerCamera::kCameraState_VATS]))
	{
		if (view->GetVisible())
			view->SetVisible(false);

		visible = 0;
	}
	else
	{
		if (!view->GetVisible())
		{
			if (visible == 2)
				view->SetVisible(true);
			else if (visible == 0)
				visible = 1;
		}
		else
		{
			visible = 2;
		}
	}

	IMenu::Render();
}

void MiniMapMenu::Unk_07()
{
	if (MenuManager::IsInMenuMode() || visible == 0 || !UpdateCamera())
		return;

	ShaderManager* sm = ShaderManager::GetSingleton();
	//sm->unk148 = 1;
	cullingProcess->shaderAccumulator->unk1A0 = sm;
	//UInt32 unk0C = *(UInt32*)((char*)sm->unkAC + 0x0C);
	//*(UInt32*)((char*)unk0C + 0x98) &= 0xFFFFFFFE;
	//*unk1BA8454 = 1;
	//*unk12CF4B4 = 0;
	//*unk12CF4B5 = 0;
	//*unk1BA72A8 = 0;
	//Unk12E353C::GetSingleton()->Fn005AB8D0(1);

	NiDX9Renderer::GetSingleton()->Fn00486140(0x12C5C90);

	//if (g_TES->currentCell)
	//{
	//	*unk12CFC58 = 0;
	//}
	//else
	//{
	//	if (!g_TES->worldSpace)
	//	{
	//		*unk12CFC58 = 1;
	//	}
	//	else
	//	{
	//		if (g_TES->worldSpace->Fn004F0200())
	//			*unk12CFC58 = 0;
	//		else
	//			*unk12CFC58 = 1;
	//	}
	//}

	if (!g_TES->worldSpace || (*(UInt8*)((char*)g_TES->worldSpace + 0x5C) & 0x40) == 0)
		loadTerrain = true;
	else
		loadTerrain = false;

	g_TES->LoadObjects(&cullingProcessPtr, 0);

	BSPortalGraphEntry* portalEntry = (BSPortalGraphEntry*)g_main->Fn00691400();
	if (portalEntry)
	{
		UInt32 obj = *(UInt32*)((char*)portalEntry + 0x8);
		if (obj != 0)
		{
			obj += 0x2C;
			typedef UInt32(*Fn00C8D8D0)(NiCamera*, NiAVObject*, BSCullingProcess*, UInt32);
			const Fn00C8D8D0 fn00C8D8D0 = (Fn00C8D8D0)0x00C8D8D0;
			fn00C8D8D0((NiCamera*)cullingProcess->localMapCamera.niCamera, (NiAVObject*)obj, cullingProcessPtr, 0);
		}
	}

	typedef UInt32(*Fn00C8D180)(NiCamera*, NiAVObject*, BSCullingProcess*);
	const Fn00C8D180 fn00C8D180 = (Fn00C8D180)0x00C8D180;
	if (sm->unkB2 == 9)
	{
		UInt32 obj = *(UInt32*)((char*)sm->unkAC + 0x24);
		fn00C8D180((NiCamera*)cullingProcess->localMapCamera.niCamera, (NiAVObject*)obj, cullingProcessPtr);
	}
	else if (sm->unkB2 == 8)
	{
		UInt32 obj = *(UInt32*)((char*)sm->unkAC + 0x20);
		fn00C8D180((NiCamera*)cullingProcess->localMapCamera.niCamera, (NiAVObject*)obj, cullingProcessPtr);
	}

	BSRenderTargetGroup * renderedTargetGroup = NiRenderManager::GetSingleton()->CreateRenderTarget2(NiDX9Renderer::GetSingleton2(), ini.Width, ini.Height, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	typedef UInt32(*Fn00C90850)(UInt32, NiRenderTargetGroup*);
	const Fn00C90850 fn00C90850 = (Fn00C90850)0x00C90850;
	fn00C90850(0x7, renderedTargetGroup->GetNiRenderTargetGroup());

	typedef UInt32(*Fn00C8DA90)(NiCamera*, BSShaderAccumulator*, UInt32);
	const Fn00C8DA90 fn00C8DA90 = (Fn00C8DA90)0x00C8DA90;
	isProcessing = true;
	if (!useOriginal || *useOriginal == 1)
	{
		fn00C8DA90((NiCamera*)cullingProcess->localMapCamera.niCamera, cullingProcess->shaderAccumulator, 0);
	}
	else
	{
		*useOriginal = 1;
		fn00C8DA90((NiCamera*)cullingProcess->localMapCamera.niCamera, cullingProcess->shaderAccumulator, 0);
		*useOriginal = 0;
	}
	isProcessing = false;

	//typedef void(*Fn00C70D50)(UInt32);
	//const Fn00C70D50 fn00C70D50 = (Fn00C70D50)0x00C70D50;
	//fn00C70D50(0x13);
	//typedef UInt32(*Fn00C8D180)(NiCamera*, NiNode*, BSCullingProcess*);
	//const Fn00C8D180 fn00C8D180 = (Fn00C8D180)0x00C8D180;
	//fn00C8D180(niCamera, cullingProcess->niNode, process);
	typedef UInt32(*Fn00C906B0)();
	const Fn00C906B0 fn00C906B0 = (Fn00C906B0)0x00C906B0;
	fn00C906B0();

	NiRenderManager::GetSingleton2()->Fn00C7EEE0(0x21, NiDX9Renderer::GetSingleton2(), renderedTargetGroup->renderedTexture[0], cullingProcess->localMapTexture, nullptr, 1);

	NiRenderManager::GetSingleton()->Fn00C920C0(renderedTargetGroup);

	if (visible == 1)
		visible = 2;
}

EventResult MiniMapMenu::ReceiveEvent(InputEvent ** evns, BSTEventSource<InputEvent*> * src)
{
	if (!*evns)
		return kEvent_Continue;

	MenuManager* mm = MenuManager::GetSingleton();
	if (!mm || mm->numPauseGame != 0 || mm->numStopCrosshairUpdate != 0)
		return kEvent_Continue;

	for (InputEvent * e = *evns; e; e = e->next)
	{
		if (e->eventType == InputEvent::kEventType_Button)
		{
			ButtonEvent * t = DYNAMIC_CAST<ButtonEvent*>(e);
			if (t->IsDown())
			{
				UInt32	keyCode;
				UInt32	deviceType = t->deviceType;
				UInt32	keyMask = t->keyMask;

				if (deviceType == kDeviceType_Mouse)
					keyCode = InputMap::kMacro_MouseButtonOffset + keyMask;
				else if (deviceType == kDeviceType_Gamepad)
					keyCode = InputMap::GamepadMaskToKeycode(keyMask);
				else
					keyCode = keyMask;

				if (keyCode >= InputMap::kMaxMacros)
					continue;

				if (keyCode == ini.ToggleVisibilityKeyCode)
					bHide = !bHide;

				if (keyCode == ini.ToggleZoomKeyCode && visible == 2)
					bZoom = !bZoom;
			}
		}
	}

	return kEvent_Continue;
}

void MiniMapMenu::OnMenuOpen()
{
	BSSpinLockGuard guard(ms_lock);

	if (!ms_pSingleton)
	{
		InputManager *im = InputManager::GetSingleton();
		im->AddEventSink(this);

		((NiRenderedTexture*)&texture.renderedLocalMapTexture)->UpdateVirtualImage(cullingProcess->localMapTexture->renderedTexture[0]);

		if (view)
		{
			GFxValue args[9];
			args[0] = (double)ini.Width;
			args[1] = (double)ini.Height;
			args[2] = (double)ini.Alpha;
			args[3] = (double)ini.Align;
			args[4] = (double)ini.OffsetX;
			args[5] = (double)ini.OffsetY;
			args[6] = (double)ini.FrameShape;
			args[7] = (double)ini.FrameColor;
			args[8] = ini.HideFrame;

			view->Invoke("_root.widget.Setup", nullptr, args, 9);
		}

		ms_pSingleton = this;
	}
}

void MiniMapMenu::OnMenuClose()
{
	BSSpinLockGuard guard(ms_lock);

	if (ms_pSingleton)
	{
		ms_pSingleton = nullptr;

		InputManager *im = InputManager::GetSingleton();
		im->RemoveEventSink(this);

		if (view)
			view->Invoke("_root.widget.Unload", nullptr, nullptr, 0);

		((NiRenderedTexture*)&texture.renderedLocalMapTexture)->ReleaseVirtualImage();
		//((NiRenderedTexture*)&texture.renderedLocalMapTexture)->ReleaseVirtualImage2();
	}

	AutoOpenMiniMapMenuSink::Register();
}

bool MiniMapMenu::UpdateCamera()
{
	LoadedAreaBoundEx* loadedbound = (LoadedAreaBoundEx*)g_TES->loadedAreaBound;
	TESObjectCELL* cell = g_thePlayer->GetParentCell();
	GridCellArray* arr = g_TES->gridCellArray;
	if (!loadedbound || !cell || !arr)
		return false;

	LoadedAreaBoundEx::NiBounds bounds;
	if (!loadedbound->Fn005853C0(cell, arr, &bounds))
		return false;

	bounds.boundsMax.pos *= *units;
	bounds.boundsMin.pos *= *units;

	static float height = 40000.0f;
	static float margin = 2000.0f;

	float zoomPercent = bZoom ? ini.ZoomPercent2 : ini.ZoomPercent1;

	LocalMapCamera* camera = &cullingProcess->localMapCamera;
	camera->areaBoundsMax = bounds.boundsMax.pos;
	camera->areaBoundsMin = bounds.boundsMin.pos;
	//camera->areaBoundsMax = loadedbound->boundsMax;
	//camera->areaBoundsMin = loadedbound->boundsMin;
	if (cell->IsInterior())
	{
		camera->areaBoundsMax.x += margin;
		camera->areaBoundsMax.y += margin;
		camera->areaBoundsMax.z += height * 2;
		camera->areaBoundsMin.x -= margin;
		camera->areaBoundsMin.y -= margin;
	}
	else
	{
		camera->areaBoundsMax.z += height;
	}

	camera->defaultState->zoomPercent = zoomPercent;
	camera->defaultState->someBoundMax = g_thePlayer->pos;
	camera->defaultState->someBoundMax.z += height;
	camera->defaultState->someBoundMin.x = camera->defaultState->someBoundMin.y = camera->defaultState->someBoundMin.z = 0.0f;
	//camera->SetNorthRotation(2.883192f);
	camera->Update();
	if (camera->defaultState->someBoundMin.x != 0.0f || camera->defaultState->someBoundMin.y != 0.0f)
	{
		float diff = max(std::fabs(camera->defaultState->someBoundMin.x), std::fabs(camera->defaultState->someBoundMin.y));
		diff /= zoomPercent;
		camera->areaBoundsMax.x += diff;
		camera->areaBoundsMax.y += diff;
		camera->areaBoundsMin.x -= diff;
		camera->areaBoundsMin.y -= diff;
		camera->defaultState->someBoundMin.x = camera->defaultState->someBoundMin.y = camera->defaultState->someBoundMin.z = 0.0f;
		//camera->SetNorthRotation(2.883192f);
		camera->Update();
	}

	return true;
}