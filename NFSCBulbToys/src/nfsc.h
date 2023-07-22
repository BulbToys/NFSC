#pragma once

#define FUNC(address, return_t, callconv, name, ...) inline return_t (callconv* name)(__VA_ARGS__) = reinterpret_cast<decltype(name)>(address)

#define LS(address, name, type) inline ListableSet<type>* name = reinterpret_cast<decltype(name)>(address)

struct ImDrawList;
struct ImVec4;

namespace nfsc
{
	/* ===== NAMESPACES ===== */

	namespace events
	{
		/*struct EventDef
		{
			uint32_t id;
			const char* name;
			uint32_t properties;
			EventSys::Event* (__cdecl* Construct)(EventSys::StaticData*, EventSys::DynamicData*);
			unsigned int StaticFieldsCount;
			EventSys::FieldDef* StaticFields;
			unsigned int StaticDataSize;
		};*/
	}

	/* ===== GAME ENUMS ===== */

	inline const char* player_cop_cars[] = { "player_cop", "player_cop_suv", "player_cop_gto" };

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
	enum class driver_class : int
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

	enum class race_type : int
	{
		none              = -1,
		sprint            = 0x0,
		circuit           = 0x1,
		drag              = 0x2,
		ko                = 0x3,
		tollbooth         = 0x4,
		speedtrap         = 0x5,
		checkpoint        = 0x6,
		cash_grab         = 0x7,
		challenge_series  = 0x8,
		jump_to_speedtrap = 0x9,
		jump_to_milestone = 0xA,
		drift             = 0xB,
		canyon_drift      = 0xC,
		canyon            = 0xD,
		pursuit_tag       = 0xE,
		pursuit_ko        = 0xF,
		encounter         = 0x10,
		max               = 0x11,
	};

	enum class rbelem_t : int
	{
		none       = 0,
		car        = 1,
		barrier    = 2,
		spikestrip = 3
	};

	inline const char* veh_lists[] = { "All", "Players", "AI", "AI Racers", "AI Cops", "AI Traffic", "Racers", "Remote", "Inactive", "Trailers", "Active Racers", "Ghosts" };
	enum vehicle_list : int
	{
		all          = 0x0,
		players      = 0x1,
		ai           = 0x2,
		airacers     = 0x3,
		aicops       = 0x4,
		aitraffic    = 0x5,
		racers       = 0x6,
		remote       = 0x7,
		inactive     = 0x8,
		trailers     = 0x9,
		activeracers = 0xA,
		ghost        = 0xB,
		max          = 0xC,
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

	struct FEColor
	{
		int b = 0, g = 0, r = 0, a = 0;
	};

	// fuck templates
	uint32_t ListableSet_GetGrowSizeVirtually(void* ls, uint32_t amount);

	template <typename T>
	struct ListableSet
	{
		uintptr_t vtbl;
		T* begin;
		size_t capacity;
		size_t size;

		/* ===== STRUCT END, HELPER FUNCTIONS BELOW ===== */

		//ListableSet(size_t capacity) : vtbl(0x9C3A98), begin(new T[capacity]), capacity(capacity), size(0) {}

		void Add(T elem, uintptr_t pfnPushback)
		{
			if (capacity >= size)
			{
				// Calling UTL::Vector<...>::GetGrowSize (ListableSet<T>::GetGrowSize) virtually
				uint32_t offset = nfsc::ListableSet_GetGrowSizeVirtually(this, size + 1);

				// Calls its respective vector's push_back function
				// Pretty sure there is no way to get this information from the listable itself, definitely not virtually
				reinterpret_cast<void(__thiscall*)(void*, uint32_t)>(pfnPushback)(this, offset);
			}

			T* end = begin + size;
			if (end)
			{
				*end = elem;
			}

			++size;
		}

		// NOTE: Leaks memory!
		bool Resize(size_t new_size)
		{
			// Allocate memory
			T* memory = new T[new_size];

			// Move elements
			for (int i = 0; i < size; i++)
			{
				memory[i] = begin[i];
			}

			// Don't think we're allowed to do this for our first resize
			//delete begin;

			// Assign new properties
			begin = memory;
			capacity = new_size;
		}
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

	struct Vector4
	{
		float x = 0;
		float y = 0;
		float z = 0;
		float w = 0;
	};

	struct Matrix4
	{
		Vector4 v0;
		Vector4 v1;
		Vector4 v2;
		Vector4 v3;
	};

	struct WCollisionMgr
	{
		unsigned int fSurfaceExclusionMask = 0;
		unsigned int fPrimitiveMask = 0;
	};

	/* ===== GAME CONSTANTS ===== */

	// Globals
	constexpr uintptr_t Direct3DDevice9 = 0xAB0ABC;
	inline gameflow_state* GameFlowManager_State = reinterpret_cast<gameflow_state*>(0xA99BBC);
	constexpr uintptr_t GRaceStatus = 0xA98284;
	constexpr uintptr_t ThePursuitSimables = 0xA98140;
	constexpr uintptr_t WorldMap = 0xA977F0;

	// FastMem is an OBJECT
	constexpr uintptr_t FastMem = 0xA99720;

	// Interfaces
	constexpr uintptr_t IVehicle = 0x403D30;
	constexpr uintptr_t IInput = 0x403AA0;

	// ListableSets
	LS(0xA83E90, AIPursuitList, uintptr_t);
	LS(0xA83AE8, AITargetsList, uintptr_t);
	LS(0xA9FE98, EntityList, uintptr_t);
	LS(0xA9FF28, IPlayerList, uintptr_t);

	inline ListableSet<uintptr_t>* VehicleList[vehicle_list::max] = {
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x0),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x1),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x2),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x3),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x4),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x5),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x6),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x7),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x8),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0x9),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0xA),
		reinterpret_cast<ListableSet<uintptr_t>*>(0xA9F158 + 0x88 * 0xB)
	};

	// Misc
	inline Vector3* ZeroV3 = reinterpret_cast<Vector3*>(0x9D780C);
	inline bool* SkipNIS = reinterpret_cast<bool*>(0xA9E64E);

	namespace spectate
	{
		inline bool* enabled = reinterpret_cast<bool*>(0xA888F1);
	}
	
	/* ===== GAME FUNCTIONS ===== */

	FUNC(0x436820, void, __thiscall, AIGoal_AddAction, uintptr_t ai_goal, char const* name);
	FUNC(0x42A2D0, void, __thiscall, AIGoal_ChooseAction, uintptr_t ai_goal, float dt);
	FUNC(0x42A240, void, __thiscall, AIGoal_ClearAllActions, uintptr_t ai_goal);

	FUNC(0x40EBC0, bool, __thiscall, AITarget_GetVehicleInterface, uintptr_t ai_target, uintptr_t* ivehicle);
	FUNC(0x429C80, void, __thiscall, AITarget_Acquire, uintptr_t ai_target, uintptr_t simable);

	FUNC(0x4639D0, uint32_t, , Attrib_StringToKey, const char* string);

	FUNC(0x471050, uint32_t, , bStringHash, const char* string);
	FUNC(0x4710B0, uint32_t, , bStringHashUpper, const char* string);

	FUNC(0x48D620, void, , CameraAI_SetAction, int eview, const char* name);

	FUNC(0x7D4E40, bool, __thiscall, DebugVehicleSelection_SwitchPlayerVehicle, uintptr_t debug_veh_sel, char* name);

	FUNC(0x5227F0, void, __thiscall, EAXSound_StartNewGamePlay, uintptr_t eax_sound);

	FUNC(0x597900, void, , FE_Object_GetCenter, uintptr_t object, float* x, float* y);
	FUNC(0x570CC0, void, , FE_Object_SetColor, uintptr_t object, FEColor* color);

	FUNC(0x5CDEA0, void, , FEDialogScreen_ShowDialog, const char* message, const char* button1, const char* button2, const char* button3);

	FUNC(0x579200, void, __thiscall, FEStateManager_ChangeState, uintptr_t state_manager, int current_state);
	FUNC(0x5A53A0, void, __thiscall, FEStateManager_PopBack, uintptr_t state_manager, int next_state);
	FUNC(0x579C10, void, __thiscall, FEStateManager_ShowDialog, uintptr_t state_manager, int next_state);

	FUNC(0x65C330, void, , Game_ClearAIControl, int unk);
	FUNC(0x65C2C0, void, , Game_ForceAIControl, int unk);
	FUNC(0x65D620, uintptr_t, , Game_PursuitSwitch, int racer_index, bool is_busted, int* result);
	FUNC(0x651750, void, , Game_SetAIGoal, uintptr_t simable, const char* goal);
	FUNC(0x6513E0, void, , Game_SetCopsEnabled, bool enable);
	FUNC(0x6517A0, void, , Game_SetPursuitTarget, uintptr_t chaser_simable, uintptr_t target_simable);
	FUNC(0x65DD60, void, , Game_TagPursuit, int index1, int index2, bool busted);
	FUNC(0x667FF0, void, , Game_UnlockNikki);

	FUNC(0x578830, const char*, , GetLocalizedString, uint32_t key);

	FUNC(0x627840, void, __thiscall, GIcon_Spawn, uintptr_t icon);

	FUNC(0x616FE0, uint32_t, , GKnockoutRacer_GetPursuitVehicleKey, bool is_player);

	FUNC(0x626F90, uintptr_t, __thiscall, GManager_AllocIcon, uintptr_t g_manager, char type, Vector3* position, float rotation, bool is_disposable);

	FUNC(0x433AB0, bool, , GPS_Engage, Vector3* target, float max_deviation, bool always_re_establish);

	FUNC(0x422730, uintptr_t, __thiscall, GRacerInfo_GetSimable, uintptr_t g_racer_info);
	FUNC(0x61B8F0, void, __thiscall, GRacerInfo_SetSimable, uintptr_t g_racer_info, uintptr_t simable);

	FUNC(0x624E80, uintptr_t, __thiscall, GRaceStatus_GetRacePursuitTarget, uintptr_t g_race_status, int* index);
	FUNC(0x612230, int, __thiscall, GRaceStatus_GetRacerCount, uintptr_t g_race_status);
	FUNC(0x6121E0, uintptr_t, __thiscall, GRaceStatus_GetRacerInfo, uintptr_t g_race_status, int index);
	FUNC(0x629670, uintptr_t, __thiscall, GRaceStatus_GetRacerInfo2, uintptr_t g_race_status, uintptr_t simable);

	FUNC(0x7BF9B0, void, , KillSkidsOnRaceRestart);

	FUNC(0x75DA60, void, __thiscall, LocalPlayer_ResetHUDType, uintptr_t local_player, int hud_type);

	FUNC(0x6C6740, bool, __thiscall, PhysicsObject_Attach, uintptr_t physics_object, uintptr_t player);
	FUNC(0x6D6C40, uintptr_t, __thiscall, PhysicsObject_GetPlayer, uintptr_t physics_object);
	FUNC(0x6D6CD0, uintptr_t, __thiscall, PhysicsObject_GetRigidBody, uintptr_t physics_object);
	FUNC(0x6D19A0, void, __thiscall, PhysicsObject_Kill, uintptr_t physics_object);

	FUNC(0x803B40, bool, , Props_CreateInstance, uintptr_t& placeable_scenery, const char* name, uint32_t attributes);

	FUNC(0x6C0BE0, void, __thiscall, PVehicle_Activate, uintptr_t pvehicle);
	FUNC(0x6C0C00, void, __thiscall, PVehicle_Deactivate, uintptr_t pvehicle);
	FUNC(0x6C0C40, void, __thiscall, PVehicle_ForceStopOn, uintptr_t pvehicle, uint8_t force_stop);
	FUNC(0x6C0C70, void, __thiscall, PVehicle_ForceStopOff, uintptr_t pvehicle, uint8_t force_stop);
	FUNC(0x6D8110, uintptr_t, __thiscall, PVehicle_GetAIVehiclePtr, uintptr_t pvehicle);
	FUNC(0x6D7F60, nfsc::driver_class, __thiscall, PVehicle_GetDriverClass, uintptr_t pvehicle);
	FUNC(0x6D7F80, uint8_t, __thiscall, PVehicle_GetForceStop, uintptr_t pvehicle);
	FUNC(0x6D7EC0, uintptr_t, __thiscall, PVehicle_GetSimable, uintptr_t pvehicle);
	FUNC(0x6D8070, float, __thiscall, PVehicle_GetSpeed, uintptr_t pvehicle);
	FUNC(0x6D7F20, char*, __thiscall, PVehicle_GetVehicleName, uintptr_t pvehicle);
	FUNC(0x6C0BA0, void, __thiscall, PVehicle_GlareOn, uintptr_t pvehicle, uint32_t fx_id);
	FUNC(0x6D80C0, bool, __thiscall, PVehicle_IsActive, uintptr_t pvehicle);
	FUNC(0x6D43A0, void, __thiscall, PVehicle_Kill, uintptr_t pvehicle);
	FUNC(0x6D4410, void, __thiscall, PVehicle_ReleaseBehaviorAudio, uintptr_t pvehicle);
	FUNC(0x6DA500, void, __thiscall, PVehicle_SetDriverClass, uintptr_t pvehicle, driver_class dc);
	FUNC(0x6C61E0, void, __thiscall, PVehicle_SetSpeed, uintptr_t pvehicle, float speed);
	FUNC(0x6D1100, bool, __thiscall, PVehicle_SetVehicleOnGround, uintptr_t pvehicle, Vector3* position, Vector3* forward_vector);
	
	FUNC(0x6C6CD0, void, __thiscall, RigidBody_GetDimension, uintptr_t rigid_body, Vector3* dimension);
	FUNC(0x6C70E0, void, __thiscall, RigidBody_GetForwardVector, uintptr_t rigid_body, Vector3* forward_vector);
	FUNC(0x6C6FF0, Vector3*, __thiscall, RigidBody_GetPosition, uintptr_t rigid_body);
	FUNC(0x6E8210, void, __thiscall, RigidBody_SetPosition, uintptr_t rigid_body, Vector3* position);

	FUNC(0x761550, float, , Sim_DistanceToCamera, Vector3* target);

	FUNC(0x411FD0, float, , UMath_Distance, Vector3* vec1, Vector3* vec2);
	FUNC(0x401DD0, float, , UMath_DistanceNoSqrt, Vector3* vec1, Vector3* vec2);
	FUNC(0x412190, void, , UMath_Normalize, Vector3* vec);
	FUNC(0x401B50, void, , UMath_Rotate, Vector3* vec, Matrix4* rot, Vector3* result);

	FUNC(0x764E00, void, , Util_GenerateMatrix, Matrix4* result, Vector3* fwd_vec, Vector3* in_up);

	FUNC(0x406340, void, , VU0_v3add, float a1, Vector3* a2, Vector3* a3);

	FUNC(0x816DF0, bool, __thiscall, WCollisionMgr_GetWorldHeightAtPointRigorous, WCollisionMgr& mgr, Vector3* point, float* height, Vector3* normal);

	FUNC(0x7CA1A0, void, , World_RestoreProps);

	FUNC(0x5ACA90, void, __thiscall, WorldMap_GetPanFromMapCoordLocation, uintptr_t world_map, Vector2* output, Vector2* input);
	FUNC(0x582C30, void, , WorldMap_SetGPSIng, uintptr_t icon);

	FUNC(0x7FB090, bool, __thiscall, WRoadNav_FindPath, uintptr_t roadnav, Vector3* goal_position, Vector3* goal_direction, bool shortcuts_allowed);

	/* ===== CUSTOM FUNCTIONS ===== */

	inline uintptr_t BulbToys_CreateSimable(uintptr_t vehicle_cache, driver_class dc, uint32_t key, Vector3* rotation, Vector3* position, uint32_t vpf,
		uintptr_t customization_record, uintptr_t performance_matching)
	{
		struct VehicleParams { uint8_t _[0x3C]; } params;

		// void __thiscall VehicleParams::VehicleParams(...);
		reinterpret_cast<void(__thiscall*)(VehicleParams&, uintptr_t, driver_class, uint32_t, Vector3*, Vector3*, uint32_t, uintptr_t, uintptr_t)>(0x412590)
			(params, vehicle_cache, dc, key, rotation, position, vpf, customization_record, performance_matching);

		// ISimable* UCOM::Factory<Sim::Param,ISimable,UCrc32>::CreateInstance(stringhash32("PVehicle"), params);
		uintptr_t simable = reinterpret_cast<uintptr_t(*)(uint32_t, VehicleParams)>(0x41F920)(0x1396EBE1, params);

		// Attrib::Instance::~Instance(&params.attributes);
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x469870)(reinterpret_cast<uintptr_t>(&params) + 0x14);

		return simable;
	}

	uintptr_t BulbToys_CreatePursuitSimable(nfsc::driver_class dc);

	void BulbToys_DrawObject(ImDrawList* draw_list, Vector3& position, Vector3& dimension, Vector3& rotation, ImVec4& color, float thickness);

	void BulbToys_DrawVehicleInfo(ImDrawList* draw_list, uintptr_t vehicle, vehicle_list type, ImVec4& color);

	template <uintptr_t handle>
	inline uintptr_t BulbToys_FindInterface(uintptr_t iface)
	{
		uintptr_t ucom_object = ReadMemory<uintptr_t>(iface + 4);

		// IInterface* UTL::COM::Object::_IList::Find(UCOM::Object::_IList*, IInterface::IHandle);
		return reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uintptr_t)>(0x60CB50)(ucom_object, handle);
	}

	uintptr_t BulbToys_GetAIVehicleGoal(uintptr_t ai_vehicle_ivehicleai);

	const char* BulbToys_GetCameraName();

	bool BulbToys_GetDebugCamCoords(Vector3* position, Vector3* fwd_vec);

	inline float BulbToys_GetDistanceBetween(uintptr_t simable1, uintptr_t simable2)
	{
		uintptr_t rb1 = nfsc::PhysicsObject_GetRigidBody(simable1);
		uintptr_t rb2 = nfsc::PhysicsObject_GetRigidBody(simable2);

		return nfsc::UMath_Distance(nfsc::RigidBody_GetPosition(rb1), nfsc::RigidBody_GetPosition(rb2));
	}

	inline float BulbToys_GetDistanceBetween(uintptr_t simable, Vector3* pos)
	{
		uintptr_t rb = nfsc::PhysicsObject_GetRigidBody(simable);

		// UMath::Distance
		return reinterpret_cast<float(*)(Vector3*, Vector3*)>(0x411FD0)(nfsc::RigidBody_GetPosition(rb), pos);
	}

	bool BulbToys_GetMyVehicle(uintptr_t* my_vehicle, uintptr_t* my_simable);

	int BulbToys_GetPVehicleTier(uintptr_t pvehicle);

	void BulbToys_GetScreenPosition(Vector3& world_position, Vector3& screen_position);

	float BulbToys_GetStreetWidth(Vector3* position, Vector3* direction, float distance, Vector3* left_pos, Vector3* right_pos, Vector3* fwd_vec);

	race_type BulbToys_GetRaceType();

	bool BulbToys_IsGPSDown();

	void BulbToys_PathToTarget(uintptr_t ai_vehicle, Vector3* target);

	void* BulbToys_RoadblockCalculations(nfsc::RoadblockSetup* setup, uintptr_t rigid_body);

	enum class sv_mode
	{
		// One-way switch. Use only when creating your own simable, not recommended otherwise, as it kills the old vehicle (TODO: test if this matters)
		one_way,

		// TODO: Two-way switch, with teleportation
		two_way,

		// TODO: Two-way switch, without teleportation
		two_way_no_tp
	};

	bool BulbToys_SwitchVehicle(uintptr_t simable, uintptr_t simable2, sv_mode mode);

	/*
	inline void BulbToys_SetCopActions(uintptr_t ai_goal)
	{
		nfsc::AIGoal_ClearAllActions(ai_goal);

		nfsc::AIGoal_AddAction(ai_goal, "AIActionPursuitOffRoad");
		nfsc::AIGoal_AddAction(ai_goal, "AIActionStunned");
		nfsc::AIGoal_AddAction(ai_goal, "AIActionGetUnstuck");

		nfsc::AIGoal_ChooseAction(ai_goal, 0.0);
	}

	inline void BulbToys_SetRacerActions(uintptr_t ai_goal)
	{
		nfsc::AIGoal_ClearAllActions(ai_goal);

		nfsc::AIGoal_AddAction(ai_goal, "AIActionRace");
		nfsc::AIGoal_AddAction(ai_goal, "AIActionStunned");
		nfsc::AIGoal_AddAction(ai_goal, "AIActionGetUnstuck");

		nfsc::AIGoal_ChooseAction(ai_goal, 0.0);
	}
	*/

	void __fastcall BulbToys_SwitchPTagTarget(uintptr_t race_status, bool busted);

	void BulbToys_UpdateWorldMapCursor();

	/* ===== AI PLAYER ===== */
	struct AIPlayer
	{
		/* ===== DATA ===== */
		struct
		{
			struct
			{
				struct
				{
					uintptr_t vtbl;
				}
				Sim_IServiceable;

				struct
				{
					uintptr_t vtbl;
				}
				Sim_ITaskable;

				struct
				{
					struct
					{
						uintptr_t mpBegin;
						uintptr_t mpEnd;
						uintptr_t mpCapacity;
						char allocator[4];
					}
					IList;
				}
				UCOM_Object;

				uint32_t count;
				uint32_t task_count;
				uint32_t service_count;
			}
			Sim_Object;

			struct
			{
				struct
				{
					uintptr_t vtbl;
					uintptr_t com_object;
				}
				UCOM_IUnknown;

				char _[4];
			}
			Sim_IEntity;

			struct
			{
				uintptr_t vtbl;
				uintptr_t com_object;
			}
			IAttachable;

			struct
			{
				bool dirty;
			}
			UTL_GarbageNode;

			uintptr_t simable;
			uintptr_t attachments;
		}
		Sim_Entity;

		struct
		{
			uintptr_t vtbl;
			uintptr_t com_object;
		}
		IPlayer;

		int setting_index;

		/* ===== FUNCTIONS ===== */
		// We're using __fastcalls because they are the most compatible with __thiscalls and we need to get their function pointers later
		// It is not possible to normally get a function pointer of a __thiscall function

		static AIPlayer* New();

		static void __fastcall Destructor(AIPlayer* ai_player)
		{
			// IPlayer::~IPlayer(&this->IPlayer)
			reinterpret_cast<void(__thiscall*)(void*)>(0x76BCD0)(&ai_player->IPlayer);

			// Sim::Entity::~Entity(this)
			reinterpret_cast<void(__thiscall*)(void*)>(0x76C790)(ai_player);
		}

		static AIPlayer* __fastcall VecDelDtor(AIPlayer* ai_player, int edx, uint8_t flags);

		template <ptrdiff_t offset>
		static AIPlayer* __fastcall VecDelDtorAdj(AIPlayer* ai_player, int edx, uint8_t flags)
		{
			return VecDelDtor(reinterpret_cast<AIPlayer*>(reinterpret_cast<uintptr_t>(ai_player) - offset), edx, flags);
		}

		static uintptr_t __fastcall GetSimable_IPlayer(AIPlayer* ai_player);

		static Vector3* __fastcall GetPosition_IPlayer(AIPlayer* ai_player)
		{
			uintptr_t simable = GetSimable_IPlayer(ai_player);
			if (!simable)
			{
				return nfsc::ZeroV3;
			}

			uintptr_t rigid_body = nfsc::PhysicsObject_GetRigidBody(simable);
			if (!rigid_body)
			{
				return nfsc::ZeroV3;
			}

			return RigidBody_GetPosition(rigid_body);
		}
		static bool __fastcall SetPosition_IPlayer(AIPlayer* ai_player, int edx, Vector3* pos)
		{
			uintptr_t simable = GetSimable_IPlayer(ai_player);
			if (!simable)
			{
				return false;
			}

			uintptr_t rigid_body = nfsc::PhysicsObject_GetRigidBody(simable);
			if (!rigid_body)
			{
				return false;
			}

			nfsc::RigidBody_SetPosition(rigid_body, pos);
			return true;
		}
	};
}