#pragma once

#define FUNC(address, return_t, callconv, name, ...) inline return_t (callconv* name)(__VA_ARGS__) = reinterpret_cast<return_t(callconv*)(__VA_ARGS__)>(address)

namespace nfsc
{
	struct vector3
	{
		float x = 0;
		float y = 0;
		float z = 0;
	};

	struct vector2
	{
		float x = 0;
		float y = 0;
	};

	struct WCollisionMgr
	{
		unsigned int fSurfaceExclusionMask = 0;
		unsigned int fPrimitiveMask = 0;
	};

	enum class gameflow_state : int
	{
		in_frontend = 3,
		racing      = 6
	};

	inline const char* goals[] = { "AIGoalEncounterPursuit", "AIGoalNone", "AIGoalRacer", "AIGoalTraffic" };
	enum class ai_goal : int
	{
		encounter_pursuit = 0,
		none              = 1,
		racer             = 2,
		traffic           = 3
	};

	enum class rbelem_t : int
	{
		none       = 0,
		car        = 1,
		barrier    = 2,
		spikestrip = 3
	};

	struct RoadblockElement
	{
		rbelem_t type = rbelem_t::none;
		float offset_x = 0;
		float offset_z = 0;
		float angle = 0;
	};

	struct RoadblockSetup
	{
		float minimum_width = 0;
		int required_vehicles = 0;
		RoadblockElement contents[6] = { {rbelem_t::none, 0, 0, 0} };
	};

	constexpr uintptr_t IVehicleList_begin = 0xA9F158 + 0x04;

	FUNC(0x471050, unsigned int, , bStringHash, const char* string);

	FUNC(0x48D620, void, , CameraAI_SetAction, int eview, const char* name);

	FUNC(0x7D4E40, bool, __thiscall, DebugVehicleSelection_SwitchPlayerVehicle, void* debug_veh_sel, char* name);

	FUNC(0x5227F0, void, __thiscall, EAXSound_StartNewGamePlay, void* eax_sound);

	FUNC(0x597900, void, , FE_Object_GetCenter, void* object, float* x, float* y);

	FUNC(0x65C330, void, , Game_ClearAIControl, int unk);
	FUNC(0x65C2C0, void, , Game_ForceAIControl, int unk);
	FUNC(0x6513E0, void, , Game_SetCopsEnabled, bool enable);
	FUNC(0x667FF0, void, , Game_UnlockNikki);

	FUNC(0x627840, void, __thiscall, GIcon_Spawn, void* icon);
	
	FUNC(0x626F90, void*, __thiscall, GManager_AllocIcon, void* g_manager, char type, vector3* position, float rotation, bool is_disposable);

	FUNC(0x433AB0, bool, , GPS_Engage, vector3* target, float max_deviation, bool always_re_establish);

	FUNC(0x7BF9B0, void, , KillSkidsOnRaceRestart);

	FUNC(0x75DA60, void, __thiscall, LocalPlayer_ResetHUDType, void* local_player, int hud_type);

	FUNC(0x6D6CD0, void*, __thiscall, PhysicsObject_GetRigidBody, void* physics_object);
	
	FUNC(0x6D8110, void*, __thiscall, PVehicle_GetAIVehiclePtr, void* pvehicle);
	FUNC(0x6D8070, float, __thiscall, PVehicle_GetSpeed, void* pvehicle);
	FUNC(0x6D7EC0, void*, __thiscall, PVehicle_GetSimable, void* pvehicle);
	
	FUNC(0x6E8210, void, __thiscall, RigidBody_SetPosition, void* rigid_body, vector3* position);

	FUNC(0x816DF0, bool, __thiscall, WCollisionMgr_GetWorldHeightAtPointRigorous, WCollisionMgr* mgr, vector3* point, float* height, vector3* normal);
	
	FUNC(0x7CA1A0, void, , World_RestoreProps);

	FUNC(0x5ACA90, void, __thiscall, WorldMap_GetPanFromMapCoordLocation, void* world_map, vector2* output, vector2* input);
	FUNC(0x582C30, void, , WorldMap_SetGPSIng, void* icon);

	FUNC(0x7FB090, bool, __thiscall, WRoadNav_FindPath, void* roadnav, vector3* goal_position, vector3* goal_direction, bool shortcuts_allowed);
}