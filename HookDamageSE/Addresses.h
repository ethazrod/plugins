#pragma once
#include "skse64/GameReferences.h"

//1.5.97
extern uintptr_t EvadeDamage_Target; //0x00626400 + 0x89
extern uintptr_t PhysicalDamage_Target1; //0x00626400 + 0x36A
extern uintptr_t PhysicalDamage_Retn1; //0x00626400 + 0x374
extern uintptr_t PhysicalDamage_Target2; //0x00626400 + 0x397
extern uintptr_t MagicDamage_Target; //0x00567A80 + 0x237
extern uintptr_t ActorValue_Target; //0x006210F0 + 0x14
extern uintptr_t FallDamage_Target; //0x0060B620 + 0xCE
extern uintptr_t WaterDamage_Target; //0x005D6FB0 + 0x8A3
extern uintptr_t ReflectDamage_Target; //0x00628C20 + 0x3DC
extern uintptr_t MagicHeal_Target; //0x00567A80 + 0x2D1

namespace Addresses
{
	bool Init();
}