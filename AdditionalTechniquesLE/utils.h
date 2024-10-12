#pragma once
#include <SKSE/GameForms.h>
#include <SKSE/NiNodes.h>

namespace Utils
{
	NiAVObject* GetTorsoObject(Actor* actor);
	bool WorldPtToScreenPt3(const NiPoint3 &in, NiPoint3 &out, float zeroTolerance = 1e-5f);
	bool PlayerHasPerk(UInt32 perkID);
}