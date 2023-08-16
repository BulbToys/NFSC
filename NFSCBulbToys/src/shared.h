#pragma once
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <d3d9.h>

#include "nfsc.h"

#include "gui.h"
#include "hooks.h"
#include "patches.h"

#include "rev_count.h"

/* === Main project stuff === */

#define PROJECT_NAME "NFSC Bulb Toys"

#define MENU_KEY VK_F9

inline bool exitMainLoop = false;

/* === Utility === */

#define ASSERT(cond) if (!(cond)) *((int*)0) = 0

struct Patch
{
	char* bytes = nullptr;
	size_t len = 0;

	Patch(uintptr_t address, size_t len) : len(len)
	{
		bytes = new char[len];
		memcpy(bytes, reinterpret_cast<void*>(address), len);
	}

	~Patch()
	{
		delete[] bytes;
	}
};
inline std::unordered_map<uintptr_t, Patch*> patch_map;

struct Stopwatch
{
	uint32_t s, i;

	Stopwatch() : s(0), i(0) {}

	void Add(uint32_t fps) { s += fps; i++; }
};

template <size_t size>
struct VTable
{
	uintptr_t f[size] = { 0 };
};

void Error(const char* message, ...);

template <typename T>
inline T ReadMemory(uintptr_t address)
{
	return *reinterpret_cast<T*>(address);
}

template <typename T>
inline void WriteMemory(uintptr_t address, T value)
{
	T* memory = reinterpret_cast<T*>(address);
	*memory = value;
}

// Identical to WriteMemory, except it gets added to the patch map (for later unpatching)
template <typename T>
inline void PatchMemory(uintptr_t address, T value)
{
	Patch* p = new Patch(address, sizeof(T));
	patch_map.insert({ address, p });

	T* memory = reinterpret_cast<T*>(address);
	*memory = value;
}

inline void PatchNop(uintptr_t address, int count = 1)
{
	Patch* p = new Patch(address, count);
	patch_map.insert({ address, p });

	memset(reinterpret_cast<void*>(address), 0x90, count);
}

// NOTE: The jump instruction is 5 bytes
inline void PatchJmp(uintptr_t address, void* jump_to, size_t length = 5)
{
	Patch* p = new Patch(address, length);
	patch_map.insert({ address, p });

	ptrdiff_t relative = reinterpret_cast<uintptr_t>(jump_to) - address - 5;

	// Write the jump instruction
	WriteMemory<uint8_t>(address, 0xE9);

	// Write the relative address to jump to
	WriteMemory<ptrdiff_t>(address + 1, relative);

	// Write nops until we've reached length
	memset(reinterpret_cast<void*>(address + 5), 0x90, length - 5);
}

inline void Unpatch(uintptr_t address, bool low_priority = false)
{
	if (patch_map.find(address) == patch_map.end())
	{
		if (!low_priority)
		{
			Error("Tried to Unpatch() non-existent patch %08X.", address);
			ASSERT(0);
		}

		// Low priority unpatching - ignore non-existent patch error
		// Only use for dynamic patches such as No Busted
		// For static patches, it's better not to ignore these errors
		return;
	}

	Patch* p = patch_map.at(address);
	patch_map.erase(address);

	memcpy(reinterpret_cast<void*>(address), p->bytes, p->len);
	delete p;
}

inline uintptr_t VirtualFunction(uintptr_t thisptr, ptrdiff_t index)
{
	return ReadMemory<uintptr_t>(ReadMemory<uintptr_t>(thisptr) + index * 4);
}

inline void PurecallHandler()
{
	Error("Pure virtual function call.");
	ASSERT(0);
}

inline void SaveStruct(const char* filename, IValidatable& object, size_t size)
{
	if (!object.Validate())
	{
		char msg[512];
		sprintf_s(msg, 512, "Error saving file %s.\n\n"
			"The object you are trying to save has failed to validate, indicating it contains invalid (corrupt or otherwise unsafe) values.\n\nProceed?", filename);

		if (MessageBoxA(NULL, msg, PROJECT_NAME, MB_ICONWARNING | MB_YESNO) != IDYES)
		{
			return;
		}
	}

	// Avoid saving the vtable pointer
	size -= 4;

	FILE* file = nullptr;
	fopen_s(&file, filename, "wb");
	if (!file)
	{
		char error[64];
		strerror_s(error, errno);
		Error("Error saving file %s.\n\nError code %d: %s", filename, errno, error);
	}
	else
	{
		// Avoid saving the vtable pointer
		fwrite((char*)&object + 4, 1, size, file);
		fclose(file);
	}
}

inline bool LoadStruct(const char* filename, IValidatable& object, size_t size, bool allow_undersize = false)
{
	// Avoid saving the vtable pointer
	size -= 4;

	FILE* file = nullptr;
	fopen_s(&file, filename, "rb");
	if (!file)
	{
		char error[64];
		strerror_s(error, errno);
		Error("Error opening file %s.\n\nError code %d: %s", filename, errno, error);
		return false;
	}
	else
	{
		char* buffer = new char[size];

		fseek(file, 0, SEEK_END);
		int len = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (len > size || (!allow_undersize && len < size))
		{
			fclose(file);
			Error("Error opening file %s.\n\nInvalid file length - expected %d, got %d.", filename, size, len);
			return false;
		}

		fread_s(buffer, len, 1, len, file);
		fclose(file);

		// Avoid saving the vtable pointer
		memcpy((char*)&object + 4, buffer, len);

		if (!object.Validate())
		{
			char msg[512];
			sprintf_s(msg, 512, "Error opening file %s.\n\n"
				"The object you are trying to load has failed to validate, indicating it contains invalid (corrupt or otherwise unsafe) values.\n\nProceed?", filename);

			return MessageBoxA(NULL, msg, PROJECT_NAME, MB_ICONWARNING | MB_YESNO) == IDYES;
		}
	}

	return true;
}

// !!! Length MUST match for both strings !!!
inline void ScuffedStringToWide(const char* string, wchar_t* wide, size_t len)
{
	char* wide_multi = reinterpret_cast<char*>(wide);

	for (int i = 0; i < len; i++)
	{
		wide_multi[i * 2] = string[i];
		wide_multi[i * 2 + 1] = '\0';

		// Null character - assume we're done
		if (string[i] == '\0')
		{
			return;
		}
	}
}

/* === Shared hook/gui data (Globals) === */

namespace g
{
	// See patches::AIPlayer() for more info about these vtables
	namespace ai_player
	{
		inline VTable<3>* iserviceable_vtbl;
		inline VTable<10>* ientity_vtbl;
		inline VTable<7>* iattachable_vtbl;
		inline VTable<27>* iplayer_vtbl;
	}

	// GetAvailablePresetVehicle hook
	namespace encounter
	{
		inline char vehicle[32] = {0};
		inline bool overridden = false;
	}

	// GPS only
	namespace world_map
	{
		constexpr uint32_t gps_color = 0xFFFFFFFF;

		inline bool gps_only = false;
		inline bool shift_held = false;

		inline nfsc::Vector3 location = { 0, 0, 0 };
	}

	// Health icon render
	namespace health_icon
	{
		inline bool show = false;
	}

	// MoveVinyl hook
	namespace move_vinyl
	{
		inline int step_size = 1;
	}

	// NeedsEncounter hook
	namespace needs_encounter
	{
		inline bool value = false;
		inline bool overridden = false;
	}

	// NeedsTraffic hook
	namespace needs_traffic
	{
		inline bool value = false;
		inline bool overridden = false;
	}

	// NeedsTraffic hook
	namespace pursue_racers
	{
		inline bool value = false;
		inline bool overridden = false;
	}

	// Roadblock setups
	namespace roadblock_setups
	{
		inline nfsc::RoadblockSetup normal[16];
		inline nfsc::RoadblockSetup spiked[10];

		constexpr size_t size = 100;
		inline nfsc::RoadblockSetup* mine;

		//inline nfsc::RoadblockSetup* force = nullptr;
	}

	// Smart AI hook(s)
	namespace smart_ai
	{
		inline nfsc::Vector3 target = { 0, 0, 0 };
	}

	namespace custom_sms
	{
		inline constexpr uint8_t handle = 149;

		inline wchar_t subject[64] = {0};
		inline wchar_t from[64] = {0};
		inline wchar_t message[1024] = {0};

		inline bool exists = false;
		inline bool read = false;
	}

	// FOV overrides
	namespace fov
	{
		constexpr uintptr_t player = 0xB1D520;
		inline int player_fov = 0;
		inline bool player_override = false;

		constexpr uintptr_t rvm = 0xB1EE00;
		inline int rvm_fov = 0;
		inline bool rvm_override = false;

		constexpr uintptr_t pip = 0xAB0C90;
		inline int pip_fov = 0;
		inline bool pip_override = false;
	}
	
	// Wrong warp fix
	namespace wrong_warp_fix
	{
		inline bool enabled = false;
	}
}