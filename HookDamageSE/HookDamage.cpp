#include "HookDamage.h"

namespace HookDamage
{
	std::map<UInt32, FallDamageEventHandler_t> fallDamageEventHandlers;
	std::map<UInt32, ReflectDamageEventHandler_t> reflectDamageEventHandlers;
	std::map<UInt32, WaterDamageEventHandler_t> waterDamageEventHandlers;
	std::map<UInt32, MagicDamageEventHandler_t> magicDamageEventHandlers;
	std::map<UInt32, ActorValueEventHandler_t> actorValueEventHandlers;
	std::map<UInt32, MagicHealEventHandler_t> magicHealEventHandlers;
	std::map<UInt32, EvadeDamageEventHandler_t> evadeDamageEventHandlers;
	std::map<UInt32, PhysicalDamageEventHandler_t> physicalDamageEventHandlers;

	bool RegisterEvent(Hook hookType, void* func, UInt32 priority)
	{
		bool result;

		switch (hookType)
		{
		case Hook::kFallDamage:
			result = fallDamageEventHandlers.insert(std::make_pair(priority, static_cast<FallDamageEventHandler_t>(func))).second;
			break;
		case Hook::kReflectDamage:
			result = reflectDamageEventHandlers.insert(std::make_pair(priority, static_cast<ReflectDamageEventHandler_t>(func))).second;
			break;
		case Hook::kWaterDamage:
			result = waterDamageEventHandlers.insert(std::make_pair(priority, static_cast<WaterDamageEventHandler_t>(func))).second;
			break;
		case Hook::kMagicDamage:
			result = magicDamageEventHandlers.insert(std::make_pair(priority, static_cast<MagicDamageEventHandler_t>(func))).second;
			break;
		case Hook::kActorValue:
			result = actorValueEventHandlers.insert(std::make_pair(priority, static_cast<ActorValueEventHandler_t>(func))).second;
			break;
		case Hook::kMagicHeal:
			result = magicHealEventHandlers.insert(std::make_pair(priority, static_cast<MagicHealEventHandler_t>(func))).second;
			break;
		case Hook::kEvadeDamage:
			result = evadeDamageEventHandlers.insert(std::make_pair(priority, static_cast<EvadeDamageEventHandler_t>(func))).second;
			break;
		case Hook::kPhysicalDamage:
			result = physicalDamageEventHandlers.insert(std::make_pair(priority, static_cast<PhysicalDamageEventHandler_t>(func))).second;
			break;
		default:
			result = false;
			break;
		}

		return result;
	}
}
