#pragma once
#include <SKSE.h>
#include <SKSE/GameMenus.h>

class TechniquesMenu : public IMenu
{
protected:
	static TechniquesMenu * ms_pSingleton;

public:
	TechniquesMenu();
	virtual ~TechniquesMenu();

	static bool Register();

	static TechniquesMenu * GetSingleton()
	{
		return ms_pSingleton;
	}

	// @override IMenu
	virtual UInt32	ProcessMessage(UIMessage *message) override;
	virtual void	Render() override;

protected:
	void OnMenuOpen();
	void OnMenuClose();
};