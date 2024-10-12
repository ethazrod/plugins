#pragma once
#include <SKSE/GameReferences.h>

#include <unordered_map>

typedef bool(*FnCondition)(Actor*, std::vector<UInt32>*, UInt32);

namespace ConditionFunctions
{
	//1==================================================================================================
	bool IsEquippedRight(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsEquippedRightType(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsEquippedRightHasKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsEquippedLeft(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsEquippedLeftType(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsEquippedLeftHasKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsEquippedShout(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsWorn(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsWornHasKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsFemale(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsChild(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsPlayerTeammate(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsInInterior(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsInFaction(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool HasKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool HasMagicEffect(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool HasMagicEffectWithKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool HasPerk(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool HasSpell(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsActorValueEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsActorValueLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsActorValueBaseEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsActorValueBaseLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsActorValueMaxEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsActorValueMaxLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsActorValuePercentageEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsActorValuePercentageLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsLevelLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsActorBase(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsRace(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool CurrentWeather(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool CurrentGameTimeLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool ValueEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool ValueLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool Random(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	//2===================================================================================================
	bool IsUnique(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsClass(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsCombatStyle(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsVoiceType(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsAttacking(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsRunning(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsSneaking(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsSprinting(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsInAir(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsInCombat(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsWeaponDrawn(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsInLocation(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool HasRefType(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsParentCell(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsWorldSpace(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsFactionRankEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsFactionRankLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
	bool IsMovementDirection(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags);
}

class ConditionFunction
{
public:
	ConditionFunction(FnCondition func, UInt32 numArgs, UInt32 allowDirectlyFlags) : func(func), numArgs(numArgs), allowDirectlyFlags(allowDirectlyFlags)
	{
	}

	FnCondition func;
	UInt32 numArgs;
	UInt32 allowDirectlyFlags;
};

extern std::unordered_map<std::string, ConditionFunction> conditionFunctionMap;