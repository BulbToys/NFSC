#pragma once

#define FUNC(address, return_t, callconv, name, ...) inline return_t (callconv* name)(__VA_ARGS__) = reinterpret_cast<decltype(name)>(address)

namespace nfsc
{
	/* ===== GAME ENUMS ===== */

	inline const char* goals[] = { "AIGoalEncounterPursuit", "AIGoalNone", "AIGoalRacer", "AIGoalTraffic", "AIGoalPatrol"};
	enum class ai_goal : int
	{
		encounter_pursuit = 0,
		none              = 1,
		racer             = 2,
		traffic           = 3,
		patrol            = 4
	};

	inline const char* driver_classes[] = { "traffic", "cop", "racer", "none", "nis", "remote", "remote_racer", "ghost", "hub" };
	enum driver_class
	{
		human        = 0x0,
		traffic      = 0x1,
		cop          = 0x2,
		racer        = 0x3,
		none         = 0x4,
		nis          = 0x5,
		remote       = 0x6,
		remote_racer = 0x7,
		ghost        = 0x8,
		hub          = 0x9,
		max          = 0xA
	};

	enum class gameflow_state : int
	{
		in_frontend = 3,
		racing      = 6
	};

	enum class rbelem_t : int
	{
		none       = 0,
		car        = 1,
		barrier    = 2,
		spikestrip = 3
	};

	enum vehicle_param_flags : uint32_t
	{
		spool_resources     = 0x1,
		snap_to_ground      = 0x2,
		remove_nos          = 0x4,
		compute_performance = 0x8,
		force_nos           = 0x10,
		low_rez             = 0x20,
		critical            = 0x40,
		physics_only        = 0x80
	};

	/* ===== GAME OBJECTS ===== */

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

	struct Vector2
	{
		float x = 0;
		float y = 0;
	};

	struct Vector3
	{
		float x = 0;
		float y = 0;
		float z = 0;
	};

	struct WCollisionMgr
	{
		unsigned int fSurfaceExclusionMask = 0;
		unsigned int fPrimitiveMask = 0;
	};

	/* ===== GAME CONSTANTS ===== */

	constexpr uintptr_t IVehicle = 0x403D30;

	constexpr uintptr_t IVehicleList_begin = 0xA9F158 + 0x04;

	constexpr uintptr_t ThePursuitSimables = 0xA98140;

	/* ===== GAME FUNCTIONS ===== */

	FUNC(0x4639D0, uint32_t, , Attrib_StringToKey, const char* string);

	FUNC(0x471050, unsigned int, , bStringHash, const char* string);

	FUNC(0x48D620, void, , CameraAI_SetAction, int eview, const char* name);

	FUNC(0x7D4E40, bool, __thiscall, DebugVehicleSelection_SwitchPlayerVehicle, void* debug_veh_sel, char* name);

	FUNC(0x5227F0, void, __thiscall, EAXSound_StartNewGamePlay, void* eax_sound);

	FUNC(0x597900, void, , FE_Object_GetCenter, void* object, float* x, float* y);

	FUNC(0x65C330, void, , Game_ClearAIControl, int unk);
	FUNC(0x65C2C0, void, , Game_ForceAIControl, int unk);
	FUNC(0x651750, void, , Game_SetAIGoal, void* simable, const char* goal);
	FUNC(0x6513E0, void, , Game_SetCopsEnabled, bool enable);
	FUNC(0x667FF0, void, , Game_UnlockNikki);

	FUNC(0x627840, void, __thiscall, GIcon_Spawn, void* icon);

	FUNC(0x616FE0, uint32_t, , GKnockoutRacer_GetPursuitVehicleKey, bool is_player);

	FUNC(0x626F90, void*, __thiscall, GManager_AllocIcon, void* g_manager, char type, Vector3* position, float rotation, bool is_disposable);

	FUNC(0x433AB0, bool, , GPS_Engage, Vector3* target, float max_deviation, bool always_re_establish);

	FUNC(0x7BF9B0, void, , KillSkidsOnRaceRestart);

	FUNC(0x75DA60, void, __thiscall, LocalPlayer_ResetHUDType, void* local_player, int hud_type);

	FUNC(0x6D6CD0, void*, __thiscall, PhysicsObject_GetRigidBody, void* physics_object);

	FUNC(0x6D8110, void*, __thiscall, PVehicle_GetAIVehiclePtr, void* pvehicle);
	FUNC(0x6D7EC0, void*, __thiscall, PVehicle_GetSimable, void* pvehicle);
	FUNC(0x6D8070, float, __thiscall, PVehicle_GetSpeed, void* pvehicle);
	
	FUNC(0x6C6FF0, Vector3*, __thiscall, RigidBody_GetPosition, void* rigid_body);
	FUNC(0x6E8210, void, __thiscall, RigidBody_SetPosition, void* rigid_body, Vector3* position);

	FUNC(0x816DF0, bool, __thiscall, WCollisionMgr_GetWorldHeightAtPointRigorous, WCollisionMgr* mgr, Vector3* point, float* height, Vector3* normal);

	FUNC(0x7CA1A0, void, , World_RestoreProps);

	FUNC(0x5ACA90, void, __thiscall, WorldMap_GetPanFromMapCoordLocation, void* world_map, Vector2* output, Vector2* input);
	FUNC(0x582C30, void, , WorldMap_SetGPSIng, void* icon);

	FUNC(0x7FB090, bool, __thiscall, WRoadNav_FindPath, void* roadnav, Vector3* goal_position, Vector3* goal_direction, bool shortcuts_allowed);

	/* ===== CUSTOM FUNCTIONS ===== */

	template <uintptr_t handle>
	inline void* BulbToys_FindInterface(uintptr_t ucom_object)
	{
		// IInterface* UTL::COM::Object::_IList::Find(UCOM::Object::_IList*, IInterface::IHandle);
		return reinterpret_cast<void*(__thiscall*)(void*, void*)>(0x60CB50)(reinterpret_cast<void*>(ucom_object + 4), reinterpret_cast<void*>(handle));
	}

	inline void* BulbToys_CreateSimable(void* vehicle_cache, driver_class dc, uint32_t key, Vector3* rotation, Vector3* position, uint32_t vpf,
		void* customization_record, void* performance_matching)
	{
		
		struct VehicleParams { uint8_t _[0x3C]; } params;

		/*
			void __thiscall VehicleParams::VehicleParams(
				VehicleParams *this,
				IVehicleCache *cache,
				DriverClass driver_class,
				unsigned int key,
				UMath::Vector3 *rotation,
				UMath::Vector3 *position,
				eVehicleParamFlags flags,
				FECustomizationRecord *record,
				Physics::Info::PerformanceMatching *matching);
		*/
		reinterpret_cast<void(__thiscall*)(VehicleParams&, void*, driver_class, uint32_t, Vector3*, Vector3*, uint32_t, void*, void*)>(0x412590)
			(params, vehicle_cache, dc, key, rotation, position, vpf, customization_record, performance_matching);

		// ISimable* UCOM::Factory<Sim::Param,ISimable,UCrc32>::CreateInstance(stringhash32("PVehicle"), params);
		void* simable = reinterpret_cast<void*(*)(uint32_t, VehicleParams)>(0x41F920)(0x1396EBE1, params);

		// Attrib::Instance::~Instance(&params.attributes);
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x469870)(reinterpret_cast<uintptr_t>(&params) + 0x14);

		return simable;
	}
}