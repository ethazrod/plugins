#include "HookDamage.h"
#include "iniSettings.h"
#include "FloatingDamage.h"

#include <SKSE/PluginAPI.h>
#include <SKSE/DebugLog.h>
#include <SKSE/SafeWrite.h>
#include <SKSE/HookUtil.h>
#include <SKSE/GameRTTI.h>
#include <SKSE/GameForms.h>
#include <SKSE/GameReferences.h>
#include <SKSE/GameObjects.h>
#include <SKSE/GameCamera.h>
#include <Skyrim/Magic/EffectSetting.h>
#include <SKSE/Scaleform.h>

#include <random>

static bool WorldPtToScreenPt3(const NiPoint3 &in, NiPoint3 &out, float zeroTolerance = 1e-5f)
{
	float *worldToCamMatrix = (float*)0x001B3EA10;
	static NiRect<float> viewPort = { 0.0f, 1.0f, 0.0f, 1.0f };

	typedef bool(*FnWorldPtToScreenPt3)(const float *worldToCamMatrix, const NiRect<float> *port, const NiPoint3 &in, float &x_out, float &y_out, float &z_out, float zeroTolerance);
	const FnWorldPtToScreenPt3 fnWorldPtToScreenPt3 = (FnWorldPtToScreenPt3)0x00AB84C0;

	return fnWorldPtToScreenPt3(worldToCamMatrix, &viewPort, in, out.x, out.y, out.z, zeroTolerance);
}

static bool IsValidActor(Actor* actor)
{
	return actor->Is3DLoaded() && !actor->IsDead(true) && actor->GetActorValueCurrent(24) > 0.0f;
}

static float CalcDamage(Actor* actor, float damage, UInt32 arg3)
{
	return -(actor->Fn006AAEB0(-damage, arg3));
}

static bool IsAlly(Actor* actor)
{
	if (actor->IsPlayerTeammate())
		return true;

	RefHandle handle;
	actor->Fn006A8EC0(&handle);
	if (handle != g_invalidRefHandle)
	{
		TESObjectREFRPtr refPtr;
		if (TESObjectREFR::LookupByHandle(handle, refPtr) && refPtr->formType == FormType::Character)
		{
			Actor* commander = static_cast<Actor*>((TESForm*)refPtr);
			if (commander == g_thePlayer || commander->IsPlayerTeammate())
				return true;
		}
	}

	return false;
}

static void GetDispersion(NiPoint3& move)
{
	std::mt19937 mt(std::random_device{}());
	std::uniform_real_distribution<> score(-10.0, 10.0);
	move.x = score(mt);
	move.y = score(mt);
	move.z = score(mt);
}

static void DamageHealEvent(Actor* target, TESObjectREFR* attacker, int number, NiPoint3& pos, HitType flags, bool dispersion)
{
	bool isPlayer = target == g_thePlayer;
	if (!isPlayer)
	{
		TESObjectCELL* cell = g_thePlayer->GetParentCell();
		if (cell && cell->IsInterior() && cell != target->GetParentCell())
			return;

		if ((flags & HitType::Hit_Heal) != 0)
		{
			if (ini.HealNonPlayerAlpha == 0 && attacker != g_thePlayer)
				return;
		}
		else
		{
			if (ini.DamageNonPlayerAlpha == 0 && attacker != g_thePlayer)
				return;
		}

		if (ini.HideDistance != 0 && attacker != g_thePlayer)
		{
			double dx = target->pos.x - g_thePlayer->pos.x;
			double dy = target->pos.y - g_thePlayer->pos.y;
			double dz = target->pos.z - g_thePlayer->pos.z;
			double d2 = dx * dx + dy * dy + dz * dz;
			if (d2 > ini.HideDistance * ini.HideDistance)
				return;
		}

		if (ini.HideNoLOS && !g_thePlayer->HasLOS(target))
			return;
	}

	bool isFirstPerson = false;
	if (isPlayer)
	{
		PlayerCamera* camera = PlayerCamera::GetSingleton();
		isFirstPerson = camera && camera->IsFirstPerson();
	}

	bool track = (ini.DisplayPosition & 2) != 0;
	UInt32 formId = 0;
	NiPoint3 screenPos;
	if (isFirstPerson)
	{
		screenPos.x = ini.FirstPersonPosX / 100.0f;
		screenPos.y = ini.FirstPersonPosY / 100.0f;
		screenPos.z = 0.5f;
		track = false;
		if ((flags & (HitType::Hit_Heal | HitType::Hit_DoT)) == 0)
			dispersion = true;
	}
	else
	{
		if ((ini.DisplayPosition & 1) != 0)
		{
			target->GetMarkerPosition(&pos);
			if ((flags & (HitType::Hit_Heal | HitType::Hit_DoT)) == 0)
				dispersion = true;

			NiPoint3 move(0.0f, 0.0f, 0.0f);
			if (dispersion)
			{
				GetDispersion(move);
				pos += move;
				dispersion = false;
			}

			WorldPtToScreenPt3(pos, screenPos);
			if (screenPos.z <= 0.0f || screenPos.x < 0.0f || screenPos.x > 1.0f || screenPos.y < 0.0f || screenPos.y > 1.0f)
				return;

			if (track)
			{
				formId = target->formID;
				if (formId == 0)
					return;

				pos = move;
			}
		}
		else
		{
			if (dispersion)
			{
				NiPoint3 move;
				GetDispersion(move);
				pos += move;
			}

			WorldPtToScreenPt3(pos, screenPos);
			if (screenPos.z <= 0.0f || screenPos.x < 0.0f || screenPos.x > 1.0f || screenPos.y < 0.0f || screenPos.y > 1.0f)
			{
				if (!isPlayer)
					return;

				target->GetMarkerPosition(&pos);
				if ((flags & (HitType::Hit_Heal | HitType::Hit_DoT)) == 0)
					dispersion = true;

				if (dispersion)
				{
					NiPoint3 move;
					GetDispersion(move);
					pos += move;
					dispersion = false;
				}

				WorldPtToScreenPt3(pos, screenPos);
				if (screenPos.z <= 0.0f || screenPos.x < 0.0f || screenPos.x > 1.0f || screenPos.y < 0.0f || screenPos.y > 1.0f)
					return;
			}
			else
			{
				dispersion = false;
			}
		}
	}

	UInt32 alpha = 100;
	if (!isPlayer && attacker != g_thePlayer)
	{
		if ((flags & HitType::Hit_Heal) != 0)
			alpha = ini.HealNonPlayerAlpha;
		else
			alpha = ini.DamageNonPlayerAlpha;
	}

	bool isEnemy = !isPlayer && !IsAlly(target);

	FloatingDamage *FloatingDamage = FloatingDamage::GetSingleton();
	if (FloatingDamage)
	{
		const SKSEPlugin *plugin = SKSEPlugin::GetSingleton();
		const SKSETaskInterface *task = plugin->GetInterface(SKSETaskInterface::Version_2);
		if (task)
		{
			if (!track)
			{
				task->AddUITask([FloatingDamage, screenPos, number, alpha, flags, isEnemy, dispersion, track, formId]()-> void {
					FloatingDamage->PopupText(screenPos, number, alpha, flags, isEnemy, dispersion, track, formId);
				});
			}
			else
			{
				task->AddUITask([FloatingDamage, pos, number, alpha, flags, isEnemy, dispersion, track, formId]()-> void {
					FloatingDamage->PopupText(pos, number, alpha, flags, isEnemy, dispersion, track, formId);
				});
			}
		}
	}
}

struct WaterDoTData
{
	float damage;
	UInt32 Time;

	WaterDoTData() : damage(0.0f), Time(0)
	{
	}
};
WaterDoTData WaterDoT;

struct MagicData
{
	float number;
	Actor* target;
	UInt32 targetId;
	TESObjectREFR* attacker;
	UInt32 attackerId;
	UInt32 Time;

	MagicData(float number, Actor* target, UInt32 targetId, TESObjectREFR* attacker, UInt32 attackerId) : number(number),
		target(target), targetId(targetId), attacker(attacker), attackerId(attackerId), Time(1)
	{
	}
};

std::unordered_map<ActiveEffect*, MagicData> DoTMap;
BSSpinLock lock_DoT;
std::unordered_map<ActiveEffect*, MagicData> HealMap;
BSSpinLock lock_Heal;

class ActorEx : public Actor
{
public:
	DEFINE_MEMBER_FN(Fn006AC240, bool, 0x006AC240, float damage, TESObjectREFR* attacker, UInt32 arg3);

	bool Hook_Damage(float damage, TESObjectREFR* attacker, UInt32 arg3)
	{
		if (!MagicTarget_Unk_04() && IsValidActor(this) && !IsInKillMove())
		{
			float dmg = CalcDamage(this, damage, arg3);
			if (dmg > 0.0f)
			{
				NiPoint3 worldPos;
				GetMarkerPosition(&worldPos);
				DamageHealEvent(this, attacker, (int)dmg, worldPos, static_cast<HitType>(0), true);
			}
		}

		return Fn006AC240(damage, attacker, arg3);
	}

	bool Hook_WaterDamage(float damage, TESObjectREFR* attacker, UInt32 arg3)
	{
		if ((Actor*)this == g_thePlayer && !MagicTarget_Unk_04() && IsValidActor(this) && !IsInKillMove())
		{
			float dmg = CalcDamage(this, damage, arg3);
			if (dmg > 0.0f)
			{
				if (WaterDoT.Time == 0)
				{
					WaterDoT.damage = dmg;
					WaterDoT.Time = 1;
				}
				else
				{
					WaterDoT.damage += dmg;
				}
			}
		}

		return Fn006AC240(damage, attacker, arg3);
	}

	bool Hook_MagicDamage(ActiveEffect* activeEffect, float damage, TESObjectREFR* attacker, UInt32 arg3)
	{
		UInt32 flags = 0;

		if (!MagicTarget_Unk_04() && IsValidActor(this) && !IsInKillMove())
		{
			float dmg = CalcDamage(this, damage, arg3);
			if (dmg > 0.0f)
			{
				if (activeEffect->duration > 0.0f)
				{
					BSSpinLockGuard guard(lock_DoT);

					auto it = DoTMap.find(activeEffect);
					if (it == DoTMap.end())
					{
						UInt32 attackerId = 0;
						if (attacker)
							attackerId = attacker->formID;

						DoTMap.insert(std::make_pair(activeEffect, MagicData(dmg, this, formID, attacker, attackerId)));
					}
					else
					{
						it->second.number += dmg;
					}
				}
				else
				{
					NiPoint3 worldPos;
					GetMarkerPosition(&worldPos);
					DamageHealEvent(this, attacker, (int)dmg, worldPos, static_cast<HitType>(flags), true);
				}
			}
		}

		return Fn006AC240(damage, attacker, arg3);
	}

	DEFINE_MEMBER_FN(Fn006E0760, void, 0x006E0760, UInt32 arg1, UInt32 id, float damage, TESObjectREFR* attacker);

	void Hook_ActorValue(UInt32 arg1, UInt32 id, float damage, TESObjectREFR* attacker)
	{
		if (id == 24)
		{
			if (!ini.HideDamageAV)
			{
				if (IsValidActor(this))
				{
					float dmg = -damage;
					if (dmg >= 1.0f)
					{
						NiPoint3 worldPos;
						GetMarkerPosition(&worldPos);
						DamageHealEvent(this, attacker, (int)dmg, worldPos, static_cast<HitType>(0), true);
					}
				}
			}
		}

		Fn006E0760(arg1, id, damage, attacker);
	}

	void Hook_MagicHeal(ActiveEffect* activeEffect, UInt32 arg1, UInt32 id, float heal, TESObjectREFR* attacker)
	{
		if (id == 24)
		{
			Actor* caster = nullptr;
			RefHandle handle = activeEffect->casterRefhandle;
			if (handle != g_invalidRefHandle)
			{
				TESObjectREFRPtr refPtr;
				if (TESObjectREFR::LookupByHandle(handle, refPtr) && refPtr->formType == FormType::Character)
					caster = static_cast<Actor*>((TESForm*)refPtr);
			}

			if (heal > 0.0f && IsValidActor(this))
			{
				if (activeEffect->effect->mgef->properties.archetype == EffectSetting::Properties::Archetype::kArchetype_Absorb)
				{
					if (activeEffect->magicTarget)
						caster = activeEffect->magicTarget->GetMagicTargetActor();
				}

				if (activeEffect->duration > 0.0f)
				{
					BSSpinLockGuard guard(lock_Heal);

					auto it = HealMap.find(activeEffect);
					if (it == HealMap.end())
					{
						UInt32 casterId = 0;
						if (caster)
							casterId = caster->formID;

						HealMap.insert(std::make_pair(activeEffect, MagicData(heal, this, formID, caster, casterId)));
					}
					else
					{
						it->second.number += heal;
					}
				}
				else
				{
					NiPoint3 worldPos;
					GetMarkerPosition(&worldPos);
					DamageHealEvent(this, caster, (int)heal, worldPos, HitType::Hit_Heal, false);
				}
			}
		}

		Fn006E0760(arg1, id, heal, attacker);
	}
};

static bool Hook_PhysicalDamage(UInt32 edi, UInt32* stack)
{
	float* damage = (float*)&stack[1];
	Actor* target = (Actor*)stack[0];
	TESObjectREFR* attacker = (TESObjectREFR*)stack[2];
	UInt32 arg3 = stack[3];

	if (!target->MagicTarget_Unk_04() && IsValidActor(target))
	{
		float dmg = CalcDamage(target, *damage, arg3);
		int damageInt = dmg > 0.0f && !target->IsInKillMove() ? (int)dmg : 0;
		bool dispersion = false;
		NiPoint3 pos(*(float*)((char*)edi + 0x0), *(float*)((char*)edi + 0x4), *(float*)((char*)edi + 0x8));
		if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f)
		{
			target->GetMarkerPosition(&pos);
			dispersion = true;
		}

		UInt32 flags = 0;
		if ((*(UInt32*)((char*)edi + 0x68) & 8) != 0)
			flags |= HitType::Hit_Critical;
		if ((*(UInt32*)((char*)edi + 0x68) & 0x800) != 0)
			flags |= HitType::Hit_Sneak;
		if ((*(UInt16*)((char*)edi + 0x68) & 3) != 0)
			flags |= HitType::Hit_Block;

		DamageHealEvent(target, attacker, damageInt, pos, static_cast<HitType>(flags), dispersion);
	}

	return *damage == 0.0f;
}

static UInt32 addr_006E4A6A = 0x006E4A6A;
static UInt32 addr_006E4E92 = 0x006E4E92;
static UInt32 addr_006AC240 = 0x006AC240;
static UInt32 addr_006E4ABB = 0x006E4ABB;
static UInt32 addr_006E4A99 = 0x006E4A99;

static void __declspec(naked) Hook_006E4A90(void)
{
	__asm
	{
		push	ecx
		lea		eax, dword ptr[esp]
		push	eax
		push	edi
		call	Hook_PhysicalDamage
		pop		edi
		add		esp, 4
		pop		ecx
		test	al, al
		jnz		jmp_label2

		call	[addr_006AC240]
		test	al, al
		jnz		jmp_label1

		jmp		addr_006E4A99

jmp_label1 :
		jmp		addr_006E4ABB

jmp_label2 :
		add		esp, 0xC
		jmp		addr_006E4E92
	}
}

static UInt32 addr_006706E4 = 0x006706E4;
static UInt32 addr_006706D2 = 0x006706D2;

static void __declspec(naked) Hook_006706C9(void)
{
	__asm
	{
		push	esi
		call	ActorEx::Hook_MagicDamage
		test	al, al
		jnz		jmp_label

		jmp		addr_006706D2

jmp_label :
		jmp		addr_006706E4
	}
}

static UInt32 addr_00670738 = 0x00670738;

static void __declspec(naked) Hook_00670769(void)
{
	__asm
	{
		push	esi
		call	ActorEx::Hook_MagicHeal
		jmp		addr_00670738
	}
}

class DamageUpdateEventHandler : public BSTEventSink <UpdateEvent>
{
public:
	virtual EventResult ReceiveEvent(UpdateEvent * evn, BSTEventSource<UpdateEvent> * src)
	{
		MenuManager *mm = MenuManager::GetSingleton();
		if (!mm || mm->numPauseGame == 0)
		{
			if (WaterDoT.Time != 0 && ++(WaterDoT.Time) > ini.DoTInterval)
			{
				if (g_thePlayer->Is3DLoaded())
				{
					NiPoint3 worldPos;
					g_thePlayer->GetMarkerPosition(&worldPos);
					DamageHealEvent(g_thePlayer, nullptr, (int)WaterDoT.damage, worldPos, HitType::Hit_DoT, false);
				}

				WaterDoT.Time = 0;
			}

			{
				BSSpinLockGuard guard(lock_DoT);

				if (!DoTMap.empty())
				{
					auto it = DoTMap.begin();
					while (it != DoTMap.end())
					{
						if (++(it->second.Time) > ini.DoTInterval)
						{
							TESForm* targetForm = LookupFormByID(it->second.targetId);
							if (targetForm && targetForm->formType == FormType::Character && targetForm == it->second.target && it->second.target->Is3DLoaded())
							{
								TESObjectREFR* attacker = nullptr;
								if (it->second.attackerId != 0)
								{
									TESForm* attackerForm = LookupFormByID(it->second.attackerId);
									if (attackerForm && (attackerForm->formType == FormType::Character || attackerForm->formType == FormType::Reference) && attackerForm == it->second.attacker)
										attacker = it->second.attacker;
								}

								NiPoint3 worldPos;
								it->second.target->GetMarkerPosition(&worldPos);
								DamageHealEvent(it->second.target, attacker, (int)it->second.number, worldPos, HitType::Hit_DoT, false);
							}

							DoTMap.erase(it++);
							continue;
						}

						it++;
					}
				}
			}

			{
				BSSpinLockGuard guard(lock_Heal);

				if (!HealMap.empty())
				{
					auto it = HealMap.begin();
					while (it != HealMap.end())
					{
						if (++(it->second.Time) > ini.DoTInterval)
						{
							TESForm* targetForm = LookupFormByID(it->second.targetId);
							if (targetForm && targetForm->formType == FormType::Character && targetForm == it->second.target && it->second.target->Is3DLoaded())
							{
								TESObjectREFR* attacker = nullptr;
								if (it->second.attackerId != 0)
								{
									TESForm* attackerForm = LookupFormByID(it->second.attackerId);
									if (attackerForm && (attackerForm->formType == FormType::Character || attackerForm->formType == FormType::Reference) && attackerForm == it->second.attacker)
										attacker = it->second.attacker;
								}

								NiPoint3 worldPos;
								it->second.target->GetMarkerPosition(&worldPos);
								DamageHealEvent(it->second.target, attacker, (int)it->second.number, worldPos, HitType::Hit_Heal, false);
							}

							HealMap.erase(it++);
							continue;
						}

						it++;
					}
				}
			}
		}

		return kEvent_Continue;
	}
};
DamageUpdateEventHandler	g_damageUpdateEventHandler;

class DamageMenuOpenCloseEventHandler : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	virtual EventResult ReceiveEvent(MenuOpenCloseEvent * evn, BSTEventSource<MenuOpenCloseEvent> * src)
	{
		UIStringHolder *holder = UIStringHolder::GetSingleton();
		if (evn->menuName == holder->loadingMenu && evn->IsOpen())
		{
			WaterDoT.Time = 0;

			{
				BSSpinLockGuard guard(lock_DoT);

				DoTMap.clear();
			}

			{
				BSSpinLockGuard guard(lock_Heal);

				HealMap.clear();
			}
		}

		return kEvent_Continue;
	}
};
DamageMenuOpenCloseEventHandler	g_damageMenuOpenCloseEventHandler;

class SKSEScaleform_WorldPtToScreenPt3FD2 : public GFxFunctionHandler
{
public:
	virtual void	Call(const Params& params)
	{
		if (params.ArgCount >= 4)
		{
			UInt32 formId = params.pArgs[3].GetNumber();
			if (formId == 0)
			{
				NiPoint3 pos(params.pArgs[0].GetNumber(), params.pArgs[1].GetNumber(), params.pArgs[2].GetNumber());
				NiPoint3 screenPos;
				WorldPtToScreenPt3(pos, screenPos);
				params.pMovie->CreateArray(params.pRetVal);
				params.pRetVal->PushBack((double)screenPos.x);
				params.pRetVal->PushBack((double)screenPos.y);
				params.pRetVal->PushBack((double)screenPos.z);
				params.pRetVal->PushBack(true);
			}
			else
			{
				TESForm* form = LookupFormByID(formId);
				if (!form || form->formType != FormType::Character)
				{
					params.pMovie->CreateArray(params.pRetVal);
					params.pRetVal->PushBack((double)0.0);
					params.pRetVal->PushBack((double)0.0);
					params.pRetVal->PushBack((double)0.0);
					params.pRetVal->PushBack(false);
				}
				else
				{
					NiPoint3 move(params.pArgs[0].GetNumber(), params.pArgs[1].GetNumber(), params.pArgs[2].GetNumber());
					NiPoint3 pos;
					static_cast<Actor*>(form)->GetMarkerPosition(&pos);
					pos += move;
					NiPoint3 screenPos;
					WorldPtToScreenPt3(pos, screenPos);
					params.pMovie->CreateArray(params.pRetVal);
					params.pRetVal->PushBack((double)screenPos.x);
					params.pRetVal->PushBack((double)screenPos.y);
					params.pRetVal->PushBack((double)screenPos.z);
					params.pRetVal->PushBack(true);
				}
			}
		}
	}
};

void ScaleformHandlerFD2(GFxMovieView * view, GFxValue * root)
{
	RegisterFunction<SKSEScaleform_WorldPtToScreenPt3FD2>(root, view, "WorldPtToScreenPt3FD2");
}

void HookDamage::Init()
{
	WriteRelJump(0x006E4A64, addr_006E4A6A);
	WriteRelJump(0x006E4A90, &Hook_006E4A90); //PhysicalDamage
	WriteRelJump(0x006706C9, &Hook_006706C9); //MagicDamage
	WriteRelCall(0x006E08D7, &ActorEx::Hook_ActorValue); //ActorValue

	//WriteRelCall(0x006A1736, &ActorEx::Hook_Damage); //?
	//WriteRelCall(0x006AF584, &ActorEx::Hook_Damage); //?
	WriteRelCall(0x006B9D5F, &ActorEx::Hook_Damage); //Falldamage
	WriteRelCall(0x006C7D5F, &ActorEx::Hook_WaterDamage); //WaterDamage
	//WriteRelCall(0x006C83D5, &ActorEx::Hook_Damage); //?
	//WriteRelCall(0x006C98E7, &ActorEx::Hook_Damage); //?
	WriteRelCall(0x006E7355, &ActorEx::Hook_Damage); //ReflectDamage?

	//WriteRelCall(0x00670331, &ActorEx::Hook_ActorValue); //?
	//WriteRelCall(0x00670395, &ActorEx::Hook_ActorValue); //?
	//WriteRelCall(0x006E4C96, &ActorEx::Hook_ActorValue); //kEntryPoint_Adjust_Limb_Damage?

	//WriteRelCall(0x0067066D, &ActorEx::Hook_ActorValue); //kEffectType_Recover?
	WriteRelJump(0x00670769, &Hook_00670769); //MagicHeal

	UpdateEvent::eventSource.AddEventSink(&g_damageUpdateEventHandler);
	MenuManager *mm = MenuManager::GetSingleton();
	mm->BSTEventSource<MenuOpenCloseEvent>::AddEventSink(&g_damageMenuOpenCloseEventHandler);

	if ((ini.DisplayPosition & 2) != 0)
		SKSEScaleformManager::AddCallback(ScaleformHandlerFD2);
}