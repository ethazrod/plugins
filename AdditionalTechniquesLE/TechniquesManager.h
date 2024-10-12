#pragma once
#include <SKSE/GameMenus.h>

enum Mode_Arrow
{
	Mode_Default = 0,
	Mode_Multishot,
	Mode_Penetrating,
	Mode_Num
};

namespace TechniquesManager
{
	void TechniquesMessageHandler(SKSEMessagingInterface::Message *msg);
	void Init();
}

extern EnemyHealth * enemyHealth;
extern UInt32 stackCountHM;
extern UInt32 stackCountHA;
extern UInt32 mode_Arrow;