#include "utils.h"
#include "iniSettings.h"

#include <SKSE/DebugLog.h>
#include <SKSE/GameObjects.h>
#include <SKSE/GameReferences.h>

#include <unordered_map>

static std::unordered_map<std::string, BSFixedString> TorsoNodeMap = {
	{ "actors\\character\\defaultmale.hkx" , "NPC Spine2 [Spn2]" },
{ "actors\\character\\defaultfemale.hkx" , "NPC Spine2 [Spn2]" },
{ "actors\\draugr\\draugrproject.hkx" , "NPC Spine2 [Spn2]" },
{ "actors\\spriggan\\spriggan.hkx" , "NPC COM [COM ]" },
{ "actors\\draugr\\draugrskeletonproject.hkx" , "NPC Spine2 [Spn2]" },
{ "actors\\vampirelord\\vampirelord.hkx" , "NPC Spine2 [Spn2]" },
{ "actors\\hagraven\\hagravenproject.hkx" , "NPC Spine1 [Spn1]" },
{ "actors\\dlc02\\benthiclurker\\benthiclurkerproject.hkx" , "NPC Spine1 [Spn1]" },
{ "actors\\giant\\giantproject.hkx" , "NPC Spine1 [Spn1]" },
{ "actors\\atronachfrost\\atronachfrostproject.hkx" , "NPC Spine2 [Spn2]" },
{ "actors\\dlc01\\vampirebrute\\vampirebruteproject.hkx" , "NPC Spine1 [Spn1]" },
{ "actors\\troll\\trollproject.hkx" , "NPC Spine2 [Spn2]" },
{ "actors\\werewolfbeast\\werewolfbeastproject.hkx" , "NPC Spine2 [Spn2]" },
{ "actors\\witchlight\\witchlightproject.hkx" , "Witchlight Body Lag" },
{ "actors\\ambient\\hare\\hareproject.hkx" , "RabbitSpine1" },
{ "actors\\ambient\\chicken\\chickenproject.hkx" , "Spine" },
{ "actors\\mudcrab\\mudcrabproject.hkx" , "Mcrab_Body" },
{ "actors\\dlc02\\netch\netchproject.hkx" , "NetchPelvis [Pelv]" },
{ "actors\\dragon\\dragonproject.hkx" , "NPC Hub01" },
{ "actors\\canine\\dogproject.hkx" , "Canine_Spine3" },
{ "actors\\canine\\wolfproject.hkx" , "Canine_Spine3" },
{ "actors\\falmer\\falmerproject.hkx" , "NPC Spine1" },
{ "actors\\dlc02\\riekling\\rieklingproject.hkx" , "NPC Spine1" },
{ "actors\\bear\\bearproject.hkx" , "NPC Spine3" },
{ "actors\\chaurus\\chaurusproject.hkx" , "Spine[Spn1]" },
{ "actors\\deer\\deerproject.hkx" , "ElkSpine2" },
{ "actors\\dragonpriest\\dragon_priest.hkx" , "DragPriestNPC Spine2 [Spn2]" },
{ "actors\\dwarvensteamcenturion\\steamproject.hkx" , "NPC Spine2 [Spn2]" },
{ "actors\\dwarvenspherecenturion\\spherecenturion.hkx" , "NPC Spine2" },
{ "actors\\atronachflame\\atronachflame.hkx" , "FireAtronach_Spine2 [Spn2]" },
{ "actors\\atronachstorm\\atronachstormproject.hkx" , "Torso Main" },
{ "actors\\goat\\goatproject.hkx" , "Goat_Spine3" },
{ "actors\\horker\\horkerproject.hkx" , "Horker_Head01" },
{ "actors\\horse\\horseproject.hkx" , "HorseSpine3" },
{ "actors\\mammoth\\mammothproject.hkx" , "Mammoth Spine 3" },
{ "actors\\sabrecat\\sabrecatproject.hkx" , "Sabrecat_Spine[Spn3]" },
{ "actors\\skeever\\skeeverproject.hkx" , "SpineUpperSpine" },
{ "actors\\cow\\highlandcowproject.hkx" , "Spine3" },
{ "actors\\dlc01\\chaurusflyer\\chaurusflyer.hkx" , "ChaurusFlyerPelvis [Pelv]" },
{ "actors\\dlc02\\boarriekling\\boarproject.hkx" , "Boar_Spine3" },
{ "actors\\dlc02\\scrib\\scribproject.hkx" , "Pelvis [Pelv]" },
{ "actors\\dlc02\\dwarvenballistacenturion\\ballistacenturion.hkx" , "Pelvis" },
{ "actors\\dwarvenspider\\dwarvenspidercenturionproject.hkx" , "DwarvenSpiderBody" },
{ "actors\\frostbitespider\\frostbitespiderproject.hkx" , "Tail1" },
{ "actors\\icewraith\\icewraithproject.hkx" , "IW Seg02" },
{ "actors\\slaughterfish\\slaughterfishproject.hkx" , "SlaughterfishBody" },
{ "actors\\wisp\\wispproject.hkx" , "Wisp Spine2" },
{ "actors\\dlc02\\hmdaedra\\hmdaedra.hkx" , "NPC COM [COM ]" }
};

namespace Utils
{
	NiAVObject* Utils::GetTorsoObject(Actor* actor)
	{
		NiAVObject* object = (NiAVObject*)actor->GetNiNode();
		if (object && actor->race)
		{
			NiAVObject* foundObject = nullptr;
			if (actor->race->bodyPartData && actor->race->bodyPartData->part[0] && actor->race->bodyPartData->part[0]->unk04.c_str())
				foundObject = object->GetObjectByName(actor->race->bodyPartData->part[0]->unk04);

			if (foundObject)
				return foundObject;

			int gender = 0;
			TESNPC* base = actor->GetActorBase();
			if (base && base->GetSex())
				gender = 1;

			std::string pathString = std::string(actor->race->behaviorGraph[gender].modelName);
			ini.ToLower(pathString);

			if (TorsoNodeMap.count(pathString) >= 1)
				return object->GetObjectByName(TorsoNodeMap.at(pathString));
		}

		return nullptr;
	}

	bool Utils::WorldPtToScreenPt3(const NiPoint3 &in, NiPoint3 &out, float zeroTolerance)
	{
		float *worldToCamMatrix = (float*)0x001B3EA10;
		static NiRect<float> viewPort = { 0.0f, 1.0f, 0.0f, 1.0f };

		typedef bool(*FnWorldPtToScreenPt3)(const float *worldToCamMatrix, const NiRect<float> *port, const NiPoint3 &in, float &x_out, float &y_out, float &z_out, float zeroTolerance);
		const FnWorldPtToScreenPt3 fnWorldPtToScreenPt3 = (FnWorldPtToScreenPt3)0x00AB84C0;

		return fnWorldPtToScreenPt3(worldToCamMatrix, &viewPort, in, out.x, out.y, out.z, zeroTolerance);
	}

	bool Utils::PlayerHasPerk(UInt32 perkID)
	{
		if (perkID == 0)
			return true;

		TESForm *perk = TESForm::LookupByID(perkID);
		if (perk && perk->formType == FormType::Perk && g_thePlayer->HasPerk(static_cast<BGSPerk*>(perk)))
			return true;

		return false;
	}
}