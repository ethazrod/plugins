#include <SKSE.h>
#include <SKSE/PluginAPI.h>
#include <SKSE/DebugLog.h>
#include <SKSE/GameRTTI.h>
#include <SKSE/HookUtil.h>
#include <SKSE/GameData.h>
#include <SKSE/GameForms.h>
#include <SKSE/GameObjects.h>
#include <SKSE/GameReferences.h>
#include <Skyrim/BSMain/Setting.h>

#include "ConditionFunctions.h"

#include <shlwapi.h> // PathRemoveFileSpec
#pragma comment( lib, "shlwapi.lib" )
#include <map>
#include <unordered_map>
#include <algorithm>
#include <string>

static bool initialized = false;

static void ToLower(std::string &str)
{
	for (auto &c : str)
		c = tolower(c);
}

static void Trim(std::string& str, const char* trimCharacterList = " \t")
{
	std::string::size_type left = str.find_first_not_of(trimCharacterList);
	if (left != std::string::npos)
	{
		std::string::size_type right = str.find_last_not_of(trimCharacterList);
		str = str.substr(left, right - left + 1);
	}
	else
	{
		str = "";
	}
}

static std::vector<std::string> Split(const std::string &str, char sep)
{
	std::vector<std::string> result;

	auto first = str.begin();
	while (first != str.end())
	{
		auto last = first;
		while (last != str.end() && *last != sep)
			++last;
		result.push_back(std::string(first, last));
		if (last != str.end())
			++last;
		first = last;
	}
	return result;
}

static std::vector<std::string> Split(const std::string &str, char sep, char exc)
{
	std::vector<std::string> result;

	bool inside = false;
	auto first = str.begin();
	while (first != str.end())
	{
		auto last = first;
		while (last != str.end())
		{
			if (*last == exc)
				inside = !inside;

			if (*last != sep || inside)
				++last;
			else
				break;
		}

		result.push_back(std::string(first, last));
		if (last != str.end())
			++last;
		first = last;
	}
	return result;
}

static bool StartsWith(const std::string &str, std::string pat)
{
	return str.size() >= pat.size() && std::equal(std::begin(pat), std::end(pat), std::begin(str));
}

static bool EndsWith(const std::string &str, std::string pat)
{
	return str.size() >= pat.size() && std::equal(std::rbegin(pat), std::rend(pat), std::rbegin(str));
}

static std::string GetSkyrimPath()
{
	std::string result;
	char buf[MAX_PATH];
	GetModuleFileName(nullptr, buf, MAX_PATH);
	PathRemoveFileSpec(buf);
	result = buf;
	result += "\\";
	return result;
}

static bool IsTargetFile(std::string name, std::string extension)
{
	ToLower(name);
	return EndsWith(name, extension);
}

static bool GetFileFolderNames(std::string folderPath, std::vector<std::string> &out_names, bool file, bool all, std::string extension = "", std::string relativePath = "")
{
	HANDLE hFind;
	WIN32_FIND_DATA win32fd;
	std::string search_name = folderPath + "\\*";
	hFind = FindFirstFile(search_name.c_str(), &win32fd);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	do {
		if (win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if ('.' != win32fd.cFileName[0] || ('\0' != win32fd.cFileName[1] && ('.' != win32fd.cFileName[1] || '\0' != win32fd.cFileName[2])))
			{
				if (!file)
				{
					std::string name = relativePath + win32fd.cFileName;
					out_names.push_back(name);
				}

				if (all)
				{
					std::string folder = folderPath;
					folder += "\\";
					folder += win32fd.cFileName;
					std::string relative = relativePath;
					relative += win32fd.cFileName;
					relative += "\\";
					GetFileFolderNames(folder, out_names, file, all, extension, relative);
				}
			}
		}
		else
		{
			if (file)
			{
				if (extension.empty() || IsTargetFile(win32fd.cFileName, extension))
				{
					std::string name = relativePath + win32fd.cFileName;
					out_names.push_back(name);
				}
			}
		}
	} while (FindNextFile(hFind, &win32fd));

	FindClose(hFind);

	return true;
}

static UInt8 GetOrderIDByModName(std::string name)
{
	TESDataHandler* dhnd = TESDataHandler::GetSingleton();
	return (!dhnd || name.empty()) ? 0xFF : dhnd->GetModIndex(name.c_str());
}

class hkStringPtr
{
public:
	hkStringPtr() : m_string(nullptr)
	{
	}

	hkStringPtr(const char* string)
	{
		UInt32 len = strlen(string) + 1;
		m_string = new char[len];
		strcpy_s(m_string, len, string);
		m_string = reinterpret_cast<char*>(UInt32(m_string) | HEAP_FLAG);
	}

	const char* val() const
	{
		return reinterpret_cast<char*>(UInt32(m_string) & ~FLAGS_MASK);
	}

	~hkStringPtr()
	{
		if ((UInt32(m_string) & HEAP_FLAG) == HEAP_FLAG)
		{
			delete[] reinterpret_cast<char*>(UInt32(m_string) & ~FLAGS_MASK);
			m_string = nullptr;
		}
	}

	hkStringPtr& operator=(const char* string)
	{
		if (string == val())
			return *this;

		if ((UInt32(m_string) & HEAP_FLAG) == HEAP_FLAG)
			delete[] reinterpret_cast<char*>(UInt32(m_string) & ~FLAGS_MASK);

		if (string)
		{
			UInt32 len = strlen(string) + 1;
			m_string = new char[len];
			strcpy_s(m_string, len, string);
			m_string = reinterpret_cast<char*>(UInt32(m_string) | HEAP_FLAG);
		}
		else
		{
			m_string = nullptr;
		}

		return *this;
	}

	hkStringPtr& operator=(hkStringPtr stringPtr)
	{
		if (m_string == stringPtr.m_string)
			return *this;

		if ((UInt32(m_string) & HEAP_FLAG) == HEAP_FLAG)
			delete[] reinterpret_cast<char*>(UInt32(m_string) & ~FLAGS_MASK);

		const char* string = stringPtr.val();
		if (string)
		{
			UInt32 len = strlen(string) + 1;
			m_string = new char[len];
			strcpy_s(m_string, len, string);
			m_string = reinterpret_cast<char*>(UInt32(m_string) | HEAP_FLAG);
		}
		else
		{
			m_string = nullptr;
		}

		return *this;
	}

	enum StringFlags
	{
		HEAP_FLAG = 0x1,
		FLAGS_MASK = 0x1
	};

private:
	char* m_string;
};

struct StrArrayData
{
	hkStringPtr* strArray;
	UInt32 strArraySize;

	StrArrayData(hkStringPtr* strArray, UInt32 strArraySize) : strArray(strArray), strArraySize(strArraySize)
	{
	}
};

static const UInt32 maxStrArraySize = 0x4000;
static std::unordered_map<UInt32, StrArrayData> strArrayMap;
static BSSpinLock strArrayLock;

static void InsertStrArray(UInt32 characterStringData)
{
	BSSpinLockGuard guard(strArrayLock);

	strArrayMap.insert(std::make_pair(characterStringData, StrArrayData(*(hkStringPtr**)((char*)characterStringData + 0x20), *(UInt32*)((char*)characterStringData + 0x24))));
}

static void RevertStrArray(UInt32 characterStringData)
{
	BSSpinLockGuard guard(strArrayLock);

	auto it = strArrayMap.find(characterStringData);
	if (it != strArrayMap.end())
	{
		delete[] * (hkStringPtr**)((char*)characterStringData + 0x20);
		*(hkStringPtr**)((char*)characterStringData + 0x20) = it->second.strArray;
		*(UInt32*)((char*)characterStringData + 0x24) = it->second.strArraySize;
		strArrayMap.erase(it);
	}
}

class hkbCharacterStringDataEx
{
public:
	typedef UInt32(hkbCharacterStringDataEx::*FnDtor)(UInt32);

	static FnDtor fnDtorOld;

	UInt32 Dtor_Hook(UInt32 arg1)
	{
		RevertStrArray((UInt32)this);
		return (this->*fnDtorOld)(arg1);
	}

	static void InitHook()
	{
		fnDtorOld = SafeWrite32(0x01141504 + 0x0 * 4, &Dtor_Hook);
	}
};
hkbCharacterStringDataEx::FnDtor hkbCharacterStringDataEx::fnDtorOld;

struct AnimFile
{
	AnimFile(std::string cmpPath, std::string fullPath) : fullPath(fullPath), cmpPath(cmpPath)
	{
	}

	std::string cmpPath;
	std::string fullPath;
};

struct BaseAnimFile : AnimFile
{
	BaseAnimFile(std::string cmpPath, std::string fullPath, UInt32 baseId) : AnimFile(cmpPath, fullPath), baseId(baseId)
	{
	}

	UInt32 baseId;
};

class CustomCondition
{
public:
	CustomCondition(FnCondition func, std::vector<UInt32>& args, UInt32 directlyFlags, bool not, bool and, bool ignore) : func(func), args(args), directlyFlags(directlyFlags), not(not), and (and), ignore(ignore)
	{
	}

	bool Match(Actor* actor)
	{
		if (ignore)
			return false;

		return func(actor, &args, directlyFlags);
	}

	inline bool IsNot()
	{
		return not;
	}

	inline bool IsAnd()
	{
		return and;
	}

private:
	FnCondition func;
	std::vector<UInt32> args;
	UInt32 directlyFlags;
	bool not;
	bool and;
	bool ignore;
};

class CustomConditions
{
public:
	CustomConditions()
	{
	}

	void Add(FnCondition func, std::vector<UInt32>& args, UInt32 directlyFlags, bool not, bool and, bool ignore)
	{
		conditions.emplace_back(func, args, directlyFlags, not, and, ignore);
	}

	bool Match(Actor* actor)
	{
		bool skip = false;

		for (auto& condition : conditions)
		{
			if (skip)
			{
				if (condition.IsAnd())
					skip = false;
			}
			else
			{
				if (condition.Match(actor) == condition.IsNot())
				{
					if (condition.IsAnd())
						return false;
				}
				else
				{
					if (!condition.IsAnd())
						skip = true;
				}
			}
		}

		return true;
	}

private:
	std::vector<CustomCondition> conditions;
};

struct ConditionAnimFile : AnimFile
{
	ConditionAnimFile(std::string cmpPath, std::string fullPath, int priority, CustomConditions* customConditions) : AnimFile(cmpPath, fullPath), priority(priority), customConditions(customConditions)
	{
	}

	int priority;
	CustomConditions* customConditions;
};

class LinkData
{
public:
	LinkData()
	{
	}

	virtual UInt16 FindLinkId(Actor* actor) = 0;
	virtual UInt16 GetLinkId(Actor* actor) = 0;
};

class BaseLinkData : public LinkData
{
public:
	BaseLinkData()
	{
	}

	void Add(UInt32 formId, UInt16 linkId)
	{
		linkIdMap.insert(std::make_pair(formId, linkId));
	}

	virtual UInt16 FindLinkId(Actor* actor) override
	{
		TESNPC* base = actor->GetActorBase();
		if (!base)
			return 0xFFFF;

		auto it = linkIdMap.find(base->formID);
		if (it != linkIdMap.end())
			return it->second;

		return 0xFFFF;
	}

	virtual UInt16 GetLinkId(Actor* actor) override
	{
		return FindLinkId(actor);
	}

private:
	std::unordered_map<UInt32, UInt16> linkIdMap;
};

class ConditionLinkData : public LinkData
{
public:
	ConditionLinkData(CustomConditions * customConditions, UInt16 linkId) : customConditions(customConditions), linkId(linkId)
	{
	}

	virtual UInt16 FindLinkId(Actor* actor) override
	{
		if (customConditions->Match(actor))
			return linkId;

		return 0xFFFF;
	}

	virtual UInt16 GetLinkId(Actor* actor) override
	{
		return linkId;
	}

private:
	CustomConditions * customConditions;
	UInt16 linkId;
};

struct AnimData
{
	std::unordered_map<UInt16, std::map<int, LinkData*>> linkDataMap;
	std::vector<BaseAnimFile> baseAnimFiles;
	std::vector<ConditionAnimFile> conditionAnimFiles;
	std::string folderName;
	UInt32 projectData;
	bool notification;

	AnimData(std::string folderName) : folderName(folderName), projectData(0), notification(false)
	{
	}

	void ClearLinkDataMap()
	{
		for (auto& linkData : linkDataMap)
		{
			for (auto& linkData2 : linkData.second)
			{
				delete linkData2.second;
			}
		}

		linkDataMap.clear();
	}

	UInt16 FindLinkId(UInt16 id, Actor* actor)
	{
		auto it = linkDataMap.find(id);
		if (it != linkDataMap.end())
		{
			for (auto it2 = it->second.rbegin(), it2e = it->second.rend(); it2 != it2e; ++it2)
			{
				UInt16 linkId = it2->second->FindLinkId(actor);
				if (linkId != 0xFFFF)
					return linkId;
			}
		}

		return 0xFFFF;
	}

	void GetAllLinkId(Actor* actor, std::vector<UInt16>* linkId_list)
	{
		for (auto& linkData : linkDataMap)
		{
			for (auto& linkData2 : linkData.second)
			{
				UInt16 linkId = linkData2.second->GetLinkId(actor);
				if (linkId != 0xFFFF)
					linkId_list->push_back(linkId);
			}
		}
	}
};

static std::map<std::string, AnimData> AnimDataMap;

static AnimData* GetAnimData(UInt32 projectData)
{
	if (projectData == 0 || !initialized)
		return nullptr;

	for (auto& pair : AnimDataMap)
	{
		if (pair.second.projectData == projectData)
			return &pair.second;
	}

	return nullptr;
}

static void ResetAnimData(UInt32 projectData)
{
	if (!initialized)
		return;

	for (auto& pair : AnimDataMap)
	{
		if (pair.second.projectData == projectData)
			pair.second.projectData = 0;
	}
}

class hkbProjectDataEx
{
public:
	typedef UInt32(hkbProjectDataEx::*FnDtor)(UInt32);

	static FnDtor fnDtorOld;

	UInt32 Dtor_Hook(UInt32 arg1)
	{
		ResetAnimData((UInt32)this);
		return (this->*fnDtorOld)(arg1);
	}

	static void InitHook()
	{
		fnDtorOld = SafeWrite32(0x011467DC + 0x0 * 4, &Dtor_Hook);
	}
};
hkbProjectDataEx::FnDtor hkbProjectDataEx::fnDtorOld;

class hkbClipGenerator
{
public:
	UInt32 vtbl;
	UInt32 unk04[(0x48 - 0x4) >> 2];
	UInt16 unk48;
	UInt16 unk4A;
	UInt32 unk4C[(0x68 - 0x4C) >> 2];
	UInt32 unk68;

	hkbClipGenerator(UInt16 id)
	{
		for (int i = 0x4; i < 0x6C; i += 4)
		{
			*(UInt32*)((char*)this + i) = 0;
		}

		vtbl = 0x01144C5C;
		unk48 = id;
	}
};

class hkbClipGeneratorEx : public hkbClipGenerator
{
public:
	typedef void(hkbClipGeneratorEx::*FnActivate)(UInt32*);

	static FnActivate fnActivateOld;

	void Activate_Hook(UInt32* characterPtr)
	{
		UInt16 id = unk48;
		if (id != 0xFFFF)
		{
			UnkClass_011250F0* unk011250F0 = (UnkClass_011250F0*)(*characterPtr - 0x68);
			AnimData* data = GetAnimData(*(UInt32*)((char*)unk011250F0 + 0x9C));
			if (data)
			{
				TESObjectREFR* ref = *(TESObjectREFR**)((char*)unk011250F0 + 0x11C);
				if (ref && ref->formType == FormType::Character)
				{
					UInt16 linkId = data->FindLinkId(id, static_cast<Actor*>(ref));
					if (linkId != 0xFFFF)
					{
						unk48 = linkId;
						(this->*fnActivateOld)(characterPtr);
						unk48 = id;
						return;
					}
				}
			}
		}

		(this->*fnActivateOld)(characterPtr);
	}

	static void InitHook()
	{
		fnActivateOld = SafeWrite32(0x01144C5C + 0x4 * 4, &Activate_Hook);
	}
};
hkbClipGeneratorEx::FnActivate hkbClipGeneratorEx::fnActivateOld;

class UnkClass_0112B2A4
{
public:
	virtual UInt32	unk00(UInt32) = 0;								//00BCE290
	virtual bool	unk01(UInt32*, hkbClipGenerator*, UInt32) = 0;	//00BCBBA0
	virtual bool	unk02(UInt32*, hkbClipGenerator*, UInt32) = 0;	//00BCC2D0
	virtual void	unk03(UInt32*, hkbClipGenerator*, UInt32) = 0;	//00BCC4B0
	virtual bool	unk04(UInt32*, UInt32 id) = 0;					//00BCBD00
	virtual void	unk05(UInt32, UInt32) = 0;						//00BCB5E0

	void Hook_unk02(UInt32* characterPtr, hkbClipGeneratorEx* gen, UInt32 arg3)
	{
		UInt16 id = gen->unk48;
		if (id != 0xFFFF)
		{
			UnkClass_011250F0* unk011250F0 = (UnkClass_011250F0*)(*characterPtr - 0x68);
			AnimData* data = GetAnimData(*(UInt32*)((char*)unk011250F0 + 0x9C));
			if (data)
			{
				TESObjectREFR* ref = *(TESObjectREFR**)((char*)unk011250F0 + 0x11C);
				if (ref && ref->formType == FormType::Character)
				{
					UInt16 linkId = data->FindLinkId(id, static_cast<Actor*>(ref));
					if (linkId != 0xFFFF)
					{
						gen->unk48 = linkId;
						if (unk02(characterPtr, gen, arg3))
							(gen->*hkbClipGeneratorEx::fnActivateOld)(characterPtr);

						gen->unk48 = id;
						return;
					}
				}
			}
		}

		if (unk02(characterPtr, gen, arg3))
			(gen->*hkbClipGeneratorEx::fnActivateOld)(characterPtr);
	}
};

static UInt32 Addr_00BF2269 = 0x00BF2269;

static void __declspec(naked) Hook_00BF2259(void)
{
	__asm
	{
		call	UnkClass_0112B2A4::Hook_unk02
		jmp		Addr_00BF2269
	}
}

static void PreloadAnimation(TESObjectREFR* ref)
{
	if (ref && ref->formType == FormType::Character)
	{
		Actor* actor = static_cast<Actor*>(ref);
		BSAnimationGraphManagerPtr out;
		if (actor->GetAnimationGraphManager(out))
		{
			UnkClass_0112B2A4 *& unk0112B2A4 = *(UnkClass_0112B2A4**)0x01BA3D20;
			if (unk0112B2A4)
			{
				BSSpinLockGuard guard(out->lock);

				for (auto& unk011250F0 : out->unk20)
				{
					UInt32 character = (UInt32)((char*)unk011250F0 + 0x68);
					AnimData* data = GetAnimData(*(UInt32*)((char*)unk011250F0 + 0x9C));
					if (data)
					{
						std::vector<UInt16> linkId_list;
						data->GetAllLinkId(actor, &linkId_list);
						for (auto& linkId : linkId_list)
						{
							hkbClipGenerator gen(linkId);
							unk0112B2A4->unk01(&character, &gen, 0);
						}
					}
				}
			}
		}
	}
}

class QueuedReferenceAnimationTaskEx
{
public:
	DEFINE_MEMBER_FN(Fn00651160, bool, 0x00651160);

	bool Hook_AnimationTask()
	{
		bool retn = Fn00651160();
		if (retn)
			PreloadAnimation(*(TESObjectREFR**)((char*)this + 0x280));

		return retn;
	}
};

class IAnimationGraphManagerHolderEx2 : public IAnimationGraphManagerHolder
{
public:
	DEFINE_MEMBER_FN(Fn00650D20, bool, 0x00650D20, UInt32);

	bool Hook_AnimationHolder(UInt32 arg1)
	{
		bool retn = Fn00650D20(arg1);
		if (retn)
			PreloadAnimation((TESObjectREFR*)((char*)this - 0x20));

		return retn;
	}
};

static void Hook_00BC296B(UInt32 ecx, UInt32* stack)
{
	if (!initialized)
		return;

	UInt32 unkPtr = stack[1];
	UInt32 projectData = stack[2];
	UInt32 characterData = stack[3];
	if (unkPtr != 0 && projectData != 0 && characterData != 0)
	{
		const char* path1 = (const char*)((char*)unkPtr + 0x0);
		const char* path2 = (const char*)((char*)unkPtr + 0x110);
		if (path1 && path2)
		{
			std::string path = path1;
			path += "\\";
			path += path2;
			ToLower(path);
			auto it_pair = AnimDataMap.find(path);
			if (it_pair != AnimDataMap.end())
			{
				AnimData* data = &it_pair->second;
				UInt32 characterStringData = *(UInt32*)((char*)characterData + 0x74);
				if (characterStringData != 0)
				{
					hkStringPtr* strArray = *(hkStringPtr**)((char*)characterStringData + 0x20);
					UInt32 size = *(UInt32*)((char*)characterStringData + 0x24);
					if (size != 0)
					{
						typedef std::pair<UInt32, BaseAnimFile*> BaseLinkPair;
						std::vector<BaseLinkPair> baseLinks;
						typedef std::pair<UInt32, ConditionAnimFile*> ConditionLinkPair;
						std::vector<ConditionLinkPair> conditionLinks;
						for (int i = 0; i < size; i++)
						{
							std::string str = strArray[i].val();
							ToLower(str);
							for (auto& file : data->baseAnimFiles)
							{
								if (str == file.cmpPath)
									baseLinks.push_back(std::make_pair(i, &file));
							}

							for (auto& file : data->conditionAnimFiles)
							{
								if (str == file.cmpPath)
									conditionLinks.push_back(std::make_pair(i, &file));
							}
						}

						UInt32 new_strArraySize = size + baseLinks.size() + conditionLinks.size();
						if (!data->notification)
						{
							data->notification = true;
							if (new_strArraySize > maxStrArraySize)
							{
								_MESSAGE("Too many animation files. %d / %d : %s", new_strArraySize, maxStrArraySize, it_pair->first.c_str());

								static std::string strNewLine = "\n";
								std::string str = "Too many animation files.";
								str += strNewLine;
								str += std::to_string(new_strArraySize);
								str += " / ";
								str += std::to_string(maxStrArraySize);
								str += strNewLine;
								str += it_pair->first;

								typedef void(*FnDebug_MessageBox)(const char *, UInt32, UInt32, UInt32, UInt32, const char *, UInt32);
								const FnDebug_MessageBox fnDebug_MessageBox = (FnDebug_MessageBox)0x0087AC60;
								fnDebug_MessageBox(str.c_str(), 0, 0, 4, 0x0A, "OK", 0);
							}
							else
							{
								_MESSAGE("%d / %d : %s", new_strArraySize, maxStrArraySize, it_pair->first.c_str());
							}
						}

						if (size <= maxStrArraySize)
						{
							data->ClearLinkDataMap();
							hkStringPtr* new_strArray = new hkStringPtr[maxStrArraySize];
							if (new_strArraySize <= maxStrArraySize)
							{
								for (int i = 0; i < baseLinks.size(); i++)
								{
									new_strArray[i] = baseLinks[i].second->fullPath.c_str();
									UInt16 id = maxStrArraySize - size + baseLinks[i].first;
									auto it = data->linkDataMap.find(id);
									if (it == data->linkDataMap.end())
									{
										std::map<int, LinkData*> linkDataPair;
										BaseLinkData* linkData = new BaseLinkData();
										linkData->Add(baseLinks[i].second->baseId, (UInt16)i);
										linkDataPair.insert(std::make_pair(0, linkData));
										data->linkDataMap.insert(std::make_pair(id, linkDataPair));
									}
									else
									{
										BaseLinkData* linkData = static_cast<BaseLinkData*>(it->second.at(0));
										linkData->Add(baseLinks[i].second->baseId, (UInt16)i);
									}
								}

								for (int i = 0; i < conditionLinks.size(); i++)
								{
									UInt16 idx = (UInt16)(baseLinks.size() + i);
									new_strArray[idx] = conditionLinks[i].second->fullPath.c_str();
									int prio = conditionLinks[i].second->priority;
									UInt16 id = maxStrArraySize - size + conditionLinks[i].first;
									auto it = data->linkDataMap.find(id);
									if (it == data->linkDataMap.end())
									{
										std::map<int, LinkData*> linkDataPair;
										ConditionLinkData* linkData = new ConditionLinkData(conditionLinks[i].second->customConditions, idx);
										linkDataPair.insert(std::make_pair(prio, linkData));
										data->linkDataMap.insert(std::make_pair(id, linkDataPair));
									}
									else
									{
										auto it2 = it->second.find(prio);
										if (it2 == it->second.end())
										{
											ConditionLinkData* linkData = new ConditionLinkData(conditionLinks[i].second->customConditions, idx);
											it->second.insert(std::make_pair(prio, linkData));
										}
										else
										{
											static bool error_notification = false;
											if (!error_notification)
											{
												error_notification = true;
												_MESSAGE("couldn't add conditions");
											}
										}
									}
								}

								for (int i = baseLinks.size() + conditionLinks.size(); i < maxStrArraySize - size; i++)
								{
									new_strArray[i] = "";
								}
							}
							else
							{
								for (int i = 0; i < maxStrArraySize - size; i++)
								{
									new_strArray[i] = "";
								}
							}

							for (int i = maxStrArraySize - size, j = 0; i < maxStrArraySize; i++, j++)
							{
								new_strArray[i] = strArray[j].val();
							}

							InsertStrArray(characterStringData);
							*(hkStringPtr**)((char*)characterStringData + 0x20) = new_strArray;
							*(UInt32*)((char*)characterStringData + 0x24) = maxStrArraySize;
							data->projectData = projectData;
						}
					}
				}
			}
		}
	}
}

static void AddAnimData(std::string str)
{
	std::string pathString = str;
	if (!pathString.empty())
	{
		ToLower(pathString);
		auto it = AnimDataMap.find(pathString);
		if (it == AnimDataMap.end())
		{
			auto pos = pathString.rfind('\\');
			if (pos != std::string::npos && pos != 0)
			{
				std::string folderName = pathString.substr(0, pos);
				AnimDataMap.insert(std::make_pair(pathString, AnimData(folderName)));
			}
		}
	}
}

class DynamicAnimationReplacerPlugin : public SKSEPlugin
{
public:
	DynamicAnimationReplacerPlugin()
	{
	}

	virtual bool InitInstance() override
	{
		if (!Requires(kSKSEVersion_1_7_3))
			return false;

		SetName("DynamicAnimationReplacer");
		SetVersion(1);

		return true;
	}

	virtual bool OnLoad() override
	{
		SKSEPlugin::OnLoad();

		return true;
	}

	virtual void OnModLoaded() override
	{
		TESDataHandler* dhnd = TESDataHandler::GetSingleton();
		if (!dhnd)
		{
			_MESSAGE("couldn't get TESDataHandler");
			return;
		}

		hkbCharacterStringDataEx::InitHook();
		hkbProjectDataEx::InitHook();
		hkbClipGeneratorEx::InitHook();
		WriteRelJump(0x00BF2259, &Hook_00BF2259);
		WriteRelCall(0x0042C2D5, &QueuedReferenceAnimationTaskEx::Hook_AnimationTask);
		WriteRelCall(0x004E9692, &IAnimationGraphManagerHolderEx2::Hook_AnimationHolder);
		WriteRelCall(0x00651101, &IAnimationGraphManagerHolderEx2::Hook_AnimationHolder);
		WriteRelCall(0x0065136C, &IAnimationGraphManagerHolderEx2::Hook_AnimationHolder);
		HookRelCall(0x00BC296B, Hook_00BC296B);

		for (auto& race : dhnd->races)
		{
			if (race)
			{
				for (int i = 0; i < 2; i++)
				{
					std::string pathString = race->behaviorGraph[i].modelName;
					AddAnimData(pathString);
				}
			}
		}

		Setting	* setting = g_iniSettingCollection->Get("strPlayerCharacterBehavior1stPGraph:Animation");
		if (setting && setting->GetType() == Setting::kType_String)
		{
			std::string pathString = setting->data.s;
			AddAnimData(pathString);
		}

		std::string basePath = GetSkyrimPath();
		basePath += "data\\meshes\\";
		for (auto& data : AnimDataMap)
		{
			std::string path = basePath;
			path += data.second.folderName;
			path += "\\animations\\DynamicAnimationReplacer";
			std::vector<std::string> espNames;
			if (!GetFileFolderNames(path, espNames, false, false))
			{
				_MESSAGE("couldn't find %s\\animations\\DynamicAnimationReplacer", data.second.folderName.c_str());
				continue;
			}

			std::unordered_map<std::string, UInt8> espMap;
			for (auto& esp : espNames)
			{
				if (IsTargetFile(esp, ".esp") || IsTargetFile(esp, ".esm"))
				{
					UInt8 index = GetOrderIDByModName(esp);
					if (index == 0xFF)
						_MESSAGE("esp file not loaded: %s", esp.c_str());
					else
						espMap.insert(std::make_pair(esp, index));
				}
			}

			path += "\\";
			for (auto& esp : espMap)
			{
				std::string path2 = path + esp.first;
				std::vector<std::string> formIdNames;
				GetFileFolderNames(path2, formIdNames, false, false);
				std::unordered_map<std::string, UInt32> formIdMap;
				for (auto& formId : formIdNames)
				{
					if (formId.size() == 8)
					{
						std::string formid = "0x" + formId;
						UInt32 id = 0xFFFFFFFF;
						try {
							id = std::stoi(formid, nullptr, 0);
						}
						catch (const std::invalid_argument& e)
						{
							_MESSAGE("%s : %s : invalid argument", esp.first.c_str(), formid.c_str());
							_MESSAGE("   %s", e.what());
						}
						catch (const std::out_of_range& e)
						{
							_MESSAGE("%s : %s : out of range", esp.first.c_str(), formid.c_str());
							_MESSAGE("   %s", e.what());
						}

						if (id <= 0xFFFFFF)
							formIdMap.insert(std::make_pair(formId, id));
					}
				}

				path2 += "\\";
				for (auto& formId : formIdMap)
				{
					std::string path3 = path2 + formId.first;
					std::vector<std::string> hkxNames;
					GetFileFolderNames(path3, hkxNames, true, true, ".hkx");
					for (auto& hkx : hkxNames)
					{
						std::string cmpPath = "Animations\\";
						cmpPath += hkx;
						ToLower(cmpPath);
						std::string fullPath = "Animations\\DynamicAnimationReplacer\\";
						fullPath += esp.first;
						fullPath += "\\";
						fullPath += formId.first;
						fullPath += "\\";
						fullPath += hkx;
						data.second.baseAnimFiles.emplace_back(cmpPath, fullPath, (esp.second << 24) + formId.second);
					}
				}
			}

			std::string customConditionsPath = path;
			customConditionsPath += "_CustomConditions";
			std::vector<std::string> priorityNames;
			if (GetFileFolderNames(customConditionsPath, priorityNames, false, false))
			{
				customConditionsPath += "\\";
				for (auto& priority : priorityNames)
				{
					if (!priority.empty() && priority[0] != '0' && (std::all_of(priority.cbegin(), priority.cend(), isdigit) || (priority.size() >= 2 && priority[0] == '-' && priority[1] != '0' && std::all_of(priority.cbegin() + 1, priority.cend(), isdigit))))
					{
						int prio = 0;
						try {
							prio = std::stoi(priority);
						}
						catch (const std::invalid_argument& e)
						{
							_MESSAGE("%s : invalid argument", priority.c_str());
							_MESSAGE("   %s", e.what());
						}
						catch (const std::out_of_range& e)
						{
							_MESSAGE("%s : out of range", priority.c_str());
							_MESSAGE("   %s", e.what());
						}

						if (prio != 0)
						{
							std::string path2 = customConditionsPath + priority;
							std::string conditionTxtPath = path2;
							conditionTxtPath += "\\_conditions.txt";
							std::ifstream in(conditionTxtPath.c_str());
							if (in.fail())
							{
								_MESSAGE("couldn't find %s\\animations\\DynamicAnimationReplacer\\_CustomConditions\\%s\\_conditions.txt", data.second.folderName.c_str(), priority.c_str());
							}
							else
							{
								std::vector<std::string> list_condition;
								std::string line;
								while (std::getline(in, line))
								{
									std::string str = line;
									str.erase(0, str.find_first_not_of(" \t"));
									if (str.size() && str[0] != ';')
										list_condition.push_back(str);
								}

								std::string errorStr;
								CustomConditions* customConditions = new CustomConditions();
								for (int i = 0; i < list_condition.size(); i++)
								{
									auto& condition = list_condition[i];
									std::string _condition = condition;
									std::vector<UInt32> args;
									UInt32 directlyFlags = 0;
									bool not = false;
									bool and = true;
									bool ignore = false;

									if (StartsWith(_condition, "NOT ") || StartsWith(_condition, "NOT\t"))
									{
										not = true;
										_condition.erase(0, 3);
									}

									std::string::size_type pos_func_last = _condition.find_first_of('(');
									if (pos_func_last == std::string::npos)
									{
										errorStr = condition;
										break;
									}

									std::string str_func = _condition.substr(0, pos_func_last);
									Trim(str_func);
									auto it = conditionFunctionMap.find(str_func);
									if (it == conditionFunctionMap.end())
									{
										errorStr = condition;
										break;
									}

									std::string::size_type pos_logical_first = _condition.find_last_of(')');
									if (pos_logical_first == std::string::npos || pos_logical_first < pos_func_last)
									{
										errorStr = condition;
										break;
									}

									if (pos_logical_first == _condition.size() - 1)
									{
										if (i < list_condition.size() - 1)
										{
											errorStr = condition;
											break;
										}
									}
									else
									{
										std::string str_logical = _condition.substr(pos_logical_first + 1, _condition.size() - pos_logical_first - 1);
										Trim(str_logical);
										if (str_logical.empty())
										{
											if (i < list_condition.size() - 1)
											{
												errorStr = condition;
												break;
											}
										}
										else
										{
											if (str_logical == "AND")
											{
												and = true;
											}
											else if (str_logical == "OR")
											{
												and = false;
											}
											else
											{
												errorStr = condition;
												break;
											}
										}
									}

									if (pos_func_last != pos_logical_first - 1)
									{
										std::string str_args = _condition.substr(pos_func_last + 1, pos_logical_first - pos_func_last - 1);
										Trim(str_args);
										if (!str_args.empty())
										{
											auto vec_args = Split(str_args, ',', '"');
											for (auto& arg : vec_args)
											{
												Trim(arg);
												if (!arg.empty() && arg.front() != '"')
												{
													UInt32 flag = 1;
													for (int j = 0; j < args.size(); j++)
													{
														flag *= 2;
													}

													if ((flag & it->second.allowDirectlyFlags) == 0)
													{
														errorStr = condition;
														break;
													}

													float value = std::numeric_limits<float>::quiet_NaN();
													try {
														value = std::stof(arg);
													}
													catch (const std::invalid_argument& e)
													{
														_MESSAGE("%s : invalid argument", arg.c_str());
														_MESSAGE("   %s", e.what());
													}
													catch (const std::out_of_range& e)
													{
														_MESSAGE("%s : out of range", arg.c_str());
														_MESSAGE("   %s", e.what());
													}

													if (std::isnan(value))
													{
														errorStr = condition;
														break;
													}

													directlyFlags |= flag;
													UInt32* valuePtr = (UInt32*)&value;
													args.push_back(*valuePtr);
												}
												else
												{
													auto vec = Split(arg, '|');
													if (vec.size() != 2)
													{
														errorStr = condition;
														break;
													}

													std::string str_esp = vec[0];
													Trim(str_esp);
													if (str_esp.size() <= 2 || str_esp.front() != '"' || str_esp.back() != '"')
													{
														errorStr = condition;
														break;
													}

													str_esp = str_esp.substr(1, str_esp.size() - 2);
													if (!IsTargetFile(str_esp, ".esp") && !IsTargetFile(str_esp, ".esm"))
													{
														errorStr = condition;
														break;
													}

													UInt8 index = GetOrderIDByModName(str_esp);
													if (index == 0xFF)
													{
														_MESSAGE("esp file not loaded: %s", str_esp.c_str());
														ignore = true;
													}

													std::string str_formId = vec[1];
													Trim(str_formId);
													UInt32 id = 0xFFFFFFFF;
													try {
														id = std::stoi(str_formId, nullptr, 0);
													}
													catch (const std::invalid_argument& e)
													{
														_MESSAGE("%s : invalid argument", str_formId.c_str());
														_MESSAGE("   %s", e.what());
													}
													catch (const std::out_of_range& e)
													{
														_MESSAGE("%s : out of range", str_formId.c_str());
														_MESSAGE("   %s", e.what());
													}

													if (id > 0xFFFFFF)
													{
														errorStr = condition;
														break;
													}

													args.push_back((index << 24) + id);
												}
											}
										}
									}

									if (!errorStr.empty())
										break;

									if (args.size() != it->second.numArgs)
									{
										errorStr = condition;
										break;
									}

									customConditions->Add(it->second.func, args, directlyFlags, not, and, ignore);
								}

								if (!errorStr.empty())
								{
									_MESSAGE("error: %s\\animations\\DynamicAnimationReplacer\\_CustomConditions\\%s\\_conditions.txt", data.second.folderName.c_str(), priority.c_str());
									_MESSAGE("   %s", errorStr.c_str());
									delete customConditions;
								}
								else
								{
									std::vector<std::string> hkxNames;
									GetFileFolderNames(path2, hkxNames, true, true, ".hkx");
									for (auto& hkx : hkxNames)
									{
										std::string cmpPath = "Animations\\";
										cmpPath += hkx;
										ToLower(cmpPath);
										std::string fullPath = "Animations\\DynamicAnimationReplacer\\_CustomConditions\\";
										fullPath += priority;
										fullPath += "\\";
										fullPath += hkx;
										data.second.conditionAnimFiles.emplace_back(cmpPath, fullPath, prio, customConditions);
									}
								}
							}
						}
					}
				}
			}
		}

		initialized = true;
	}

} thePlugin;