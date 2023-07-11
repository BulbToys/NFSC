#pragma once

#define FUNC(address, return_t, callconv, name, ...) inline return_t (callconv* name)(__VA_ARGS__) = reinterpret_cast<decltype(name)>(address)

#define LS(address, name, type) inline ListableSet<type>* name = reinterpret_cast<decltype(name)>(address)

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
				memory[i] = *(begin + i);
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

	struct WCollisionMgr
	{
		unsigned int fSurfaceExclusionMask = 0;
		unsigned int fPrimitiveMask = 0;
	};

	/* ===== GAME CONSTANTS ===== */

	// Globals
	constexpr uintptr_t GRaceStatus = 0xA98284;
	constexpr uintptr_t ThePursuitSimables = 0xA98140;
	inline gameflow_state* GameFlowManager_State = reinterpret_cast<gameflow_state*>(0xA99BBC);

	// FastMem is an OBJECT
	constexpr uintptr_t FastMem = 0xA99720;

	// Interfaces
	constexpr uintptr_t IVehicle = 0x403D30;

	// ListableSets
	LS(0xA83E90, AIPursuitList, void*);
	LS(0xA83AE8, AITargetsList, void*);
	LS(0xA9FE98, EntityList, void*);
	LS(0xA9FF28, IPlayerList, void*);
	LS(0xA9F158, IVehicleList, void*);

	// Misc
	inline Vector3* ZeroV3 = reinterpret_cast<Vector3*>(0x9D780C);
	inline bool* SkipNIS = reinterpret_cast<bool*>(0xA9E64E);

	/* ===== GAME FUNCTIONS ===== */

	FUNC(0x436820, void, __thiscall, AIGoal_AddAction, void* ai_goal, char const* name);
	FUNC(0x42A2D0, void, __thiscall, AIGoal_ChooseAction, void* ai_goal, float dt);
	FUNC(0x42A240, void, __thiscall, AIGoal_ClearAllActions, void* ai_goal);

	FUNC(0x40EBC0, bool, __thiscall, AITarget_GetVehicleInterface, void* ai_target, void** ivehicle);
	FUNC(0x429C80, void, __thiscall, AITarget_Acquire, void* ai_target, void* simable);

	FUNC(0x4639D0, uint32_t, , Attrib_StringToKey, const char* string);

	FUNC(0x471050, unsigned int, , bStringHash, const char* string);

	FUNC(0x48D620, void, , CameraAI_SetAction, int eview, const char* name);

	FUNC(0x7D4E40, bool, __thiscall, DebugVehicleSelection_SwitchPlayerVehicle, void* debug_veh_sel, char* name);

	FUNC(0x5227F0, void, __thiscall, EAXSound_StartNewGamePlay, void* eax_sound);

	FUNC(0x597900, void, , FE_Object_GetCenter, void* object, float* x, float* y);

	FUNC(0x5CF460, void, , FEDialogScreen_ShowOKCancel, const char* message);

	FUNC(0x65C330, void, , Game_ClearAIControl, int unk);
	FUNC(0x65C2C0, void, , Game_ForceAIControl, int unk);
	FUNC(0x65D620, void*, , Game_PursuitSwitch, int racer_index, bool is_busted, int* result);
	FUNC(0x651750, void, , Game_SetAIGoal, void* simable, const char* goal);
	FUNC(0x6513E0, void, , Game_SetCopsEnabled, bool enable);
	FUNC(0x6517A0, void, , Game_SetPursuitTarget, void* chaser_simable, void* target_simable);
	FUNC(0x65DD60, void, , Game_TagPursuit, int index1, int index2, bool busted);
	FUNC(0x667FF0, void, , Game_UnlockNikki);

	FUNC(0x578830, const char*, , GetLocalizedString, uint32_t key);

	FUNC(0x627840, void, __thiscall, GIcon_Spawn, void* icon);

	FUNC(0x616FE0, uint32_t, , GKnockoutRacer_GetPursuitVehicleKey, bool is_player);

	FUNC(0x626F90, void*, __thiscall, GManager_AllocIcon, void* g_manager, char type, Vector3* position, float rotation, bool is_disposable);

	FUNC(0x433AB0, bool, , GPS_Engage, Vector3* target, float max_deviation, bool always_re_establish);

	FUNC(0x422730, void*, __thiscall, GRacerInfo_GetSimable, void* g_racer_info);
	FUNC(0x61B8F0, void, __thiscall, GRacerInfo_SetSimable, void* g_racer_info, void* simable);

	FUNC(0x624E80, void*, __thiscall, GRaceStatus_GetRacePursuitTarget, void* g_race_status, int* index);
	FUNC(0x612230, int, __thiscall, GRaceStatus_GetRacerCount, void* g_race_status);
	FUNC(0x6121E0, void*, __thiscall, GRaceStatus_GetRacerInfo, void* g_race_status, int index);
	FUNC(0x629670, void*, __thiscall, GRaceStatus_GetRacerInfo2, void* g_race_status, void* simable);

	FUNC(0x7BF9B0, void, , KillSkidsOnRaceRestart);

	FUNC(0x75DA60, void, __thiscall, LocalPlayer_ResetHUDType, void* local_player, int hud_type);

	FUNC(0x6C6740, bool, __thiscall, PhysicsObject_Attach, void* physics_object, void* player);
	FUNC(0x6D6C40, void*, __thiscall, PhysicsObject_GetPlayer, void* physics_object);
	FUNC(0x6D6CD0, void*, __thiscall, PhysicsObject_GetRigidBody, void* physics_object);
	FUNC(0x6D19A0, void, __thiscall, PhysicsObject_Kill, void* physics_object);

	FUNC(0x6C0BE0, void, __thiscall, PVehicle_Activate, void* pvehicle);
	FUNC(0x6C0C00, void, __thiscall, PVehicle_Deactivate, void* pvehicle);
	FUNC(0x6C0C40, void, __thiscall, PVehicle_ForceStopOn, void* pvehicle, uint8_t force_stop);
	FUNC(0x6C0C70, void, __thiscall, PVehicle_ForceStopOff, void* pvehicle, uint8_t force_stop);
	FUNC(0x6D8110, void*, __thiscall, PVehicle_GetAIVehiclePtr, void* pvehicle);
	FUNC(0x6D7F60, nfsc::driver_class, __thiscall, PVehicle_GetDriverClass, void* pvehicle);
	FUNC(0x6D7F80, uint8_t, __thiscall, PVehicle_GetForceStop, void* pvehicle);
	FUNC(0x6D7EC0, void*, __thiscall, PVehicle_GetSimable, void* pvehicle);
	FUNC(0x6D8070, float, __thiscall, PVehicle_GetSpeed, void* pvehicle);
	FUNC(0x6D7F20, char*, __thiscall, PVehicle_GetVehicleName, void* pvehicle);
	FUNC(0x6C0BA0, void, __thiscall, PVehicle_GlareOn, void* pvehicle, uint32_t fx_id);
	FUNC(0x6D43A0, void, __thiscall, PVehicle_Kill, void* pvehicle);
	FUNC(0x6D4410, void, __thiscall, PVehicle_ReleaseBehaviorAudio, void* pvehicle);
	FUNC(0x6DA500, void, __thiscall, PVehicle_SetDriverClass, void* pvehicle, driver_class dc);
	FUNC(0x6C61E0, void, __thiscall, PVehicle_SetSpeed, void* pvehicle, float speed);
	FUNC(0x6D1100, bool, __thiscall, PVehicle_SetVehicleOnGround, void* pvehicle, Vector3* position, Vector3* forward_vector);
	
	FUNC(0x6C70E0, void, __thiscall, RigidBody_GetForwardVector, void* rigid_body, Vector3* forward_vector);
	FUNC(0x6C6FF0, Vector3*, __thiscall, RigidBody_GetPosition, void* rigid_body);
	FUNC(0x6E8210, void, __thiscall, RigidBody_SetPosition, void* rigid_body, Vector3* position);

	FUNC(0x816DF0, bool, __thiscall, WCollisionMgr_GetWorldHeightAtPointRigorous, WCollisionMgr* mgr, Vector3* point, float* height, Vector3* normal);

	FUNC(0x7CA1A0, void, , World_RestoreProps);

	FUNC(0x5ACA90, void, __thiscall, WorldMap_GetPanFromMapCoordLocation, void* world_map, Vector2* output, Vector2* input);
	FUNC(0x582C30, void, , WorldMap_SetGPSIng, void* icon);

	FUNC(0x7FB090, bool, __thiscall, WRoadNav_FindPath, void* roadnav, Vector3* goal_position, Vector3* goal_direction, bool shortcuts_allowed);

	/* ===== CUSTOM FUNCTIONS ===== */

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
		void* simable = reinterpret_cast<void* (*)(uint32_t, VehicleParams)>(0x41F920)(0x1396EBE1, params);

		// Attrib::Instance::~Instance(&params.attributes);
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x469870)(reinterpret_cast<uintptr_t>(&params) + 0x14);

		return simable;
	}

	void* BulbToys_CreatePursuitSimable(nfsc::driver_class dc);

	template <uintptr_t handle>
	inline void* BulbToys_FindInterface(void* iface)
	{
		uintptr_t ucom_object = ReadMemory<uintptr_t>(reinterpret_cast<uintptr_t>(iface) + 4);

		// IInterface* UTL::COM::Object::_IList::Find(UCOM::Object::_IList*, IInterface::IHandle);
		return reinterpret_cast<void*(__thiscall*)(uintptr_t, uintptr_t)>(0x60CB50)(ucom_object, handle);
	}

	void* BulbToys_GetAIVehicleGoal(void* ai_vehicle_ivehicleai);

	inline float BulbToys_GetDistanceBetween(void* simable1, void* simable2)
	{
		void* rb1 = nfsc::PhysicsObject_GetRigidBody(simable1);
		void* rb2 = nfsc::PhysicsObject_GetRigidBody(simable2);

		// UMath::Distance
		return reinterpret_cast<float(*)(Vector3*, Vector3*)>(0x411FD0)(nfsc::RigidBody_GetPosition(rb1), nfsc::RigidBody_GetPosition(rb2));
	}

	inline float BulbToys_GetDistanceBetween(void* simable, Vector3* pos)
	{
		void* rb = nfsc::PhysicsObject_GetRigidBody(simable);

		// UMath::Distance
		return reinterpret_cast<float(*)(Vector3*, Vector3*)>(0x411FD0)(nfsc::RigidBody_GetPosition(rb), pos);
	}

	int BulbToys_GetPVehicleTier(void* pvehicle);

	race_type BulbToys_GetRaceType();

	bool BulbToys_GetDebugCamCoords(Vector3& coords);

	bool BulbToys_IsGPSDown();

	void BulbToys_PathToTarget(void* ai_vehicle, Vector3* target);

	enum class sv_mode
	{
		// One-way switch. Use only when creating your own simable, not recommended otherwise, as it kills the old vehicle (TODO: test if this matters)
		one_way,

		// TODO: Two-way switch, with teleportation
		two_way,

		// TODO: Two-way switch, without teleportation
		two_way_no_tp
	};

	bool BulbToys_SwitchVehicle(void* simable, void* simable2, sv_mode mode);

	/*
	inline void BulbToys_SetCopActions(void* ai_goal)
	{
		nfsc::AIGoal_ClearAllActions(ai_goal);

		nfsc::AIGoal_AddAction(ai_goal, "AIActionPursuitOffRoad");
		nfsc::AIGoal_AddAction(ai_goal, "AIActionStunned");
		nfsc::AIGoal_AddAction(ai_goal, "AIActionGetUnstuck");

		nfsc::AIGoal_ChooseAction(ai_goal, 0.0);
	}

	inline void BulbToys_SetRacerActions(void* ai_goal)
	{
		nfsc::AIGoal_ClearAllActions(ai_goal);

		nfsc::AIGoal_AddAction(ai_goal, "AIActionRace");
		nfsc::AIGoal_AddAction(ai_goal, "AIActionStunned");
		nfsc::AIGoal_AddAction(ai_goal, "AIActionGetUnstuck");

		nfsc::AIGoal_ChooseAction(ai_goal, 0.0);
	}
	*/

	void __fastcall BulbToys_SwitchPTagTarget(void* race_status, bool busted);

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

		static AIPlayer* CreateInstance();

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

		static void* __fastcall GetSimable_IPlayer(AIPlayer* ai_player);

		static Vector3* __fastcall GetPosition_IPlayer(AIPlayer* ai_player)
		{
			void* simable = GetSimable_IPlayer(ai_player);
			if (!simable)
			{
				return nfsc::ZeroV3;
			}

			void* rigid_body = nfsc::PhysicsObject_GetRigidBody(simable);
			if (!rigid_body)
			{
				return nfsc::ZeroV3;
			}

			return RigidBody_GetPosition(rigid_body);
		}
		static bool __fastcall SetPosition_IPlayer(AIPlayer* ai_player, int edx, Vector3* pos)
		{
			void* simable = GetSimable_IPlayer(ai_player);
			if (!simable)
			{
				return false;
			}

			void* rigid_body = nfsc::PhysicsObject_GetRigidBody(simable);
			if (!rigid_body)
			{
				return false;
			}

			nfsc::RigidBody_SetPosition(rigid_body, pos);
			return true;
		}
	};
}