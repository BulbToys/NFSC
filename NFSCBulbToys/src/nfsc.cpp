#include "shared.h"

uint32_t nfsc::ListableSet_GetGrowSizeVirtually(void* ls, uint32_t amount)
{
	return reinterpret_cast<uint32_t(__thiscall*)(void*, uint32_t)>(VirtualFunction(ls, 3))(ls, amount);
}

void* nfsc::BulbToys_CreatePursuitSimable(nfsc::driver_class dc)
{
	uint32_t cop_key = nfsc::GKnockoutRacer_GetPursuitVehicleKey(1);
	nfsc::Vector3 p = { 0, 0, 0 };
	nfsc::Vector3 r = { 1, 0, 0 };

	void* simable = nfsc::BulbToys_CreateSimable(ReadMemory<void*>(0xA98284), dc, cop_key, &r, &p, nfsc::vehicle_param_flags::critical, nullptr, nullptr);

	return simable;
}

void* nfsc::BulbToys_GetAIVehicleGoal(void* ai_vehicle_ivehicleai)
{
	return ReadMemory<void*>(reinterpret_cast<uintptr_t>(ai_vehicle_ivehicleai) + 0x94);
}

// NOTE: Returns tier 0 correctly (ie. Dump Truck). Returns -1 if it can't find its attributes.
int nfsc::BulbToys_GetPVehicleTier(void* pvehicle)
{
	uintptr_t attributes = reinterpret_cast<uintptr_t>(pvehicle) + 0xF0;

	uintptr_t layout_ptr = ReadMemory<uintptr_t>(attributes + 4);

	if (!layout_ptr)
	{
		return -1;
	}

	return ReadMemory<int>(layout_ptr + 0x2C);
}

nfsc::race_type nfsc::BulbToys_GetRaceType()
{
	uintptr_t g_race_status = ReadMemory<uintptr_t>(nfsc::GRaceStatus);
	if (!g_race_status)
	{
		return nfsc::race_type::none;
	}

	uintptr_t race_parameters = ReadMemory<uintptr_t>(g_race_status + 0x6A1C);
	if (!race_parameters)
	{
		return nfsc::race_type::none;
	}

	return reinterpret_cast<nfsc::race_type(__thiscall*)(uintptr_t)>(0x6136A0)(race_parameters);
}

bool nfsc::BulbToys_GetDebugCamCoords(nfsc::Vector3& coords)
{
	uintptr_t first_camera_director = ReadMemory<uintptr_t>(0xA8ACC4 + 4);

	uintptr_t camera_director = ReadMemory<uintptr_t>(first_camera_director);

	uintptr_t cd_action = ReadMemory<uintptr_t>(camera_director + 0x18);

	// Check if we're in CDActionDebug
	if (ReadMemory<uintptr_t>(cd_action) != 0x9C7EE0)
	{
		return false;
	}

	uintptr_t camera_mover = ReadMemory<uintptr_t>(cd_action + 0x2BC);

	uintptr_t camera = ReadMemory<uintptr_t>(camera_mover + 0x1C);

	// z, -x, y -> x, y, z
	coords.x = -ReadMemory<float>(camera + 0x44);
	coords.y = ReadMemory<float>(camera + 0x48);
	coords.z = ReadMemory<float>(camera + 0x40);

	return true;
}

bool nfsc::BulbToys_IsGPSDown()
{
	auto gps = ReadMemory<uintptr_t>(0xA83E3C);

	// Check if GPS state == GPS_DOWN
	return !gps || ReadMemory<int>(gps + 0x6C) == 0;
}

void nfsc::BulbToys_PathToTarget(void* ai_vehicle, Vector3* target)
{
	auto road_nav = ReadMemory<void*>(reinterpret_cast<uintptr_t>(ai_vehicle) + 0x38);
	if (!road_nav)
	{
		return;
	}

	nfsc::WRoadNav_FindPath(road_nav, target, nullptr, 1);
}

bool nfsc::BulbToys_SwitchVehicle(void* simable, void* simable2, sv_mode mode)
{
	if (!simable || !simable2)
	{
		return false;
	}

	void* player = nfsc::PhysicsObject_GetPlayer(simable);
	if (!player)
	{
		return false;
	}

	void* vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(simable);
	void* vehicle2 = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(simable2);
	if (!vehicle || !vehicle2)
	{
		return false;
	}

	void* rbody = nfsc::PhysicsObject_GetRigidBody(simable);
	void* rbody2 = nfsc::PhysicsObject_GetRigidBody(simable2);
	if (!rbody || !rbody2)
	{
		return false;
	}

	if (mode == sv_mode::one_way)
	{
		nfsc::driver_class dc = nfsc::PVehicle_GetDriverClass(vehicle);
		nfsc::PVehicle_SetDriverClass(vehicle, nfsc::driver_class::none);
		nfsc::PVehicle_ReleaseBehaviorAudio(vehicle);
		nfsc::PVehicle_Deactivate(vehicle);

		Vector3 fwd_vec = { 1, 0, 0 };
		Vector3* pos = nfsc::RigidBody_GetPosition(rbody);
		nfsc::RigidBody_GetForwardVector(rbody, &fwd_vec);
		nfsc::PVehicle_SetVehicleOnGround(vehicle2, pos, &fwd_vec);

		float speed = nfsc::PVehicle_GetSpeed(vehicle);
		nfsc::PVehicle_SetSpeed(vehicle2, speed);

		nfsc::PhysicsObject_Attach(simable2, player);
		nfsc::PVehicle_GlareOn(vehicle2, 0x7); // headlights
		nfsc::PVehicle_Activate(vehicle2);

		uint8_t force_stop = nfsc::PVehicle_GetForceStop(vehicle);

		// todo: useless? (supposed to prevent race start unfreeze? but doesn't)
		if ((force_stop & 0x10) != 0)
		{
			nfsc::PVehicle_ForceStopOn(vehicle, 0x10);
		}
		else
		{
			nfsc::PVehicle_ForceStopOff(vehicle, 0x10);
		}

		if (nfsc::BulbToys_GetRaceType() != nfsc::race_type::none)
		{
			void* racer_info = nfsc::GRaceStatus_GetRacerInfo2(ReadMemory<void*>(nfsc::GRaceStatus), simable);
			nfsc::GRacerInfo_SetSimable(racer_info, simable2);
		}

		/*
		int c = 0;

		// Retarget all necessary AITargets to our new vehicle
		for (int i = 0; i < nfsc::AITargetsList->size; i++)
		{
			void* ai_target = *(nfsc::AITargetsList->begin + i);

			void* ai_target_vehicle = nullptr;
			nfsc::AITarget_GetVehicleInterface(ai_target, &ai_target_vehicle);
			if (vehicle == ai_target_vehicle)
			{
				nfsc::AITarget_Acquire(ai_target, simable2);
				c++;
			}
		}

		Error("Retargeted %d/%d", c, nfsc::AITargetsList->size);
		*/

		// todo: fix pursuits here
		// todo: fix camera
		// todo: eloadingscreenoff/on

		nfsc::PhysicsObject_Kill(simable);
	}

	nfsc::EAXSound_StartNewGamePlay(ReadMemory<void*>(0xA8BA38));

	// Call ResetHUDType virtually (ie. our AIPlayer class uses OnlineRemotePlayer::ResetHUDType)
	reinterpret_cast<decltype(nfsc::LocalPlayer_ResetHUDType)>(VirtualFunction(player, 9))(player, 1);

	return true;
}

void __fastcall nfsc::BulbToys_SwitchPTagTarget(void* race_status, bool busted)
{
	// Store information about our current runner here
	int runner_index = -1;
	void* runner_simable = nfsc::GRaceStatus_GetRacePursuitTarget(race_status, &runner_index);

	// Store information about our soon-to-be runner here
	int min_index = -1;
	float min = FLT_MAX;

	// Find the cop car closest to the current runner's car (minimum distance between the racer and each of the cop cars)
	// TODO: use pursuit contribution instead?
	for (int i = 0; i < nfsc::GRaceStatus_GetRacerCount(race_status); i++)
	{
		if (i == runner_index)
		{
			continue;
		}

		void* racer_info = nfsc::GRaceStatus_GetRacerInfo(race_status, i);
		void* simable = nfsc::GRacerInfo_GetSimable(racer_info);

		float distance = nfsc::BulbToys_GetDistanceBetween(runner_simable, simable);
		if (distance < min)
		{
			min_index = i;
			min = distance;
		}
	}

	// FIXME: Tagging is fucked. The first time the vehicle switch is performed, the game "softlocks"
	// Likely because a vehicle is deactivated when it shouldn't be, or (more likely) a player/entity does not have a simable
	// As a workaround, we're calling Game_TagPursuit two extra times here
	// We specifically set busted to true because it doesn't increment the runner's time incorrectly here (false indicates an evasion, which gives the racer a time bonus)
	// However, we do fuck up the "number of busts/number of times busted" stats for these racers, which will need to be unfucked (TODO?)
	nfsc::Game_TagPursuit(runner_index, min_index, true);
	nfsc::Game_TagPursuit(runner_index, min_index, true);

	// Call Game_TagPursuit as intended
	nfsc::Game_TagPursuit(runner_index, min_index, busted);

	// FIXME: AI TARGETING LOGIC GOES HERE
}

/* ===== AIPLAYER ===== */

// Most of this shit is probably useless garbage the compiler spit out due to inheritance but i'm replicating it for consistency
__declspec(noinline) nfsc::AIPlayer* nfsc::AIPlayer::CreateInstance()
{
	// void* FastMem::Alloc(&FastMem, size, 0);
	auto malloc = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint32_t, const char*)>(0x60BA70)(nfsc::FastMem, sizeof(nfsc::AIPlayer), 0);
	if (!malloc)
	{
		return nullptr;
	}

	// Sim::Entity::Entity(this);
	auto entity = reinterpret_cast<void*(__thiscall*)(uintptr_t)>(0x76C5A0)(malloc);

	// ???
	auto ai_player = reinterpret_cast<nfsc::AIPlayer*>(reinterpret_cast<uintptr_t>(entity) - 0);

	void* object = &ai_player->Sim_Entity.Sim_Object.UCOM_Object;

	// UCOM::IUnknown::`vftable'
	ai_player->IPlayer.vtbl = 0x9C2490;

	ai_player->IPlayer.com_object = reinterpret_cast<uintptr_t>(object);

	void* i_list = &ai_player->Sim_Entity.Sim_Object.UCOM_Object.IList;

	// UTL::COM::Object::_IList::Add(&(object->_mInterfaces), IPlayer::IHandle, &this->IPlayer);
	reinterpret_cast<void(__thiscall*)(void*, uintptr_t, void*)>(0x60DCB0)(i_list, 0x66B460, &ai_player->IPlayer);

	// IPlayer::`vftable';
	ai_player->IPlayer.vtbl = 0x9EC370;

	ai_player->Sim_Entity.Sim_Object.Sim_IServiceable.vtbl = reinterpret_cast<uintptr_t>(g::ai_player::iserviceable_vtbl);

	// Sim::Object::`vftable'{for `Sim::ITaskable'}
	ai_player->Sim_Entity.Sim_Object.Sim_ITaskable.vtbl = 0x9EC104;

	ai_player->Sim_Entity.Sim_IEntity.UCOM_IUnknown.vtbl = reinterpret_cast<uintptr_t>(g::ai_player::ientity_vtbl);
	ai_player->Sim_Entity.IAttachable.vtbl = reinterpret_cast<uintptr_t>(g::ai_player::iattachable_vtbl);
	ai_player->IPlayer.vtbl = reinterpret_cast<uintptr_t>(g::ai_player::iplayer_vtbl);

	nfsc::EntityList->Add(&ai_player->Sim_Entity.Sim_IEntity, 0x6C9900);
	nfsc::IPlayerList->Add(&ai_player->IPlayer, 0x6C9890);

	return ai_player;
}

nfsc::AIPlayer* nfsc::AIPlayer::VecDelDtor(AIPlayer* ai_player, int edx, uint8_t flags)
{
	Destructor(ai_player);
	if (ai_player && (flags & 1) != 0)
	{
		// FastMem::Free(&FastMem, this, size, 0);
		reinterpret_cast<void(__thiscall*)(uintptr_t, void*, uint32_t, const char*)>(0x609E80)(nfsc::FastMem, ai_player, sizeof(AIPlayer), 0);
	}
	return ai_player;
}

void* nfsc::AIPlayer::GetSimable_IPlayer(AIPlayer* ai_player)
{
	VTable<10>* vtable = reinterpret_cast<VTable<10>*>(ai_player->Sim_Entity.Sim_IEntity.UCOM_IUnknown.vtbl);

	// Sim::Entity::GetSimable(Sim::IEntity *this)
	return reinterpret_cast<void*(__thiscall*)(uintptr_t)>(vtable->f[3])(reinterpret_cast<uintptr_t>(ai_player) - 0x20);
}
