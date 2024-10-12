#include "Hooks.h"
#include "HookDamage.h"
#include "Addresses.h"
#include "HookUtil.h"

#include "skse64_common/BranchTrampoline.h"
#include "skse64_common/SafeWrite.h"
#include "xbyak/xbyak.h"

namespace Hooks
{
	typedef bool(*FnEvadeDamage1_OLD)();
	FnEvadeDamage1_OLD fnEvadeDamage1_OLD;

	typedef bool(*FnPhysicalDamage_OLD)(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3);
	FnPhysicalDamage_OLD fnPhysicalDamage_OLD;

	typedef bool(*FnMagicDamage_OLD)(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3);
	FnMagicDamage_OLD fnMagicDamage_OLD;

	typedef void(*FnActorValue_OLD)(Actor* target, UInt32 arg1, UInt32 id, float damage, TESObjectREFR* attacker);
	FnActorValue_OLD fnActorValue_OLD;

	typedef bool(*FnFallDamage_OLD)(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3);
	FnFallDamage_OLD fnFallDamage_OLD;

	typedef bool(*FnWaterDamage_OLD)(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3);
	FnWaterDamage_OLD fnWaterDamage_OLD;

	typedef bool(*FnReflectDamage_OLD)(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3);
	FnReflectDamage_OLD fnReflectDamage_OLD;

	typedef void(*FnMagicHeal_OLD)(Actor* target, UInt32 arg1, UInt32 id, float heal, TESObjectREFR* attacker);
	FnMagicHeal_OLD fnMagicHeal_OLD;

	bool EvadeDamage1()
	{
		return fnEvadeDamage1_OLD();
	}

	bool EvadeDamage2(Actor* target, UInt64 r15)
	{
		bool evade = false;

		if (!HookDamage::evadeDamageEventHandlers.empty())
		{
			for (auto it = HookDamage::evadeDamageEventHandlers.rbegin(), ite = HookDamage::evadeDamageEventHandlers.rend(); it != ite; ++it)
			{
				it->second(target, r15, &evade);
			}
		}

		return evade;
	}

	UInt32 PhysicalDamage(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3, UInt64 r15)
	{
		if (!HookDamage::physicalDamageEventHandlers.empty())
		{
			for (auto it = HookDamage::physicalDamageEventHandlers.rbegin(), ite = HookDamage::physicalDamageEventHandlers.rend(); it != ite; ++it)
			{
				it->second(target, &damage, attacker, arg3, r15);
			}
		}

		if (damage == 0.0f)
			return 2;

		if (fnPhysicalDamage_OLD(target, damage, attacker, arg3))
			return 1;

		return 0;
	}

	bool MagicDamage(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3, ActiveEffect* activeEffect)
	{
		if (!HookDamage::magicDamageEventHandlers.empty())
		{
			bool crit = false;

			for (auto it = HookDamage::magicDamageEventHandlers.rbegin(), ite = HookDamage::magicDamageEventHandlers.rend(); it != ite; ++it)
			{
				it->second(target, &damage, attacker, arg3, activeEffect, &crit);
			}
		}

		return fnMagicDamage_OLD(target, damage, attacker, arg3);
	}

	void ActorValue(Actor* target, UInt32 arg1, UInt32 id, float damage, TESObjectREFR* attacker)
	{
		if (!HookDamage::actorValueEventHandlers.empty())
		{
			for (auto it = HookDamage::actorValueEventHandlers.rbegin(), ite = HookDamage::actorValueEventHandlers.rend(); it != ite; ++it)
			{
				it->second(target, arg1, id, &damage, attacker);
			}
		}

		fnActorValue_OLD(target, arg1, id, damage, attacker);
	}

	bool FallDamage(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3)
	{
		if (!HookDamage::fallDamageEventHandlers.empty())
		{
			for (auto it = HookDamage::fallDamageEventHandlers.rbegin(), ite = HookDamage::fallDamageEventHandlers.rend(); it != ite; ++it)
			{
				it->second(target, &damage, attacker, arg3);
			}
		}

		return fnFallDamage_OLD(target, damage, attacker, arg3);
	}

	bool WaterDamage(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3)
	{
		if (!HookDamage::waterDamageEventHandlers.empty())
		{
			for (auto it = HookDamage::waterDamageEventHandlers.rbegin(), ite = HookDamage::waterDamageEventHandlers.rend(); it != ite; ++it)
			{
				it->second(target, &damage, attacker, arg3);
			}
		}

		return fnWaterDamage_OLD(target, damage, attacker, arg3);
	}

	bool ReflectDamage(Actor* target, float damage, TESObjectREFR* attacker, UInt32 arg3)
	{
		if (!HookDamage::reflectDamageEventHandlers.empty())
		{
			for (auto it = HookDamage::reflectDamageEventHandlers.rbegin(), ite = HookDamage::reflectDamageEventHandlers.rend(); it != ite; ++it)
			{
				it->second(target, &damage, attacker, arg3);
			}
		}

		return fnReflectDamage_OLD(target, damage, attacker, arg3);
	}

	void MagicHeal(Actor* target, UInt32 arg1, UInt32 id, float heal, TESObjectREFR* attacker, ActiveEffect* activeEffect)
	{
		if (!HookDamage::magicHealEventHandlers.empty())
		{
			for (auto it = HookDamage::magicHealEventHandlers.rbegin(), ite = HookDamage::magicHealEventHandlers.rend(); it != ite; ++it)
			{
				it->second(target, arg1, id, &heal, attacker, activeEffect);
			}
		}

		fnMagicHeal_OLD(target, arg1, id, heal, attacker);
	}

	void Init()
	{
		{
			struct EvadeDamageHook_Code : Xbyak::CodeGenerator
			{
				EvadeDamageHook_Code(void * buf, UInt64 funcAddr1, UInt64 funcAddr2, UInt64 targetAddr) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel1;
					Xbyak::Label retnLabel2;
					Xbyak::Label retnLabel3;
					Xbyak::Label funcLabel1;
					Xbyak::Label funcLabel2;
					Xbyak::Label evasionLabel;
					Xbyak::Label defaultLabel;

					sub(rsp, 0x20);

					call(ptr[rip + funcLabel1]);

					add(rsp, 0x20);

					test(al, al);
					je(defaultLabel);

					jmp(ptr[rip + retnLabel1]);

					L(defaultLabel);

					sub(rsp, 0x20);

					mov(rdx, r15);
					mov(rcx, r14);
					call(ptr[rip + funcLabel2]);

					add(rsp, 0x20);

					test(al, al);
					jne(evasionLabel);

					jmp(ptr[rip + retnLabel2]);

					L(evasionLabel);

					jmp(ptr[rip + retnLabel3]);

					L(funcLabel1);
					dq(funcAddr1);

					L(funcLabel2);
					dq(funcAddr2);

					L(retnLabel1);
					dq(targetAddr + 0x9);

					L(retnLabel2);
					dq(targetAddr + 0x20);

					L(retnLabel3);
					dq(targetAddr + 0xECB);
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			EvadeDamageHook_Code code(codeBuf, uintptr_t(EvadeDamage1), uintptr_t(EvadeDamage2), EvadeDamage_Target);
			g_localTrampoline.EndAlloc(code.getCurr());

			fnEvadeDamage1_OLD = (FnEvadeDamage1_OLD)Write5BranchEx(EvadeDamage_Target, uintptr_t(code.getCode()));
		}

		g_branchTrampoline.Write5Branch(PhysicalDamage_Target1, PhysicalDamage_Retn1);

		{
			struct PhysicalDamageHook_Code : Xbyak::CodeGenerator
			{
				PhysicalDamageHook_Code(void * buf, UInt64 funcAddr, UInt64 targetAddr) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel1;
					Xbyak::Label retnLabel2;
					Xbyak::Label retnLabel3;
					Xbyak::Label funcLabel;
					Xbyak::Label label1;
					Xbyak::Label label2;

					sub(rsp, 0x20);

					mov(ptr[rsp + 0x20], r15);
					call(ptr[rip + funcLabel]);

					add(rsp, 0x20);

					cmp(eax, 2);
					je(label2);

					cmp(eax, 1);
					je(label1);

					jmp(ptr[rip + retnLabel1]);

					L(label1);

					jmp(ptr[rip + retnLabel2]);

					L(label2);

					mov(rdi, ptr[rsp + 0x48]);
					jmp(ptr[rip + retnLabel3]);

					L(funcLabel);
					dq(funcAddr);

					L(retnLabel1);
					dq(targetAddr + 0x9);

					L(retnLabel2);
					dq(targetAddr + 0x2C);

					L(retnLabel3);
					dq(targetAddr + 0x495);
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			PhysicalDamageHook_Code code(codeBuf, uintptr_t(PhysicalDamage), PhysicalDamage_Target2);
			g_localTrampoline.EndAlloc(code.getCurr());

			fnPhysicalDamage_OLD = (FnPhysicalDamage_OLD)Write5BranchEx(PhysicalDamage_Target2, uintptr_t(code.getCode()));
		}

		{
			struct MagicDamageHook_Code : Xbyak::CodeGenerator
			{
				MagicDamageHook_Code(void * buf, UInt64 funcAddr, UInt64 targetAddr) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					sub(rsp, 0x20);

					mov(ptr[rsp + 0x20], rdi);
					call(ptr[rip + funcLabel]);

					add(rsp, 0x20);

					jmp(ptr[rip + retnLabel]);

					L(funcLabel);
					dq(funcAddr);

					L(retnLabel);
					dq(targetAddr + 0x5);
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			MagicDamageHook_Code code(codeBuf, uintptr_t(MagicDamage), MagicDamage_Target);
			g_localTrampoline.EndAlloc(code.getCurr());

			fnMagicDamage_OLD = (FnMagicDamage_OLD)Write5BranchEx(MagicDamage_Target, uintptr_t(code.getCode()));
		}

		{
			struct ActorValueHook_Code : Xbyak::CodeGenerator
			{
				ActorValueHook_Code(void * buf, UInt64 funcAddr, UInt64 targetAddr) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					sub(rsp, 0x20);

					xor(rax, rax);
					mov(ptr[rsp + 0x20], rax);
					call(ptr[rip + funcLabel]);

					add(rsp, 0x20);

					jmp(ptr[rip + retnLabel]);

					L(funcLabel);
					dq(funcAddr);

					L(retnLabel);
					dq(targetAddr + 0x5);
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			ActorValueHook_Code code(codeBuf, uintptr_t(ActorValue), ActorValue_Target);
			g_localTrampoline.EndAlloc(code.getCurr());

			fnActorValue_OLD = (FnActorValue_OLD)Write5BranchEx(ActorValue_Target, uintptr_t(code.getCode()));
		}

		{
			struct FallDamageHook_Code : Xbyak::CodeGenerator
			{
				FallDamageHook_Code(void * buf, UInt64 funcAddr, UInt64 targetAddr) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					sub(rsp, 0x20);

					call(ptr[rip + funcLabel]);

					add(rsp, 0x20);

					jmp(ptr[rip + retnLabel]);

					L(funcLabel);
					dq(funcAddr);

					L(retnLabel);
					dq(targetAddr + 0x5);
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			FallDamageHook_Code code(codeBuf, uintptr_t(FallDamage), FallDamage_Target);
			g_localTrampoline.EndAlloc(code.getCurr());

			fnFallDamage_OLD = (FnFallDamage_OLD)Write5BranchEx(FallDamage_Target, uintptr_t(code.getCode()));
		}

		{
			struct WaterDamageHook_Code : Xbyak::CodeGenerator
			{
				WaterDamageHook_Code(void * buf, UInt64 funcAddr, UInt64 targetAddr) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					sub(rsp, 0x20);

					call(ptr[rip + funcLabel]);

					add(rsp, 0x20);

					jmp(ptr[rip + retnLabel]);

					L(funcLabel);
					dq(funcAddr);

					L(retnLabel);
					dq(targetAddr + 0x5);
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			WaterDamageHook_Code code(codeBuf, uintptr_t(WaterDamage), WaterDamage_Target);
			g_localTrampoline.EndAlloc(code.getCurr());

			fnWaterDamage_OLD = (FnWaterDamage_OLD)Write5BranchEx(WaterDamage_Target, uintptr_t(code.getCode()));
		}

		{
			struct ReflectDamageHook_Code : Xbyak::CodeGenerator
			{
				ReflectDamageHook_Code(void * buf, UInt64 funcAddr, UInt64 targetAddr) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					sub(rsp, 0x20);

					call(ptr[rip + funcLabel]);

					add(rsp, 0x20);

					jmp(ptr[rip + retnLabel]);

					L(funcLabel);
					dq(funcAddr);

					L(retnLabel);
					dq(targetAddr + 0x5);
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			ReflectDamageHook_Code code(codeBuf, uintptr_t(ReflectDamage), ReflectDamage_Target);
			g_localTrampoline.EndAlloc(code.getCurr());

			fnReflectDamage_OLD = (FnReflectDamage_OLD)Write5BranchEx(ReflectDamage_Target, uintptr_t(code.getCode()));
		}

		{
			struct MagicHealHook_Code : Xbyak::CodeGenerator
			{
				MagicHealHook_Code(void * buf, UInt64 funcAddr, UInt64 targetAddr) : Xbyak::CodeGenerator(4096, buf)
				{
					Xbyak::Label retnLabel;
					Xbyak::Label funcLabel;

					sub(rsp, 0x20);

					mov(ptr[rsp + 0x20], rbx);
					mov(ptr[rsp + 0x28], rdi);
					call(ptr[rip + funcLabel]);

					add(rsp, 0x20);

					jmp(ptr[rip + retnLabel]);

					L(funcLabel);
					dq(funcAddr);

					L(retnLabel);
					dq(targetAddr + 0x5);
				}
			};

			void * codeBuf = g_localTrampoline.StartAlloc();
			MagicHealHook_Code code(codeBuf, uintptr_t(MagicHeal), MagicHeal_Target);
			g_localTrampoline.EndAlloc(code.getCurr());

			fnMagicHeal_OLD = (FnMagicHeal_OLD)Write5BranchEx(MagicHeal_Target, uintptr_t(code.getCode()));
		}
	}
}