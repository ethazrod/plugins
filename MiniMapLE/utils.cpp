#include "utils.h"

#include <SKSE/DebugLog.h>

namespace Utils
{
	bool QuestTargetConditionMatch(TESQuest* quest, QuestTarget* target)
	{
		if (!target->condition.nodes)
			return true;

		QuestConditionParams params;
		RefHandle handle;
		target->Fn005766D0(&handle, 1, quest);
		TESObjectREFRPtr refPtr;
		LookupREFRByHandle(handle, refPtr);
		params.ref = refPtr;
		params.Fn005E92D0(quest);

		return target->condition.QuestConditionMatch(&params);
	}
}