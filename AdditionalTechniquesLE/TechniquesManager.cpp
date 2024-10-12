#include "iniSettings.h"
#include "TechniquesManager.h"
#include "utils.h"
#include "InputMap.h"

#include <SKSE/DebugLog.h>
#include <SKSE/GameRTTI.h>
#include <SKSE/HookUtil.h>
#include <SKSE/GameReferences.h>
#include <SKSE/GameCamera.h>
#include <SKSE/GameEvents.h>
#include <SKSE/GameInput.h>
#include <SKSE/GameObjects.h>

#include <map>
#define _USE_MATH_DEFINES
#include <math.h>

EnemyHealth * enemyHealth = nullptr;
UInt32 stackCountHM = 0;
UInt32 stackCountHA = 0;
UInt32 mode_Arrow = Mode_Arrow::Mode_Default;

typedef void(*FnPlayImpactEffect)(TESObjectCELL*, float, const char*, NiPoint3*, NiPoint3*, float, UInt32, UInt32);
const FnPlayImpactEffect fnPlayImpactEffect = (FnPlayImpactEffect)0x005F07C0;

static bool IsKeyboardPressed(UInt32 keyCode)
{
	InputManager* im = InputManager::GetSingleton();
	if (im && im->keyboard && im->keyboard->IsPressed(keyCode))
		return true;

	return false;
}

static bool IsMousePressed(UInt32 keyCode)
{
	InputManager* im = InputManager::GetSingleton();
	if (im && im->mouse && im->mouse->IsPressed(keyCode))
		return true;

	return false;
}

static bool IsGamepadPressed(UInt32 keyCode)
{
	InputManager* im = InputManager::GetSingleton();
	if (im && im->gamepad && im->gamepad->IsPressed(keyCode))
		return true;

	return false;
}

static void SetRotationMatrix(NiMatrix3* mat, double sacb, double cacb, double sb)
{
	double cb = sqrt(1 - sb * sb);
	double ca = cacb / cb, sa = sacb / cb;
	mat->SetEntry(0, 0, ca);
	mat->SetEntry(0, 1, -sacb);
	mat->SetEntry(0, 2, sa * sb);
	mat->SetEntry(1, 0, sa);
	mat->SetEntry(1, 1, cacb);
	mat->SetEntry(1, 2, -ca * sb);
	mat->SetEntry(2, 0, 0.0);
	mat->SetEntry(2, 1, sb);
	mat->SetEntry(2, 2, cb);
}

struct ProjectileData
{
	TESObjectREFR* self;
	Actor* target;
	UInt32 targetID;
	UInt32 state;
	UInt32 mode_A;
	bool playEffectFA;
	bool playEffectPA;
	NiPoint3 velocity;
	double speed;

	ProjectileData(TESObjectREFR* self) :self(self), target(nullptr), targetID(0), state(0), mode_A(mode_Arrow), playEffectFA(false), playEffectPA(false)
	{
		NiPoint3* velocity = (NiPoint3*)((char*)self + 0xA0);
		this->velocity.x = velocity->x, this->velocity.y = velocity->y, this->velocity.z = velocity->z;
		speed = sqrt(velocity->x * velocity->x + velocity->y * velocity->y + velocity->z * velocity->z);
	}
};

typedef bool(*FnIsKeyPressed)(UInt32);
static FnIsKeyPressed fnIsKeyPressedHM = IsKeyboardPressed;
static FnIsKeyPressed fnIsKeyPressedHA = IsKeyboardPressed;
static UInt32 stackKeyCodeHM = 57;
static UInt32 stackKeyCodeHA = 57;
static bool isStackKeyPressedHM = false;
static bool isStackKeyPressedHA = false;
static bool isUpdate = false;
static std::map<UInt32, ProjectileData> ProjectileMap;
static BSSpinLock ProjectileLock;
static std::vector<double> angleList;
static double currentAngle = 360.0;
static std::unordered_map<UInt32, double> angleMap;

struct UpdateEvent
{
	static BSTEventSource<UpdateEvent>& eventSource;
};
BSTEventSource<UpdateEvent>& UpdateEvent::eventSource = *(BSTEventSource<UpdateEvent>*)0x01B33490;

class UpdateEventHandler : public BSTEventSink <UpdateEvent>
{
public:
	virtual EventResult ReceiveEvent(UpdateEvent * evn, BSTEventSource<UpdateEvent> * src)
	{
		BSSpinLockGuard guard(ProjectileLock);

		bool releaseHM = false;
		if (isStackKeyPressedHM && !fnIsKeyPressedHM(stackKeyCodeHM))
		{
			releaseHM = true;
			isStackKeyPressedHM = false;
		}

		bool releaseHA = false;
		if (isStackKeyPressedHA && !fnIsKeyPressedHA(stackKeyCodeHA))
		{
			releaseHA = true;
			isStackKeyPressedHA = false;
		}

		auto it = ProjectileMap.begin();
		while (it != ProjectileMap.end())
		{
			TESForm* form = LookupFormByID(it->first);
			if (form == it->second.self && (form->formType == FormType::Missile || (form->formType == FormType::Arrow && *(SInt32*)((char*)it->second.self + 0xC0) >= 0)) && it->second.self->Is3DLoaded())
			{
				if (releaseHM && it->second.state == 1)
				{
					stackCountHM--;
					NiPoint3* velocity = (NiPoint3*)((char*)it->second.self + 0xA0);
					velocity->x = it->second.velocity.x;
					velocity->y = it->second.velocity.y;
					velocity->z = it->second.velocity.z;
					it->second.state = 3;
				}
				else if (releaseHA && it->second.state == 2)
				{
					stackCountHA--;
					NiPoint3* velocity = (NiPoint3*)((char*)it->second.self + 0xA0);
					velocity->x = it->second.velocity.x;
					velocity->y = it->second.velocity.y;
					velocity->z = it->second.velocity.z;
					it->second.state = 3;
				}

				it++;
			}
			else
			{
				if (it->second.state == 1)
					stackCountHM--;
				else if (it->second.state == 2)
					stackCountHA--;
				ProjectileMap.erase(it++);
			}
		}

		if (ProjectileMap.empty() && isUpdate)
		{
			isUpdate = false;
			isStackKeyPressedHM = false;
			isStackKeyPressedHA = false;
			stackCountHM = 0;
			stackCountHA = 0;
			UpdateEvent::eventSource.RemoveEventSink(this);
		}

		return kEvent_Continue;
	}
};
UpdateEventHandler	g_updateEventHandler;

static ProjectileData &GetProjectileData(TESObjectREFR* projectile)
{
	if (!isUpdate)
	{
		isUpdate = true;
		UpdateEvent::eventSource.AddEventSink(&g_updateEventHandler);
	}

	auto it = ProjectileMap.find(projectile->formID);
	if (it != ProjectileMap.end())
	{
		if (it->second.self != projectile)
		{
			it->second.self = projectile;
			it->second.target = nullptr;
			it->second.targetID = 0;
			if (it->second.state == 1)
				stackCountHM--;
			else if (it->second.state == 2)
				stackCountHA--;
			it->second.state = 0;
			it->second.mode_A = mode_Arrow;
			it->second.playEffectFA = false;
			it->second.playEffectPA = false;
			NiPoint3* velocity = (NiPoint3*)((char*)projectile + 0xA0);
			it->second.velocity.x = velocity->x, it->second.velocity.y = velocity->y, it->second.velocity.z = velocity->z;
			it->second.speed = sqrt(velocity->x * velocity->x + velocity->y * velocity->y + velocity->z * velocity->z);
		}

		return it->second;
	}
	else
	{
		return ProjectileMap.insert(std::make_pair(projectile->formID, ProjectileData(projectile))).first->second;
	}
}

static void UpdateTarget(ProjectileData &data)
{
	Actor* target = nullptr;
	if (enemyHealth)
	{
		RefHandle handle = enemyHealth->handle;
		if (handle != g_invalidRefHandle)
		{
			TESObjectREFRPtr refPtr;
			if (TESObjectREFR::LookupByHandle(handle, refPtr) && refPtr->formType == FormType::Character)
			{
				Actor* actor = static_cast<Actor*>((TESForm*)refPtr);
				NiAVObject* object = Utils::GetTorsoObject(actor);
				if (object && g_thePlayer->HasLOS(actor) && !actor->IsDead(true) && actor->GetActorValueCurrent(54) <= 0.0f && !actor->IsGhost())
				{
					NiPoint3 screenPos(-1.0f, -1.0f, -1.0f);
					NiPoint3 worldPos(object->m_worldTransform.pos.x, object->m_worldTransform.pos.y, object->m_worldTransform.pos.z);
					Utils::WorldPtToScreenPt3(worldPos, screenPos);
					if (screenPos.z > 0.0f && screenPos.x >= 0.0f && screenPos.x <= 1.0f && screenPos.y >= 0.0f && screenPos.y <= 1.0f)
						target = actor;
				}
			}
		}
	}

	if (target)
	{
		data.target = target;
		data.targetID = target->formID;
	}
	else
	{
		data.target = nullptr;
		data.targetID = 0;
	}
}

class MissileProjectileEx : public MissileProjectile
{
public:
	typedef void(MissileProjectileEx::*FnGetVelocity)(NiPoint3* velocity);

	static FnGetVelocity fnGetVelocityOld;

	void GetVelocity_Hook(NiPoint3* velocity)
	{
		if (*(UInt32*)((char*)this + 0xB8) == 0x100000 && Is3DLoaded())
		{
			BSSpinLockGuard guard(ProjectileLock);

			ProjectileData &data = GetProjectileData(this);
			NiPoint3* velocity = (NiPoint3*)((char*)this + 0xA0);

			PlayerCamera* Camera = PlayerCamera::GetSingleton();
			if (Camera && Camera->cameraState != Camera->cameraStates[PlayerCamera::kCameraState_VATS])
			{
				if (data.state != 3)
				{
					if (data.state == 1)
					{
						velocity->x = 0.0f;
						velocity->y = 0.0f;
						velocity->z = 0.0f;
						if (stackCountHM >= ini.RequiredStackNumberHM)
							UpdateTarget(data);
					}
					else
					{
						if (Utils::PlayerHasPerk(ini.RequiredPerkHM))
						{
							if (fnIsKeyPressedHM(stackKeyCodeHM) && stackCountHM < ini.MaxStackNumberHM)
							{
								velocity->x = 0.0f;
								velocity->y = 0.0f;
								velocity->z = 0.0f;
								data.state = 1;
								stackCountHM++;
								isStackKeyPressedHM = true;
								if (stackCountHM >= ini.RequiredStackNumberHM)
									UpdateTarget(data);
							}
							else
							{
								data.state = 3;
								if (ini.RequiredStackNumberHM == 0)
									UpdateTarget(data);
							}
						}
						else
						{
							data.state = 3;
						}
					}
				}

				if (data.targetID != 0)
				{
					TESForm* form = LookupFormByID(data.targetID);
					if (form && form == data.target && form->formType == FormType::Character)
					{
						Actor* actor = static_cast<Actor*>(form);
						NiAVObject* object = Utils::GetTorsoObject(actor);
						if (object && !actor->IsGhost())
						{
							NiAVObject* node = (NiAVObject*)GetNiNode();
							if (node)
							{
								double dx = object->m_worldTransform.pos.x - pos.x;
								double dy = object->m_worldTransform.pos.y - pos.y;
								double dz = object->m_worldTransform.pos.z - pos.z;
								double tmpVal = sqrt(dx * dx + dy * dy + dz * dz);
								float velocityX = dx / tmpVal;
								float velocityY = dy / tmpVal;
								float velocityZ = dz / tmpVal;
								rot.x = asin(velocityZ);
								rot.z = atan2(velocityX, velocityY);
								if (rot.z < 0.0f)
									rot.z += M_PI;
								if (velocityX < 0.0f)
									rot.z += M_PI;
								SetRotationMatrix(&node->m_localTransform.rot, -velocityX, velocityY, velocityZ);
								data.velocity.x = velocityX * data.speed;
								data.velocity.y = velocityY * data.speed;
								data.velocity.z = velocityZ * data.speed;
								if (data.state != 1)
								{
									velocity->x = data.velocity.x;
									velocity->y = data.velocity.y;
									velocity->z = data.velocity.z;
								}
							}
						}
					}
				}
			}
			else if (data.state == 0)
			{
				data.state = 3;
			}
		}

		(this->*fnGetVelocityOld)(velocity);
	}

	static void InitHook()
	{
		fnGetVelocityOld = SafeWrite32(0x010D8DBC + 0x86 * 4, &GetVelocity_Hook);
	}
};
MissileProjectileEx::FnGetVelocity MissileProjectileEx::fnGetVelocityOld;

class ArrowProjectileEx : public ArrowProjectile
{
public:
	typedef void(ArrowProjectileEx::*FnGetVelocity)(NiPoint3* velocity);

	static FnGetVelocity fnGetVelocityOld;

	void GetVelocity_Hook(NiPoint3* velocity)
	{
		if (*(UInt32*)((char*)this + 0xB8) == 0x100000 && *(SInt32*)((char*)this + 0xC0) >= 0 && Is3DLoaded())
		{
			BSSpinLockGuard guard(ProjectileLock);

			ProjectileData &data = GetProjectileData(this);
			NiPoint3* velocity = (NiPoint3*)((char*)this + 0xA0);
			PlayerCamera* Camera = PlayerCamera::GetSingleton();
			if (Camera && Camera->cameraState != Camera->cameraStates[PlayerCamera::kCameraState_VATS])
			{
				if (data.playEffectFA)
					fnPlayImpactEffect(GetParentCell(), 1.0f, "AdditionalTechniques\\FasterArrow01.nif", &rot, &pos, 1.0f, 7, 0);

				if (data.playEffectPA)
					fnPlayImpactEffect(GetParentCell(), 1.0f, "AdditionalTechniques\\PenetratingArrow01.nif", &rot, &pos, 1.0f, 7, 0);

				if (data.state != 3)
				{
					if (data.state == 2)
					{
						velocity->x = 0.0f;
						velocity->y = 0.0f;
						velocity->z = 0.0f;
						if (stackCountHA >= ini.RequiredStackNumberHA)
							UpdateTarget(data);
					}
					else
					{
						if (!ini.DisableFasterArrow && Utils::PlayerHasPerk(ini.RequiredPerkFA))
						{
							float* drawnMult = (float*)((char*)this + 0xFC);
							if (*drawnMult < 1.0f && *drawnMult >= ini.EffectiveTimePercentageFA)
							{
								*(float*)((char*)this + 0x10C) *= ini.DamageMultFA;
								data.velocity.x = velocity->x *= ini.SpeedMultFA;
								data.velocity.y = velocity->y *= ini.SpeedMultFA;
								data.velocity.z = velocity->z *= ini.SpeedMultFA;
								data.speed *= ini.SpeedMultFA;
								if (ini.PlaySoundFA)
								{
									NiNode* node = g_thePlayer->GetNiNode();
									if (node)
									{
										static BGSSoundDescriptorForm * sound = (BGSSoundDescriptorForm*)TESForm::LookupByID(0x03F2B4);
										if (sound->standardSoundDef)
										{
											UInt32 tmp = sound->standardSoundDef->soundCharacteristics.unk28;
											typedef bool(*FnPlaySound)(BGSSoundDescriptorForm *, UInt32, const NiPoint3 *, NiNode *);
											const FnPlaySound fnPlaySound = (FnPlaySound)0x00653A60;
											sound->standardSoundDef->soundCharacteristics.unk28 = 0;
											fnPlaySound(sound, 0, &g_thePlayer->pos, node);
											sound->standardSoundDef->soundCharacteristics.unk28 = tmp;
										}
									}
								}

								if (ini.PlayEffectFA)
								{
									data.playEffectFA = true;
									fnPlayImpactEffect(GetParentCell(), 1.0f, "AdditionalTechniques\\FasterArrow01.nif", &rot, &pos, 1.0f, 7, 0);
								}
							}
						}

						if (data.mode_A == Mode_Arrow::Mode_Multishot)
						{
							if (Utils::PlayerHasPerk(ini.RequiredPerkMS))
							{
								auto it = angleMap.find(formID);
								if (it != angleMap.end())
								{
									*(float*)((char*)this + 0x10C) *= ini.DamageMultMS;
									if (it->second != 0.0)
									{
										NiAVObject* node = (NiAVObject*)GetNiNode();
										if (node && data.speed > 0.0)
										{
											double base = rot.z * 180.0 / M_PI;
											rot.z = (base + it->second) * M_PI / 180.0;
											double rotZ = (-base + 90.0 - it->second) * M_PI / 180.0;
											double hypotenuse = sqrt(velocity->x * velocity->x + velocity->y * velocity->y);
											data.velocity.x = velocity->x = cos(rotZ) * hypotenuse;
											data.velocity.y = velocity->y = sin(rotZ) * hypotenuse;
											SetRotationMatrix(&node->m_localTransform.rot, -(data.velocity.x / data.speed), data.velocity.y / data.speed, data.velocity.z / data.speed);
										}
									}

									angleMap.erase(it);
								}
							}
							else
							{
								data.mode_A = Mode_Arrow::Mode_Default;
							}

							data.state = 3;
						}
						else if (data.mode_A == Mode_Arrow::Mode_Penetrating)
						{
							if (Utils::PlayerHasPerk(ini.RequiredPerkPA))
							{
								*(float*)((char*)this + 0x10C) *= ini.DamageMultPA;
								if (ini.PlayEffectPA)
								{
									data.playEffectPA = true;
									fnPlayImpactEffect(GetParentCell(), 1.0f, "AdditionalTechniques\\PenetratingArrow01.nif", &rot, &pos, 1.0f, 7, 0);
								}
							}
							else
							{
								data.mode_A = Mode_Arrow::Mode_Default;
							}

							data.state = 3;
						}
						else
						{
							if (!ini.DisableHomingArrow && Utils::PlayerHasPerk(ini.RequiredPerkHA))
							{
								if (fnIsKeyPressedHA(stackKeyCodeHA) && stackCountHA < ini.MaxStackNumberHA)
								{
									velocity->x = 0.0f;
									velocity->y = 0.0f;
									velocity->z = 0.0f;
									data.state = 2;
									stackCountHA++;
									isStackKeyPressedHA = true;
									if (stackCountHA >= ini.RequiredStackNumberHA)
										UpdateTarget(data);
								}
								else
								{
									data.state = 3;
									if (ini.RequiredStackNumberHA == 0)
										UpdateTarget(data);
								}
							}
							else
							{
								data.state = 3;
							}
						}
					}
				}

				if (data.targetID != 0)
				{
					TESForm* form = LookupFormByID(data.targetID);
					if (form && form == data.target && form->formType == FormType::Character)
					{
						Actor* actor = static_cast<Actor*>(form);
						NiAVObject* object = Utils::GetTorsoObject(actor);
						if (object && !actor->IsGhost())
						{
							NiAVObject* node = (NiAVObject*)GetNiNode();
							if (node)
							{
								double dx = object->m_worldTransform.pos.x - pos.x;
								double dy = object->m_worldTransform.pos.y - pos.y;
								double dz = object->m_worldTransform.pos.z - pos.z;
								double tmpVal = sqrt(dx * dx + dy * dy + dz * dz);
								float velocityX = dx / tmpVal;
								float velocityY = dy / tmpVal;
								float velocityZ = dz / tmpVal;
								rot.x = asin(velocityZ);
								rot.z = atan2(velocityX, velocityY);
								if (rot.z < 0.0f)
									rot.z += M_PI;
								if (velocityX < 0.0f)
									rot.z += M_PI;
								SetRotationMatrix(&node->m_localTransform.rot, -velocityX, velocityY, velocityZ);
								data.velocity.x = velocityX * data.speed;
								data.velocity.y = velocityY * data.speed;
								data.velocity.z = velocityZ * data.speed;
								if (data.state != 2)
								{
									velocity->x = data.velocity.x;
									velocity->y = data.velocity.y;
									velocity->z = data.velocity.z;
								}
							}
						}
					}
				}
			}
			else if (data.state == 0)
			{
				data.mode_A = Mode_Arrow::Mode_Default;
				data.state = 3;
			}
		}

		(this->*fnGetVelocityOld)(velocity);
	}

	static void InitHook()
	{
		fnGetVelocityOld = SafeWrite32(0x010D702C + 0x86 * 4, &GetVelocity_Hook);
	}
};
ArrowProjectileEx::FnGetVelocity ArrowProjectileEx::fnGetVelocityOld;

//struct SneakHandler : public PlayerInputHandler
//{
//public:
//	virtual ~SneakHandler();
//
//	// @override
//	virtual	void	ProcessButton(ButtonEvent *evn, PlayerControls::Data14 *arg2) override;
//};
//
//class SneakHandlerEx : public SneakHandler
//{
//public:
//	typedef void(SneakHandlerEx::*FnProcessButton)(ButtonEvent* evn, PlayerControls::Data14 *data);
//
//	static FnProcessButton fnProcessButtonOld;
//
//	void ProcessButton_Hook(ButtonEvent* evn, PlayerControls::Data14 *data)
//	{
//		static bool bProcessLongTap = false;
//
//		if (!evn->IsDown())
//		{
//			if (bProcessLongTap && evn->timer > 2.0f)
//			{
//				bProcessLongTap = false;
//				typedef void(*FnDebug_Notification)(const char *, bool, bool);
//				const FnDebug_Notification fnDebug_Notification = (FnDebug_Notification)0x008997A0;
//				UInt32 mode = mode_Arrow;
//				while (++mode < Mode_Arrow::Mode_Num)
//				{
//					if (mode == Mode_Arrow::Mode_Multishot)
//					{
//						if (!ini.DisableMultishot && Utils::PlayerHasPerk(ini.RequiredPerkMS))
//						{
//							mode_Arrow = Mode_Arrow::Mode_Multishot;
//							fnDebug_Notification("Changed to Multishot.", false, true);
//							return;
//						}
//					}
//					else if (mode == Mode_Arrow::Mode_Penetrating)
//					{
//						if (!ini.DisablePenetratingArrow && Utils::PlayerHasPerk(ini.RequiredPerkPA))
//						{
//							mode_Arrow = Mode_Arrow::Mode_Penetrating;
//							fnDebug_Notification("Changed to Penetrating Arrow.", false, true);
//							return;
//						}
//					}
//				}
//
//				if (mode_Arrow != Mode_Arrow::Mode_Default)
//				{
//					mode_Arrow = Mode_Arrow::Mode_Default;
//					fnDebug_Notification("Changed to Default Arrow.", false, true);
//				}
//			}
//
//			return;
//		}
//
//		bProcessLongTap = true;
//		(this->*fnProcessButtonOld)(evn, data);
//	}
//
//	static void InitHook()
//	{
//		fnProcessButtonOld = SafeWrite32(0x010D4644 + 0x04 * 4, &ProcessButton_Hook);
//	}
//};
//SneakHandlerEx::FnProcessButton SneakHandlerEx::fnProcessButtonOld;

enum
{
	kDeviceType_Keyboard = 0,
	kDeviceType_Mouse,
	kDeviceType_Gamepad
};

class InputEventHandler : public BSTEventSink<InputEvent*>
{
public:
	virtual EventResult ReceiveEvent(InputEvent ** evns, BSTEventSource<InputEvent*> * src)
	{
		if (!*evns)
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

					if (keyCode == ini.DefaultArrowKeyCode || keyCode == ini.KeyCodeMS || keyCode == ini.KeyCodePA)
					{
						if ((g_thePlayer->flags04 & 0x80000000) == 0 && !MenuManager::IsInMenuMode())
						{
							typedef void(*FnDebug_Notification)(const char *, bool, bool);
							const FnDebug_Notification fnDebug_Notification = (FnDebug_Notification)0x008997A0;
							UInt32 mode = mode_Arrow;
							int i = 0;
							while (++i < Mode_Arrow::Mode_Num)
							{
								if (++mode >= Mode_Arrow::Mode_Num)
									mode = Mode_Arrow::Mode_Default;

								if (mode == Mode_Arrow::Mode_Default)
								{
									if (keyCode == ini.DefaultArrowKeyCode)
									{
										mode_Arrow = Mode_Arrow::Mode_Default;
										fnDebug_Notification("Changed to Default Arrow.", false, true);
										break;
									}
								}
								else if (mode == Mode_Arrow::Mode_Multishot)
								{
									if (keyCode == ini.KeyCodeMS && !ini.DisableMultishot && Utils::PlayerHasPerk(ini.RequiredPerkMS))
									{
										mode_Arrow = Mode_Arrow::Mode_Multishot;
										fnDebug_Notification("Changed to Multishot.", false, true);
										break;
									}
								}
								else if (mode == Mode_Arrow::Mode_Penetrating)
								{
									if (keyCode == ini.KeyCodePA && !ini.DisablePenetratingArrow && Utils::PlayerHasPerk(ini.RequiredPerkPA))
									{
										mode_Arrow = Mode_Arrow::Mode_Penetrating;
										fnDebug_Notification("Changed to Penetrating Arrow.", false, true);
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		return kEvent_Continue;
	}
};
InputEventHandler	g_inputEventHandler;

class GFxValueEx : GFxValue
{
public:
	DEFINE_MEMBER_FN(Fn00848930, void, 0x00848930, const char *, GFxValue*, GFxValue*, UInt32);
	void Hook_00864181(const char * name, GFxValue* pResult, GFxValue* args, UInt32 nargs)
	{
		GFxValue* bArgs2 = (GFxValue*)((char*)args + 0x10);
		GFxValue* sArgs3 = (GFxValue*)((char*)args + 0x20);
		std::string aArrows;
		if (!bArgs2->GetBool())
		{
			if (mode_Arrow == Mode_Arrow::Mode_Multishot)
			{
				if (Utils::PlayerHasPerk(ini.RequiredPerkMS))
				{
					aArrows = std::string(sArgs3->GetString()) + " (Multishot)";
					*sArgs3 = aArrows.c_str();
				}
			}
			else if (mode_Arrow == Mode_Arrow::Mode_Penetrating)
			{
				if (Utils::PlayerHasPerk(ini.RequiredPerkPA))
				{
					aArrows = std::string(sArgs3->GetString()) + " (Penetrating)";
					*sArgs3 = aArrows.c_str();
				}
			}
		}

		Fn00848930(name, pResult, args, nargs);
	}
};

class TESObjectWEAPEx : public TESObjectWEAP
{
public:
	DEFINE_MEMBER_FN(Fn004AA6A0, void, 0x004AA6A0, Actor*, UInt32, UInt32, UInt32);
	void Hook_ReleaseArrow(Actor* actor, UInt32 arg2, UInt32 arg3, UInt32 arg4)
	{
		if (actor == g_thePlayer && mode_Arrow == Mode_Arrow::Mode_Multishot)
		{
			PlayerCamera* Camera = PlayerCamera::GetSingleton();
			if (Camera && Camera->cameraState != Camera->cameraStates[PlayerCamera::kCameraState_VATS])
			{
				BSSpinLockGuard guard(ProjectileLock);

				angleMap.clear();

				if (Utils::PlayerHasPerk(ini.RequiredPerkMS))
				{
					UInt32 tmp71C = *(UInt32*)((char*)actor + 0x71C);
					for (int i = 0; i < angleList.size(); i++)
					{
						*(UInt32*)((char*)actor + 0x71C) = tmp71C;
						currentAngle = angleList[i];
						Fn004AA6A0(actor, arg2, arg3, arg4);
					}

					currentAngle = 360.0;

					return;
				}
			}
		}

		Fn004AA6A0(actor, arg2, arg3, arg4);
	}
};

static bool Hook_004AB316(const RefHandle &refHandle, TESObjectREFR *&refrOut)
{
	bool retn = LookupREFRByHandle(refHandle, refrOut);
	if (retn && currentAngle != 360.0 && *(UInt32*)((char*)refrOut + 0xB8) == 0x100000 && refrOut->formType == FormType::Arrow)
		angleMap.insert(std::make_pair(refrOut->formID, currentAngle));

	return retn;
}

static bool Hook_007992A1(TESObjectREFR* target, UInt32 arg2, Projectile* projectile, BGSProjectile* base)
{
	if (projectile && projectile->Is(FormType::Arrow) && *(UInt32*)((char*)projectile + 0xB8) == 0x100000 && target && target->Is(FormType::Character))
	{
		BSSpinLockGuard guard(ProjectileLock);

		auto it = ProjectileMap.find(projectile->formID);
		if (it != ProjectileMap.end() && it->second.mode_A == Mode_Arrow::Mode_Penetrating)
			return false;
	}

	typedef bool(*Fn0079F010)(TESObjectREFR*, UInt32, Projectile*, BGSProjectile*);
	const Fn0079F010 fn0079F010 = (Fn0079F010)0x0079F010;
	return fn0079F010(target, arg2, projectile, base);
}

class MenuOpenCloseEventHandler : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	virtual EventResult ReceiveEvent(MenuOpenCloseEvent * evn, BSTEventSource<MenuOpenCloseEvent> * src)
	{
		MenuManager *mm = MenuManager::GetSingleton();
		UIStringHolder *holder = UIStringHolder::GetSingleton();

		if (evn->menuName == holder->hudMenu && evn->IsOpen())
		{
			IMenu* menu = mm->GetMenu(holder->hudMenu);
			if (menu)
			{
				HUDMenu* hudMenu = static_cast<HUDMenu*>(menu);
				for (HUDObject* object : hudMenu->hudComponents)
				{
					if (object)
					{
						EnemyHealth* eh = DYNAMIC_CAST<EnemyHealth*>(object);
						if (eh)
						{
							enemyHealth = eh;
							break;
						}
					}
				}
			}

			mm->BSTEventSource<MenuOpenCloseEvent>::RemoveEventSink(this);
		}

		return kEvent_Continue;
	}
};
MenuOpenCloseEventHandler	g_menuOpenCloseEventHandler;

void TechniquesManager::TechniquesMessageHandler(SKSEMessagingInterface::Message *msg)
{
	if (msg->type == SKSEMessagingInterface::kMessage_PostLoadGame)
	{
		BSSpinLockGuard guard(ProjectileLock);

		stackCountHM = 0;
		stackCountHA = 0;
		ProjectileMap.clear();
		angleMap.clear();
		mode_Arrow = Mode_Arrow::Mode_Default;
	}
}

void TechniquesManager::Init()
{
	if (!ini.DisableHomingMissile)
	{
		if (ini.StackKeyCodeHM < 256)
		{
			fnIsKeyPressedHM = IsKeyboardPressed;
			stackKeyCodeHM = ini.StackKeyCodeHM;
		}
		else if (ini.StackKeyCodeHM < 266)
		{
			fnIsKeyPressedHM = IsMousePressed;
			stackKeyCodeHM = ini.StackKeyCodeHM - InputMap::kMacro_MouseButtonOffset;
		}
		else
		{
			fnIsKeyPressedHM = IsGamepadPressed;
			stackKeyCodeHM = InputMap::GamepadKeycodeToMask(ini.StackKeyCodeHM);
		}

		MissileProjectileEx::InitHook();
	}

	if (!ini.DisableHomingArrow)
	{
		if (ini.StackKeyCodeHA < 256)
		{
			fnIsKeyPressedHA = IsKeyboardPressed;
			stackKeyCodeHA = ini.StackKeyCodeHA;
		}
		else if (ini.StackKeyCodeHA < 266)
		{
			fnIsKeyPressedHA = IsMousePressed;
			stackKeyCodeHA = ini.StackKeyCodeHA - InputMap::kMacro_MouseButtonOffset;
		}
		else
		{
			fnIsKeyPressedHA = IsGamepadPressed;
			stackKeyCodeHA = InputMap::GamepadKeycodeToMask(ini.StackKeyCodeHA);
		}
	}

	if (!ini.DisableHomingMissile || !ini.DisableHomingArrow)
	{
		MenuManager *mm = MenuManager::GetSingleton();
		mm->BSTEventSource<MenuOpenCloseEvent>::AddEventSink(&g_menuOpenCloseEventHandler);
	}

	if (!ini.DisableMultishot)
	{
		double angle = 0.0;
		UInt32 count = ini.NumberMS;
		if ((count & 1) == 0)
			angle = ini.AngleMS / 2.0;

		angleList.push_back(angle);
		count--;
		while (count > 0)
		{
			angle = -angle;
			if ((count & 1) == 0)
				angle += ini.AngleMS;

			angleList.push_back(angle);
			count--;
		}

		WriteRelCall(0x00522BB4, &TESObjectWEAPEx::Hook_ReleaseArrow);
		WriteRelCall(0x006A1332, &TESObjectWEAPEx::Hook_ReleaseArrow);
		WriteRelCall(0x00780E16, &TESObjectWEAPEx::Hook_ReleaseArrow);
		WriteRelCall(0x00914A1F, &TESObjectWEAPEx::Hook_ReleaseArrow);
		WriteRelCall(0x004AB316, &Hook_004AB316);
	}

	if (!ini.DisablePenetratingArrow)
	{
		WriteRelCall(0x007992A1, &Hook_007992A1);
	}

	if (!ini.DisableHomingArrow || !ini.DisableFasterArrow || !ini.DisableMultishot || !ini.DisablePenetratingArrow)
	{
		ArrowProjectileEx::InitHook();
	}

	if (!ini.DisableMultishot || !ini.DisablePenetratingArrow)
	{
		//SneakHandlerEx::InitHook();
		InputManager *im = InputManager::GetSingleton();
		im->AddEventSink(&g_inputEventHandler);
		WriteRelCall(0x00864181, &GFxValueEx::Hook_00864181);
	}
}