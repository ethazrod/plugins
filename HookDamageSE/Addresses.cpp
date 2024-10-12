#include "Addresses.h"
#include "versiondb.h"

#include <vector>

uintptr_t EvadeDamage_Target; //0x00626400 + 0x89
uintptr_t PhysicalDamage_Target1; //0x00626400 + 0x36A
uintptr_t PhysicalDamage_Retn1; //0x00626400 + 0x374
uintptr_t PhysicalDamage_Target2; //0x00626400 + 0x397
uintptr_t MagicDamage_Target; //0x00567A80 + 0x237
uintptr_t ActorValue_Target; //0x006210F0 + 0x14
uintptr_t FallDamage_Target; //0x0060B620 + 0xCE
uintptr_t WaterDamage_Target; //0x005D6FB0 + 0x8A3
uintptr_t ReflectDamage_Target; //0x00628C20 + 0x3DC
uintptr_t MagicHeal_Target; //0x00567A80 + 0x2D1

struct AddressID
{
	AddressID(void* addressPtr, UInt64 id) : addressPtr(addressPtr), id(id), offset(0)
	{
	}

	AddressID(void* addressPtr, UInt64 id, UInt64 offset) : addressPtr(addressPtr), id(id), offset(offset)
	{
	}

	void* addressPtr;
	UInt64 id;
	UInt64 offset;
};

bool Addresses::Init()
{
	std::vector<AddressID> addressIDs = {
		AddressID(&EvadeDamage_Target, 37633, 0x89),
		AddressID(&PhysicalDamage_Target1, 37633, 0x36A),
		AddressID(&PhysicalDamage_Retn1, 37633, 0x374),
		AddressID(&PhysicalDamage_Target2, 37633, 0x397),
		AddressID(&MagicDamage_Target, 34286, 0x237),
		AddressID(&ActorValue_Target, 37522, 0x14),
		AddressID(&FallDamage_Target, 36973, 0xCE),
		AddressID(&WaterDamage_Target, 36357, 0x8A3),
		AddressID(&ReflectDamage_Target, 37673, 0x3DC),
		AddressID(&MagicHeal_Target, 34286, 0x2D1)
	};

	VersionDb db;
	if (!db.Load())
	{
		_MESSAGE("Failed to load version database for current executable!");
		return false;
	}

	_MESSAGE("Loaded database for %s version %s.", db.GetModuleName().c_str(), db.GetLoadedVersionString().c_str());

	for (auto& addressID : addressIDs)
	{
		void* address = db.FindAddressById(addressID.id);
		if (!address)
		{
			_FATALERROR("Failed to find address!");
			return false;
		}

		*(void**)(addressID.addressPtr) = (void*)((UInt64)address + addressID.offset);
	}

	return true;
}