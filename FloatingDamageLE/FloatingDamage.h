#pragma once

#include <SKSE.h>
#include <SKSE/GameMenus.h>

enum HitType
{
	Hit_Miss = 1 << 0,
	Hit_DoT = 1 << 1,
	Hit_Critical = 1 << 2,
	Hit_Sneak = 1 << 3,
	Hit_Block = 1 << 4,
	Hit_Heal = 1 << 5
};

class FloatingDamage : public IMenu
{
protected:
	static FloatingDamage * ms_pSingleton;
	static BSSpinLock ms_lock;

public:
	FloatingDamage();
	virtual ~FloatingDamage();

	static bool Register();

	static FloatingDamage * GetSingleton()
	{
		return ms_pSingleton;
	}

	void PopupText(NiPoint3 pos, int number, UInt32 alpha, HitType flags, bool isEnemy, bool dispersion, bool track, UInt32 formId);

	// @override IMenu
	virtual UInt32	ProcessMessage(UIMessage *message) override;
	virtual void	Render() override;

protected:
	void OnMenuOpen();
	void OnMenuClose();

	static void Lock() { ms_lock.Lock(); }
	static void Unlock() { ms_lock.Unlock(); }
};