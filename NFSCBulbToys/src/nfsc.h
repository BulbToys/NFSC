#pragma once

namespace nfsc
{
	enum class gameflow_state : int
	{
		in_frontend = 3,
		racing      = 6
	};
	inline gameflow_state state;

	inline const char* goals[] = { "AIGoalEncounterPursuit", "AIGoalNone", "AIGoalRacer", "AIGoalTraffic" };
	enum class ai_goal : int
	{
		encounter_pursuit = 0,
		none              = 1,
		racer             = 2,
		traffic           = 3
	};

	inline void** ivehicleList = reinterpret_cast<void**>(0xA9F168);

	inline void (*CameraAI_SetAction)(int eview, const char* name) = reinterpret_cast<void(*)(int, const char*)>(0x48D620);

	inline void (*Game_ForceAIControl)(int unk) = reinterpret_cast<void(*)(int)>(0x65C2C0);
	inline void (*Game_ClearAIControl)(int unk) = reinterpret_cast<void(*)(int)>(0x65C330);
	inline void (*Game_SetCopsEnabled)(bool enable) = reinterpret_cast<void(*)(bool)>(0x6513E0);
	inline void (*Game_UnlockNikki)(void) = reinterpret_cast<void(*)(void)>(0x667FF0);

	inline void* (__thiscall* PVehicle_GetAIVehiclePtr)(void* pvehicle) = reinterpret_cast<void* (__thiscall*)(void*)>(0x6D8110);
	inline float (__thiscall* PVehicle_GetSpeed)(void* pvehicle) = reinterpret_cast<float(__thiscall*)(void*)>(0x6D8070);

	inline bool (__thiscall* WRoadNav_FindPath)(void* roadnav, void* vec3_goal_position, void* vec3_goal_direction, bool shortcuts_allowed) =
		reinterpret_cast<bool(__thiscall*)(void*, void*, void*, bool)>(0x7FB090);
}