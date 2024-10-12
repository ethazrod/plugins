#pragma once
#include <SKSE/GameReferences.h>
#include <SKSE/GameForms.h>

struct QuestConditionParams
{
	QuestConditionParams() : ref(nullptr), unk04(0), unk08(0), unk0C(0), unk10(0), unk14(0), unk18(0)
	{
	}

	TESObjectREFR*		ref;
	UInt32				unk04;
	UInt32				unk08;
	UInt32				unk0C;
	UInt32				unk10;
	UInt32				unk14;
	UInt32				unk18;

	DEFINE_MEMBER_FN(Fn005E92D0, void, 0x005E92D0, TESQuest* quest);
};

class ConditionEx2 : public Condition
{
public:
	DEFINE_MEMBER_FN(QuestConditionMatch, bool, 0x005E97E0, QuestConditionParams* params);
};

struct QuestTarget
{
	void*			unk00;
	ConditionEx2	condition;
	UInt32			aliasID;
	UInt32			unk0C;

	DEFINE_MEMBER_FN(Fn005766D0, RefHandle*, 0x005766D0, RefHandle* refOut, UInt32 arg1, TESQuest* quest);
};

namespace Utils
{
	bool QuestTargetConditionMatch(TESQuest* quest, QuestTarget* target);
}