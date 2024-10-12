#include "iniSettings.h"
#include "TechniquesMenu.h"
#include "TechniquesManager.h"
#include "utils.h"

#include <SKSE/DebugLog.h>
#include <SKSE/GameReferences.h>

class AutoOpenTechniquesMenuSink : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	EventResult ReceiveEvent(MenuOpenCloseEvent *evn, BSTEventSource<MenuOpenCloseEvent> *src)
	{
		MenuManager *mm = MenuManager::GetSingleton();
		UIStringHolder *holder = UIStringHolder::GetSingleton();

		if (evn->menuName == holder->loadingMenu && evn->IsClose() && !mm->IsMenuOpen(holder->mainMenu))
		{
			TechniquesMenu *TechniquesMenu = TechniquesMenu::GetSingleton();
			if (!TechniquesMenu)
			{
				UIManager *ui = UIManager::GetSingleton();
				ui->AddMessage("TechniquesMenu", UIMessage::kMessage_Open, nullptr);
			}

			mm->BSTEventSource<MenuOpenCloseEvent>::RemoveEventSink(this);
			delete this;
		}

		return kEvent_Continue;
	}

	static void Register()
	{
		MenuManager *mm = MenuManager::GetSingleton();
		if (mm)
		{
			AutoOpenTechniquesMenuSink *sink = new AutoOpenTechniquesMenuSink();
			if (sink)
			{
				mm->BSTEventSource<MenuOpenCloseEvent>::AddEventSink(sink);
			}
		}
	}
};

TechniquesMenu * TechniquesMenu::ms_pSingleton = nullptr;

TechniquesMenu::TechniquesMenu()
{
	const char swfName[] = "TechniquesMenu";

	if (LoadMovie(swfName, GFxMovieView::SM_NoScale, 0.0f))
	{
		_MESSAGE("loaded Inteface/%s.swf", swfName);

		menuDepth = 2;
		flags = kType_DoNotDeleteOnClose | kType_DoNotPreventGameSave | kType_Unk10000;
	}
}

TechniquesMenu::~TechniquesMenu()
{
}

bool TechniquesMenu::Register()
{
	MenuManager *mm = MenuManager::GetSingleton();
	if (!mm)
		return false;

	mm->Register("TechniquesMenu", []() -> IMenu * { return new TechniquesMenu; });

	AutoOpenTechniquesMenuSink::Register();

	return true;
}

UInt32 TechniquesMenu::ProcessMessage(UIMessage *message)
{
	UInt32 result = kResult_NotProcessed;

	if (view)
	{
		switch (message->type)
		{
		case UIMessage::kMessage_Open:
			OnMenuOpen();
			result = kResult_Processed;
			break;
		case UIMessage::kMessage_Close:
			OnMenuClose();
			result = kResult_Processed;
			break;
		}
	}

	return result;
}

void TechniquesMenu::Render()
{
	if (view)
	{
		NiPoint3 screenPos(-1.0f, -1.0f, -1.0f);
		if (mode_Arrow == Mode_Arrow::Mode_Default && ((stackCountHM >= ini.RequiredStackNumberHM && ini.DisplayMarkerHM && Utils::PlayerHasPerk(ini.RequiredPerkHM))\
			|| (stackCountHA >= ini.RequiredStackNumberHA && ini.DisplayMarkerHA && Utils::PlayerHasPerk(ini.RequiredPerkHA))) && !MenuManager::IsInMenuMode() && enemyHealth)
		{
			RefHandle handle = enemyHealth->handle;
			if (handle != g_invalidRefHandle)
			{
				TESObjectREFRPtr refPtr;
				if (TESObjectREFR::LookupByHandle(handle, refPtr) && refPtr->formType == FormType::Character)
				{
					Actor* actor = static_cast<Actor*>((TESForm*)refPtr);
					NiAVObject* object = Utils::GetTorsoObject(actor);
					if (object && g_thePlayer->HasLOS(actor) && !actor->IsDead(true) && actor->GetActorValueCurrent(54) <= 0.0f && !actor->IsGhost())
					{
						NiPoint3 worldPos(object->m_worldTransform.pos.x, object->m_worldTransform.pos.y, object->m_worldTransform.pos.z);
						Utils::WorldPtToScreenPt3(worldPos, screenPos);
					}
				}
			}
		}

		GFxValue args[3];
		args[0] = screenPos.x;
		args[1] = screenPos.y;
		args[2] = screenPos.z;

		view->Invoke("_root.widget.UpdateWidget", nullptr, args, 3);
	}

	IMenu::Render();
}

void TechniquesMenu::OnMenuOpen()
{
	if (!ms_pSingleton)
		ms_pSingleton = this;
}

void TechniquesMenu::OnMenuClose()
{
	if (ms_pSingleton)
		ms_pSingleton = nullptr;

	AutoOpenTechniquesMenuSink::Register();
}