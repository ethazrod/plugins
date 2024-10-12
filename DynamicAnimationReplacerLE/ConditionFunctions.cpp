#include "ConditionFunctions.h"

#include <SKSE/DebugLog.h>
#include <SKSE/GameData.h>
#include <SKSE/GameForms.h>
#include <SKSE/GameObjects.h>
#include <SKSE/GameExtraData.h>

#include <random>

#define _USE_MATH_DEFINES
#include <math.h>

std::unordered_map<std::string, ConditionFunction> conditionFunctionMap = {
	//1==================================================================================================
	{"IsEquippedRight",				ConditionFunction(ConditionFunctions::IsEquippedRight, 1, 0) },
{ "IsEquippedRightType",			ConditionFunction(ConditionFunctions::IsEquippedRightType, 1, 1) },
{ "IsEquippedRightHasKeyword",		ConditionFunction(ConditionFunctions::IsEquippedRightHasKeyword, 1, 0) },
{ "IsEquippedLeft",					ConditionFunction(ConditionFunctions::IsEquippedLeft, 1, 0) },
{ "IsEquippedLeftType",				ConditionFunction(ConditionFunctions::IsEquippedLeftType, 1, 1) },
{ "IsEquippedLeftHasKeyword",		ConditionFunction(ConditionFunctions::IsEquippedLeftHasKeyword, 1, 0) },
{ "IsEquippedShout",				ConditionFunction(ConditionFunctions::IsEquippedShout, 1, 0) },
{ "IsWorn",							ConditionFunction(ConditionFunctions::IsWorn, 1, 0) },
{ "IsWornHasKeyword",				ConditionFunction(ConditionFunctions::IsWornHasKeyword, 1, 0) },
{ "IsFemale",						ConditionFunction(ConditionFunctions::IsFemale, 0, 0) },
{ "IsChild",						ConditionFunction(ConditionFunctions::IsChild, 0, 0) },
{ "IsPlayerTeammate",				ConditionFunction(ConditionFunctions::IsPlayerTeammate, 0, 0) },
{ "IsInInterior",					ConditionFunction(ConditionFunctions::IsInInterior, 0, 0) },
{ "IsInFaction",					ConditionFunction(ConditionFunctions::IsInFaction, 1, 0) },
{ "HasKeyword",						ConditionFunction(ConditionFunctions::HasKeyword, 1, 0) },
{ "HasMagicEffect",					ConditionFunction(ConditionFunctions::HasMagicEffect, 1, 0) },
{ "HasMagicEffectWithKeyword",		ConditionFunction(ConditionFunctions::HasMagicEffectWithKeyword, 1, 0) },
{ "HasPerk",						ConditionFunction(ConditionFunctions::HasPerk, 1, 0) },
{ "HasSpell",						ConditionFunction(ConditionFunctions::HasSpell, 1, 0) },
{ "IsActorValueEqualTo",			ConditionFunction(ConditionFunctions::IsActorValueEqualTo, 2, (1 | 2)) },
{ "IsActorValueLessThan",			ConditionFunction(ConditionFunctions::IsActorValueLessThan, 2, (1 | 2)) },
{ "IsActorValueBaseEqualTo",		ConditionFunction(ConditionFunctions::IsActorValueBaseEqualTo, 2, (1 | 2)) },
{ "IsActorValueBaseLessThan",		ConditionFunction(ConditionFunctions::IsActorValueBaseLessThan, 2, (1 | 2)) },
{ "IsActorValueMaxEqualTo",			ConditionFunction(ConditionFunctions::IsActorValueMaxEqualTo, 2, (1 | 2)) },
{ "IsActorValueMaxLessThan",		ConditionFunction(ConditionFunctions::IsActorValueMaxLessThan, 2, (1 | 2)) },
{ "IsActorValuePercentageEqualTo",	ConditionFunction(ConditionFunctions::IsActorValuePercentageEqualTo, 2, (1 | 2)) },
{ "IsActorValuePercentageLessThan",	ConditionFunction(ConditionFunctions::IsActorValuePercentageLessThan, 2, (1 | 2)) },
{ "IsLevelLessThan",				ConditionFunction(ConditionFunctions::IsLevelLessThan, 1, 1) },
{ "IsActorBase",					ConditionFunction(ConditionFunctions::IsActorBase, 1, 0) },
{ "IsRace",							ConditionFunction(ConditionFunctions::IsRace, 1, 0) },
{ "CurrentWeather",					ConditionFunction(ConditionFunctions::CurrentWeather, 1, 0) },
{ "CurrentGameTimeLessThan",		ConditionFunction(ConditionFunctions::CurrentGameTimeLessThan, 1, 1) },
{ "ValueEqualTo",					ConditionFunction(ConditionFunctions::ValueEqualTo, 2, (1 | 2)) },
{ "ValueLessThan",					ConditionFunction(ConditionFunctions::ValueLessThan, 2, (1 | 2)) },
{ "Random",							ConditionFunction(ConditionFunctions::Random, 1, 1) },
//2===================================================================================================
{ "IsUnique",						ConditionFunction(ConditionFunctions::IsUnique, 0, 0) },
{ "IsClass",						ConditionFunction(ConditionFunctions::IsClass, 1, 0) },
{ "IsCombatStyle",					ConditionFunction(ConditionFunctions::IsCombatStyle, 1, 0) },
{ "IsVoiceType",					ConditionFunction(ConditionFunctions::IsVoiceType, 1, 0) },
{ "IsAttacking",					ConditionFunction(ConditionFunctions::IsAttacking, 0, 0) },
{ "IsRunning",						ConditionFunction(ConditionFunctions::IsRunning, 0, 0) },
{ "IsSneaking",						ConditionFunction(ConditionFunctions::IsSneaking, 0, 0) },
{ "IsSprinting",					ConditionFunction(ConditionFunctions::IsSprinting, 0, 0) },
{ "IsInAir",						ConditionFunction(ConditionFunctions::IsInAir, 0, 0) },
{ "IsInCombat",						ConditionFunction(ConditionFunctions::IsInCombat, 0, 0) },
{ "IsWeaponDrawn",					ConditionFunction(ConditionFunctions::IsWeaponDrawn, 0, 0) },
{ "IsInLocation",					ConditionFunction(ConditionFunctions::IsInLocation, 1, 0) },
{ "HasRefType",						ConditionFunction(ConditionFunctions::HasRefType, 1, 0) },
{ "IsParentCell",					ConditionFunction(ConditionFunctions::IsParentCell, 1, 0) },
{ "IsWorldSpace",					ConditionFunction(ConditionFunctions::IsWorldSpace, 1, 0) },
{ "IsFactionRankEqualTo",			ConditionFunction(ConditionFunctions::IsFactionRankEqualTo, 2, 1) },
{ "IsFactionRankLessThan",			ConditionFunction(ConditionFunctions::IsFactionRankLessThan, 2, 1) },
{ "IsMovementDirection",			ConditionFunction(ConditionFunctions::IsMovementDirection, 1, 1) }
};

static bool GetGlobalVariables(std::vector<UInt32>* args, UInt32 numArgs, UInt32 directlyFlags, float* valueOut)
{
	UInt32 flag = 1;
	for (int i = 0; i < numArgs; i++)
	{
		if ((directlyFlags & flag) != 0)
		{
			float* valuePtr = (float*)&(*args)[i];
			valueOut[i] = *valuePtr;
		}
		else
		{
			TESForm* global = LookupFormByID((*args)[i]);
			if (!global || global->formType != FormType::Global)
				return false;

			valueOut[i] = static_cast<TESGlobal*>(global)->value;
		}

		flag *= 2;
	}

	return true;
}

class ActorEx : public Actor
{
public:
	DEFINE_MEMBER_FN(HasSpellFix, bool, 0x006EAF10, SpellItem* spell);
	DEFINE_MEMBER_FN(HasShoutFix, bool, 0x006E9130, TESShout* shout);
	DEFINE_MEMBER_FN(IsInAir, bool, 0x006AB2F0);
	DEFINE_MEMBER_FN(IsMoving, bool, 0x006ABBF0);
	DEFINE_MEMBER_FN(GetFactionRank, SInt32, 0x006A90C0, TESFaction *, bool isPlayer);
	DEFINE_MEMBER_FN(GetMovementDirection, float, 0x006A7BD0);
};

class UtilityScript
{
public:
	static UtilityScript* GetSingleton()
	{
		return *(UtilityScript**)0x012E35DC;
	}

	float GetCurrentGameTime()
	{
		float int_part = 0.0f;
		return std::modf(Fn0068CFA0(), &int_part) * *(float*)0x010A5010;
	}

	DEFINE_MEMBER_FN(Fn0068CFA0, float, 0x0068CFA0);
};

enum GameDataEx
{
	kType_Warhammers = 10,
	kType_Shields,
	kType_Alteration_Spells,
	kType_Illusion_Spells,
	kType_Destruction_Spells,
	kType_Conjuration_Spells,
	kType_Restoration_Spells,
	kType_Scrolls,
	kType_Torches
};

static int GetEquippedType(TESForm* form)
{
	if (!form)
		return TESObjectWEAP::GameData::kType_HandToHandMelee;

	switch (form->formType)
	{
	case FormType::Weapon:
	{
		TESObjectWEAP *weap = static_cast<TESObjectWEAP*>(form);
		UInt8 type = weap->type();
		if (type > TESObjectWEAP::GameData::kType_CrossBow)
		{
			return -1;
		}
		else if (type == TESObjectWEAP::GameData::kType_TwoHandAxe)
		{
			static BGSKeyword *keywordWarHammer = (BGSKeyword*)TESForm::LookupByID(0x06D930);
			if (static_cast<BGSKeywordForm*>(weap)->HasKeyword(keywordWarHammer))
				return GameDataEx::kType_Warhammers;
		}

		return type;
	}
	case FormType::Armor:
	{
		if (static_cast<TESObjectARMO*>(form)->HasPartOf(BGSBipedObjectForm::kPart_Shield))
			return GameDataEx::kType_Shields;

		return -1;
	}
	case FormType::Spell:
	{
		SpellItem *spell = static_cast<SpellItem*>(form);
		if (!spell->unk44)
			return -1;

		switch (spell->unk44->school())
		{
		case EffectSetting::Properties::kActorValue_Alteration:
			return GameDataEx::kType_Alteration_Spells;
		case EffectSetting::Properties::kActorValue_Illusion:
			return GameDataEx::kType_Illusion_Spells;
		case EffectSetting::Properties::kActorValue_Destruction:
			return GameDataEx::kType_Destruction_Spells;
		case EffectSetting::Properties::kActorValue_Conjuration:
			return GameDataEx::kType_Conjuration_Spells;
		case EffectSetting::Properties::kActorValue_Restoration:
			return GameDataEx::kType_Restoration_Spells;
		default:
			return -1;
		}
	}
	case FormType::ScrollItem:
		return GameDataEx::kType_Scrolls;
	case FormType::Light:
		return GameDataEx::kType_Torches;
	default:
		return -1;
	}
}

static bool IsWornVisitor(Actor* actor, std::function<bool(TESForm*)> visitor)
{
	ExtraContainerChanges *exChanges = actor->extraData.GetByType<ExtraContainerChanges>();
	if (exChanges && exChanges->changes && exChanges->changes->entryList)
	{
		for (InventoryEntryData *pEntry : *exChanges->changes->entryList)
		{
			if (!pEntry || !visitor(pEntry->baseForm) || !pEntry->extraList)
				continue;

			for (BaseExtraList *pExtraDataList : *pEntry->extraList)
			{
				if (pExtraDataList)
				{
					if (pExtraDataList->HasType(ExtraDataType::Worn) || pExtraDataList->HasType(ExtraDataType::WornLeft))
						return true;
				}
			}
		}
	}

	return false;
}

static float GetActorValuePercentage(Actor* actor, UInt32 id)
{
	float current = actor->GetActorValueCurrent(id);
	float max = actor->GetActorValueMaximum(id);
	if (max <= 0.0f)
		return 1.0f;

	if (current <= 0.0f)
		return 0.0f;

	if (current >= max)
		return 1.0f;

	return current / max;
}

static double NormalAbsoluteAngle(double angle)
{
	while (angle < 0)
		angle += 2 * M_PI;
	while (angle > 2 * M_PI)
		angle -= 2 * M_PI;
	return angle;
}

namespace ConditionFunctions
{
	//1==================================================================================================

	bool IsEquippedRight(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* form = actor->GetEquippedObject(false);
		return form && form->formID == (*args)[0];
	}

	bool IsEquippedRightType(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[1];
		if (!GetGlobalVariables(args, 1, directlyFlags, value))
			return false;

		return GetEquippedType(actor->GetEquippedObject(false)) == (int)value[0];
	}

	bool IsEquippedRightHasKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* form = actor->GetEquippedObject(false);
		if (!form)
			return false;

		TESForm* keyword = LookupFormByID((*args)[0]);
		return keyword && keyword->formType == FormType::Keyword && form->HasKeyword((BGSKeyword*)keyword);
	}

	bool IsEquippedLeft(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* form = actor->GetEquippedObject(true);
		return form && form->formID == (*args)[0];
	}

	bool IsEquippedLeftType(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[1];
		if (!GetGlobalVariables(args, 1, directlyFlags, value))
			return false;

		return GetEquippedType(actor->GetEquippedObject(true)) == (int)value[0];
	}

	bool IsEquippedLeftHasKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* form = actor->GetEquippedObject(true);
		if (!form)
			return false;

		TESForm* keyword = LookupFormByID((*args)[0]);
		return keyword && keyword->formType == FormType::Keyword && form->HasKeyword((BGSKeyword*)keyword);
	}

	bool IsEquippedShout(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* form = actor->equippedShout;
		return form && form->formID == (*args)[0];
	}

	bool IsWorn(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return IsWornVisitor(actor, [args](TESForm* form)-> bool {
			return form && form->formID == (*args)[0];
		});
	}

	bool IsWornHasKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return IsWornVisitor(actor, [args](TESForm* form)-> bool {
			if (!form)
				return false;

			TESForm* keyword = LookupFormByID((*args)[0]);
			return keyword && keyword->formType == FormType::Keyword && form->HasKeyword((BGSKeyword*)keyword);
		});
	}

	bool IsFemale(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESNPC* base = actor->GetActorBase();
		return base && base->GetSex();
	}

	bool IsChild(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return actor->IsChild();
	}

	bool IsPlayerTeammate(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return actor->IsPlayerTeammate();
	}

	bool IsInInterior(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESObjectCELL* cell = actor->GetParentCell();
		return cell && cell->IsInterior();
	}

	bool IsInFaction(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* faction = LookupFormByID((*args)[0]);
		return faction && faction->formType == FormType::Faction && actor->IsInFaction(static_cast<TESFaction*>(faction));
	}

	bool HasKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* keyword = LookupFormByID((*args)[0]);
		return keyword && keyword->formType == FormType::Keyword && actor->HasKeyword((BGSKeyword*)keyword);
	}

	bool HasMagicEffect(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* activeEffect = LookupFormByID((*args)[0]);
		return activeEffect && activeEffect->formType == FormType::EffectSetting && actor->HasMagicEffect((ActiveEffect*)activeEffect);
	}

	bool HasMagicEffectWithKeyword(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* keyword = LookupFormByID((*args)[0]);
		return keyword && keyword->formType == FormType::Keyword && actor->HasMagicEffectWithKeyword((BGSKeyword*)keyword);
	}

	bool HasPerk(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* perk = LookupFormByID((*args)[0]);
		return perk && perk->formType == FormType::Perk && actor->HasPerk(static_cast<BGSPerk*>(perk));
	}

	bool HasSpell(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* spell = LookupFormByID((*args)[0]);
		if (!spell)
			return false;

		if (spell->formType == FormType::Spell)
			return ((ActorEx*)actor)->HasSpellFix(static_cast<SpellItem*>(spell));
		else if (spell->formType == FormType::Shout)
			return ((ActorEx*)actor)->HasShoutFix(static_cast<TESShout*>(spell));

		return false;
	}

	bool IsActorValueEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return actor->GetActorValueCurrent((UInt32)value[0]) == value[1];
	}

	bool IsActorValueLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return actor->GetActorValueCurrent((UInt32)value[0]) < value[1];
	}

	bool IsActorValueBaseEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return actor->GetActorValueBase((UInt32)value[0]) == value[1];
	}

	bool IsActorValueBaseLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return actor->GetActorValueBase((UInt32)value[0]) < value[1];
	}

	bool IsActorValueMaxEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return actor->GetActorValueMaximum((UInt32)value[0]) == value[1];
	}

	bool IsActorValueMaxLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return actor->GetActorValueMaximum((UInt32)value[0]) < value[1];
	}

	bool IsActorValuePercentageEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return GetActorValuePercentage(actor, (UInt32)value[0]) == value[1];
	}

	bool IsActorValuePercentageLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return GetActorValuePercentage(actor, (UInt32)value[0]) < value[1];
	}

	bool IsLevelLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[1];
		if (!GetGlobalVariables(args, 1, directlyFlags, value))
			return false;

		return actor->GetLevel() < value[0];
	}

	bool IsActorBase(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESNPC* base = actor->GetActorBase();
		return base && base->formID == (*args)[0];
	}

	bool IsRace(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESRace* race = actor->GetRace();
		return race && race->formID == (*args)[0];
	}

	bool CurrentWeather(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		//Sky* sky = Sky::GetSingleton();
		typedef Sky*(*FnGetSky)();
		const FnGetSky fnGetSky = (FnGetSky)0x0043B920;
		Sky* sky = fnGetSky();
		return sky && sky->curentWeather && sky->curentWeather->formID == (*args)[0];
	}

	bool CurrentGameTimeLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[1];
		if (!GetGlobalVariables(args, 1, directlyFlags, value))
			return false;

		return UtilityScript::GetSingleton()->GetCurrentGameTime() < value[0];
	}

	bool ValueEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return value[0] == value[1];
	}

	bool ValueLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[2];
		if (!GetGlobalVariables(args, 2, directlyFlags, value))
			return false;

		return value[0] < value[1];
	}

	bool Random(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[1];
		if (!GetGlobalVariables(args, 1, directlyFlags, value))
			return false;

		std::mt19937 mt(std::random_device{}());
		std::uniform_real_distribution<> score(0.0, 1.0);
		return score(mt) < value[0];
	}

	//2===================================================================================================

	bool IsUnique(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESNPC* base = actor->GetActorBase();
		return base && base->TESActorBaseData::flags.unique;
	}

	bool IsClass(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESNPC* base = actor->GetActorBase();
		return base && base->npcClass && base->npcClass->formID == (*args)[0];
	}

	bool IsCombatStyle(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESNPC* base = actor->GetActorBase();
		return base && base->combatStyle && base->combatStyle->formID == (*args)[0];
	}

	bool IsVoiceType(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESNPC* base = actor->GetActorBase();
		return base && base->voiceType && base->voiceType->formID == (*args)[0];
	}

	bool IsAttacking(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return (actor->flags04 & 0xF0000000) != 0;
	}

	bool IsRunning(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return actor->IsRunning();
	}

	bool IsSneaking(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return actor->IsSneaking();
	}

	bool IsSprinting(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return actor->IsSprinting();
	}

	bool IsInAir(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return ((ActorEx*)actor)->IsInAir();
	}

	bool IsInCombat(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return actor->IsInCombat();
	}

	bool IsWeaponDrawn(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		return actor->IsWeaponDrawn();
	}

	bool IsInLocation(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* loc = LookupFormByID((*args)[0]);
		if (!loc || loc->formType != FormType::Location)
			return false;

		BGSLocation* currentLoc = actor->GetCurrentLocation();
		if (!currentLoc)
			return false;

		return static_cast<BGSLocation*>(loc)->IsChild(currentLoc) || currentLoc == loc;
	}

	bool HasRefType(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESForm* refType = LookupFormByID((*args)[0]);
		if (!refType || refType->formType != FormType::LocationRef)
			return false;

		return actor->HasRefType(static_cast<BGSLocationRefType*>(refType));
	}

	bool IsParentCell(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESObjectCELL* cell = actor->GetParentCell();
		return cell && cell->formID == (*args)[0];
	}

	bool IsWorldSpace(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		TESWorldSpace* worldSpace = actor->GetWorldSpace();
		return worldSpace && worldSpace->formID == (*args)[0];
	}

	bool IsFactionRankEqualTo(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[1];
		if (!GetGlobalVariables(args, 1, directlyFlags, value))
			return false;

		TESForm* faction = LookupFormByID((*args)[1]);
		return faction && faction->formType == FormType::Faction && ((ActorEx*)actor)->GetFactionRank(static_cast<TESFaction*>(faction), actor == g_thePlayer) == value[0];
	}

	bool IsFactionRankLessThan(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[1];
		if (!GetGlobalVariables(args, 1, directlyFlags, value))
			return false;

		TESForm* faction = LookupFormByID((*args)[1]);
		return faction && faction->formType == FormType::Faction && ((ActorEx*)actor)->GetFactionRank(static_cast<TESFaction*>(faction), actor == g_thePlayer) < value[0];
	}

	bool IsMovementDirection(Actor* actor, std::vector<UInt32>* args, UInt32 directlyFlags)
	{
		float value[1];
		if (!GetGlobalVariables(args, 1, directlyFlags, value))
			return false;

		float dir = 0.0f;
		if (((ActorEx*)actor)->IsMoving())
		{
			dir = NormalAbsoluteAngle(((ActorEx*)actor)->GetMovementDirection());
			dir /= 6.283185f;
			dir += 0.125f;
			dir *= 4.0f;
			dir = fmod(dir, 4.0f);
			dir = floor(dir);
			dir += 1.0f;
		}

		return dir == value[0];
	}
}