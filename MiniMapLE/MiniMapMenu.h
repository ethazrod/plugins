#pragma once

#include <SKSE.h>
#include <SKSE/GameMenus.h>
#include <SKSE/GameEvents.h>
#include <SKSE/NiNodes.h>
#include <Skyrim/NetImmerse/NiRenderer.h>
#include <Skyrim/BSDevices/InputManager.h>

#include <map>

enum MarkerType
{
	Marker_UnknownDoor = 0,
	Marker_KnownDoor,
	Marker_Actor,
	Marker_Dead,
	Marker_Follower,
	Marker_Hostile,
	Marker_Quest
};

class MiniMapMenu : public IMenu,
	public BSTEventSink<InputEvent*>
{
protected:
	static MiniMapMenu * ms_pSingleton;
	static BSSpinLock ms_lock;
	static BSFixedString miniMapName;
	static std::map<UInt32, TESObjectREFR*> doors;
	static UInt32* useOriginal;

	//23C
	class LocalMapCullingProcess
	{
	public:
		// @members
		BSCullingProcess	cullingProcess;					// 000
		LocalMapCamera		localMapCamera;					// 170
		BSShaderAccumulator	* shaderAccumulator;			// 1BC
		BSRenderTargetGroup	* localMapTexture;				// 1C0
		UInt32				unk1C4[(0x230 - 0x1C4) >> 2];	// 1C4
		UInt32				width;							// 230
		UInt32				height;							// 234
		NiNode				* niNode;						// 238

		DEFINE_MEMBER_FN(ctor, void, 0x00487610);
		DEFINE_MEMBER_FN(CreateMapTarget, BSRenderTargetGroup **, 0x00486590, UInt32 width, UInt32 height);
		DEFINE_MEMBER_FN(Init, void, 0x00487D20);
		DEFINE_MEMBER_FN(Process, void, 0x00487900);
	};

	struct Texture
	{
		NiRenderedTexture		* renderedLocalMapTexture;		// 00 26C 2A4
		UInt32					unk270;							// 04 270 2A8

		Texture() : renderedLocalMapTexture(nullptr), unk270(0)
		{
		}
	};

	LocalMapCullingProcess*	cullingProcess;
	UInt8 cullingProcessMemory[0x23C];
	Texture	texture;
	BSCullingProcess* cullingProcessPtr;
	bool loadTerrain;
	UInt32 visible;
	bool bHide;
	bool bZoom;
	bool isProcessing;

public:
	MiniMapMenu();
	virtual ~MiniMapMenu();

	static MiniMapMenu * GetSingleton()
	{
		return ms_pSingleton;
	}
	static bool Register(UInt32* pointer_ENB);
	static void AddDoor(UInt32 formId, TESObjectREFR* ref);

	bool IsProcessing();

protected:
	// @override IMenu
	virtual UInt32	ProcessMessage(UIMessage *message) override;
	virtual void	Render() override;
	virtual void	Unk_07() override;

	// @override BSTEventSink
	virtual EventResult ReceiveEvent(InputEvent ** evns, BSTEventSource<InputEvent*> * src) override;

	void OnMenuOpen();
	void OnMenuClose();
	bool UpdateCamera();
};