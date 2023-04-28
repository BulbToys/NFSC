#pragma once

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
	inline void* (__thiscall* PVehicle_GetSimable)(void* pvehicle) = reinterpret_cast<void* (__thiscall*)(void*)>(0x6D7EC0);

	inline void* (__thiscall* PhysicsObject_GetRigidBody)(void* physics_object) = reinterpret_cast<void* (__thiscall*)(void*)>(0x6D6CD0);

	inline void (__thiscall* RigidBody_SetPosition)(void* rigid_body, vector3* position) = reinterpret_cast<void(__thiscall*)(void*, vector3*)>(0x6E8210);

	inline bool (__thiscall* WRoadNav_FindPath)(void* roadnav, vector3* vec3_goal_position, vector3* vec3_goal_direction, bool shortcuts_allowed) =
		reinterpret_cast<bool(__thiscall*)(void*, vector3*, vector3*, bool)>(0x7FB090);

	inline void (*FE_Object_GetCenter)(void* object, float* x, float* y) = reinterpret_cast<void(*)(void*, float*, float*)>(0x597900);

	inline bool (__thiscall* WCollisionMgr_GetWorldHeightAtPointRigorous)(WCollisionMgr* mgr, vector3* point, float* height, vector3* normal) =
		reinterpret_cast<bool(__thiscall*)(WCollisionMgr*, vector3*, float*, vector3*)>(0x816DF0);

	inline void (__thiscall* WorldMap_GetPanFromMapCoordLocation)(void* world_map, vector2* output, vector2* input) =
		reinterpret_cast<void(__thiscall*)(void*, vector2*, vector2*)>(0x5ACA90);
	inline void (*WorldMap_SetGPSIng)(void* icon) = reinterpret_cast<void(*)(void*)>(0x582C30);

	inline bool (*GPS_Engage)(vector3* target, float max_deviation, bool always_re_establish) = reinterpret_cast<bool(*)(vector3*, float, bool)>(0x433AB0);

	inline void* (__thiscall* GManager_AllocIcon)(void* g_manager, char type, vector3* position, float rotation, bool is_disposable) =
		reinterpret_cast<void*(__thiscall*)(void*, char, vector3*, float, bool)>(0x626F90);

	inline unsigned int (*bStringHash)(const char* string) = reinterpret_cast<unsigned int(*)(const char*)>(0x471050);

	inline void (__thiscall* GIcon_Spawn)(void* icon) = reinterpret_cast<void(__thiscall*)(void*)>(0x627840);
}