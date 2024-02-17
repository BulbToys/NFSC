#pragma once
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <d3d9.h>

#include "utils.h"

#include "nfsc.h"

#include "gui.h"
#include "hooks.h"
#include "patches.h"

#include "rev_count.h"

/*
struct Scenery
{
	static int count;
	std::vector<char*> props;

	void Add(char* name)
	{
		auto iter = props.begin();
		while (iter != props.end())
		{
			char* prop = *iter;

			if (!strncmp(name, prop, 24))
			{
				return;
			}

			iter++;
		}

		char* prop = new char[24];
		memcpy(prop, name, 24);
		props.push_back(prop);
		count++;
	}

	void Save()
	{
		static FILE* file = nullptr;
		fopen_s(&file, "props.txt", "a");
		if (file)
		{
			auto iter = props.begin();
			while (iter != props.end())
			{
				char* prop = *iter;

				size_t len = strlen(prop);


				fwrite(prop, 1, len, file);
				fwrite("\n", 1, 1, file);

				props.erase(iter);
				delete prop;

				count--;
			}

			fwrite("========================\n\n", 1, 26, file);

			fclose(file);
		}
	}
};
inline Scenery scenery;
inline int Scenery::count = 0;
*/

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


/* === Shared hook/gui data (Globals) === */

namespace g
{
	// See Patches::AIPlayer() for more info about these vtables
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

	/*
	// World Map Fat Line Mesh
	namespace flm
	{
		inline bool custom = false;
		inline float x = -9.f;
		inline float y = 451.f;
		inline float scale = 1.27f;

		inline bool processing = false;
	}
	*/
	
	namespace world_map
	{
		// GPS only
		constexpr uint32_t gps_color = 0xFFFFFFFF;

		inline bool gps_only = false;
		inline bool shift_held = false;

		inline NFSC::Vector3 location = { 0, 0, 0 };

		// FLM
		inline uintptr_t flm = 0;

		inline std::vector<NFSC::Vector2> test;
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
		inline NFSC::RoadblockSetup normal[16];
		inline NFSC::RoadblockSetup spiked[10];

		constexpr size_t size = 100;
		inline NFSC::RoadblockSetup* mine;

		//inline NFSC::RoadblockSetup* force = nullptr;
	}

	// Smart AI hook(s)
	namespace smart_ai
	{
		inline NFSC::Vector3 target = { 0, 0, 0 };
	}

	namespace custom_sms
	{
		// 26-29 and 63-149 inclusive are all unused by the game
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