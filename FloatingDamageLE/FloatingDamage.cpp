#include "FloatingDamage.h"
#include "iniSettings.h"

#include <SKSE/DebugLog.h>
#include <SKSE/PluginAPI.h>
#include <SKSE/GameRTTI.h>

#include <string>
#include <random>

static std::string ReplaceString(std::string& str, int number)
{
	static std::string target = "*";
	std::string retn = str;
	std::string replacement = number > 0 ? std::to_string(number) : "";
	std::string::size_type pos = 0;
	while ((pos = retn.find(target, pos)) != std::string::npos)
	{
		retn.replace(pos, target.length(), replacement);
		pos += replacement.length();
	}

	return retn;
}

class AutoOpenFloatingDamageSink : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	EventResult ReceiveEvent(MenuOpenCloseEvent *evn, BSTEventSource<MenuOpenCloseEvent> *src)
	{
		MenuManager *mm = MenuManager::GetSingleton();
		UIStringHolder *holder = UIStringHolder::GetSingleton();

		if (evn->menuName == holder->loadingMenu && evn->IsClose() && !mm->IsMenuOpen(holder->mainMenu))
		{
			FloatingDamage *FloatingDamage = FloatingDamage::GetSingleton();
			if (!FloatingDamage)
			{
				UIManager *ui = UIManager::GetSingleton();
				ui->AddMessage("Floating Damage", UIMessage::kMessage_Open, nullptr);
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
			AutoOpenFloatingDamageSink *sink = new AutoOpenFloatingDamageSink();
			if (sink)
			{
				mm->BSTEventSource<MenuOpenCloseEvent>::AddEventSink(sink);
			}
		}
	}
};

FloatingDamage * FloatingDamage::ms_pSingleton = nullptr;
BSSpinLock FloatingDamage::ms_lock;

FloatingDamage::FloatingDamage()
{
	const char swfName[] = "FloatingDamage";

	if (LoadMovie(swfName, GFxMovieView::SM_NoScale, 0.0f))
	{
		_MESSAGE("loaded Inteface/%s.swf", swfName);

		menuDepth = 2;
		flags = kType_DoNotDeleteOnClose | kType_DoNotPreventGameSave | kType_Unk10000;
	}
}

FloatingDamage::~FloatingDamage()
{
}

bool FloatingDamage::Register()
{
	MenuManager *mm = MenuManager::GetSingleton();
	if (!mm)
		return false;

	mm->Register("Floating Damage", []() -> IMenu * { return new FloatingDamage; });

	AutoOpenFloatingDamageSink::Register();

	return true;
}

void FloatingDamage::PopupText(NiPoint3 pos, int number, UInt32 alpha, HitType flags, bool isEnemy, bool dispersion, bool track, UInt32 formId)
{
	BSSpinLockGuard guard(ms_lock);

	if (view)
	{
		GFxValue argText;
		view->CreateArray(&argText);
		GFxValue argSize;
		view->CreateArray(&argSize);
		GFxValue argColor;
		view->CreateArray(&argColor);

		bool isDamage;
		std::string texts[4];

		if ((flags & HitType::Hit_Heal) != 0)
		{
			isDamage = false;
			texts[0] = ReplaceString(ini.Heal.Text, number);
			if (texts[0] != "")
			{
				GFxValue text = texts[0].c_str();
				GFxValue size = (double)ini.Heal.Size;
				GFxValue color = isEnemy ? (double)ini.Heal.EnemyColor : (double)ini.Heal.FollowerColor;
				argText.PushBack(text);
				argSize.PushBack(size);
				argColor.PushBack(color);
			}
		}
		else
		{
			isDamage = true;
			if ((flags & HitType::Hit_Miss) != 0)
			{
				texts[0] = ReplaceString(ini.Miss.Text, number);
				if (texts[0] != "")
				{
					GFxValue text = texts[0].c_str();
					GFxValue size = (double)ini.Miss.Size;
					GFxValue color = isEnemy ? (double)ini.Miss.EnemyColor : (double)ini.Miss.FollowerColor;
					argText.PushBack(text);
					argSize.PushBack(size);
					argColor.PushBack(color);
				}
			}
			else if ((flags & HitType::Hit_DoT) != 0)
			{
				texts[0] = ReplaceString(ini.DoT.Text, number);
				if (texts[0] != "")
				{
					GFxValue text = texts[0].c_str();
					GFxValue size = (double)ini.DoT.Size;
					GFxValue color = isEnemy ? (double)ini.DoT.EnemyColor : (double)ini.DoT.FollowerColor;
					argText.PushBack(text);
					argSize.PushBack(size);
					argColor.PushBack(color);
				}
			}
			else
			{
				bool hasNumber = false;
				bool overrideSize = false;
				bool overrideColor = false;

				texts[0] = ReplaceString(ini.Damage.Text, number);
				if (texts[0] != "")
				{
					GFxValue text = texts[0].c_str();
					GFxValue size = (double)ini.Damage.Size;
					GFxValue color = isEnemy ? (double)ini.Damage.EnemyColor : (double)ini.Damage.FollowerColor;
					if (argText.PushBack(text) && argSize.PushBack(size) && argColor.PushBack(color))
						hasNumber = true;
				}

				if ((flags & HitType::Hit_Critical) != 0)
				{
					if ((ini.CriticalFlags & 1) != 0)
					{
						texts[1] = ReplaceString(ini.Critical.Text, number);
						if (texts[1] != "")
						{
							GFxValue text = texts[1].c_str();
							GFxValue size = (double)ini.Critical.Size;
							GFxValue color = isEnemy ? (double)ini.Critical.EnemyColor : (double)ini.Critical.FollowerColor;
							argText.PushBack(text);
							argSize.PushBack(size);
							argColor.PushBack(color);
						}
					}

					if ((ini.CriticalFlags & 2) != 0 && hasNumber)
					{
						GFxValue size = (double)ini.Critical.Size;
						argSize.SetElement(0, size);
						overrideSize = true;
					}

					if ((ini.CriticalFlags & 4) != 0 && hasNumber)
					{
						GFxValue color = isEnemy ? (double)ini.Critical.EnemyColor : (double)ini.Critical.FollowerColor;
						argColor.SetElement(0, color);
						overrideColor = true;
					}
				}

				if ((flags & HitType::Hit_Sneak) != 0)
				{
					if ((ini.SneakFlags & 1) != 0)
					{
						texts[2] = ReplaceString(ini.Sneak.Text, number);
						if (texts[2] != "")
						{
							GFxValue text = texts[2].c_str();
							GFxValue size = (double)ini.Sneak.Size;
							GFxValue color = isEnemy ? (double)ini.Sneak.EnemyColor : (double)ini.Sneak.FollowerColor;
							argText.PushBack(text);
							argSize.PushBack(size);
							argColor.PushBack(color);
						}
					}

					if ((ini.SneakFlags & 2) != 0 && hasNumber && !overrideSize)
					{
						GFxValue size = (double)ini.Sneak.Size;
						argSize.SetElement(0, size);
						overrideSize = true;
					}

					if ((ini.SneakFlags & 4) != 0 && hasNumber && !overrideColor)
					{
						GFxValue color = isEnemy ? (double)ini.Sneak.EnemyColor : (double)ini.Sneak.FollowerColor;
						argColor.SetElement(0, color);
						overrideColor = true;
					}
				}

				if ((flags & HitType::Hit_Block) != 0)
				{
					if ((ini.BlockFlags & 1) != 0)
					{
						texts[3] = ReplaceString(ini.Block.Text, number);
						if (texts[3] != "")
						{
							GFxValue text = texts[3].c_str();
							GFxValue size = (double)ini.Block.Size;
							GFxValue color = isEnemy ? (double)ini.Block.EnemyColor : (double)ini.Block.FollowerColor;
							argText.PushBack(text);
							argSize.PushBack(size);
							argColor.PushBack(color);
						}
					}

					if ((ini.BlockFlags & 2) != 0 && hasNumber && !overrideSize)
					{
						GFxValue size = (double)ini.Block.Size;
						argSize.SetElement(0, size);
						overrideSize = true;
					}

					if ((ini.BlockFlags & 4) != 0 && hasNumber && !overrideColor)
					{
						GFxValue color = isEnemy ? (double)ini.Block.EnemyColor : (double)ini.Block.FollowerColor;
						argColor.SetElement(0, color);
						overrideColor = true;
					}
				}
			}
		}

		if (argText.GetArraySize() > 0)
		{
			if (!track)
			{
				double distance;
				if (pos.z >= 1.0f)
					distance = 999999999.0;
				else
					distance = 1.0 / (1.0f - pos.z);
				double scale = (40.0 / distance) * 100.0;
				if (scale > 150.0)
					scale = 150.0;
				else if (scale < 75.0)
					scale = 75.0;

				if (dispersion)
				{
					static std::random_device rd;
					static std::mt19937 mt(rd());
					static std::uniform_real_distribution<double> score(-0.02, 0.02);
					pos.x += score(mt) * scale / 100.0;
					pos.y += score(mt) * scale / 100.0;
				}

				std::vector<GFxValue> args;
				args.reserve(8);
				args.push_back(argText);
				args.push_back(argSize);
				args.push_back(argColor);
				args.emplace_back((double)pos.x);
				args.emplace_back((double)pos.y);
				args.emplace_back(scale);
				args.emplace_back((double)alpha);
				args.emplace_back(isDamage);

				view->Invoke("_root.widget.PopupText", nullptr, &args[0], args.size());
			}
			else
			{
				std::vector<GFxValue> args;
				args.reserve(9);
				args.push_back(argText);
				args.push_back(argSize);
				args.push_back(argColor);
				args.emplace_back((double)pos.x);
				args.emplace_back((double)pos.y);
				args.emplace_back((double)pos.z);
				args.emplace_back((double)formId);
				args.emplace_back((double)alpha);
				args.emplace_back(isDamage);

				view->Invoke("_root.widget.PopupTextTrack", nullptr, &args[0], args.size());
			}
		}
	}
}

UInt32 FloatingDamage::ProcessMessage(UIMessage *message)
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

void FloatingDamage::Render()
{
	IMenu::Render();
}

void FloatingDamage::OnMenuOpen()
{
	BSSpinLockGuard guard(ms_lock);

	if (!ms_pSingleton)
	{
		ms_pSingleton = this;

		if (view)
		{
			GFxValue args[3];
			args[0] = ini.FontName.c_str();
			args[1] = (double)ini.DamageAnimation;
			args[2] = (double)ini.HealAnimation;

			view->Invoke("_root.widget.Setup", nullptr, args, 3);
		}
	}
}

void FloatingDamage::OnMenuClose()
{
	BSSpinLockGuard guard(ms_lock);

	if (ms_pSingleton)
	{
		ms_pSingleton = nullptr;
	}

	AutoOpenFloatingDamageSink::Register();
}