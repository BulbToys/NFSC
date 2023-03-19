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

	inline void (*CameraAI_SetAction)(int, const char*) = reinterpret_cast<void(*)(int, const char*)>(0x48D620);
	inline void (*Game_ForceAIControl)(int) = reinterpret_cast<void(*)(int)>(0x65C2C0);
	inline void (*Game_ClearAIControl)(int) = reinterpret_cast<void(*)(int)>(0x65C330);
	inline void (*Game_SetCopsEnabled)(bool) = reinterpret_cast<void(*)(bool)>(0x6513E0);
}