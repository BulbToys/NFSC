#include "shared.h"

uint32_t nfsc::BulbToys_AddToListableSet_GetGrowSizeVirtually(uintptr_t vtbl, void* ls, uint32_t amount)
{
	VTable<6>* vtable = reinterpret_cast<VTable<6>*>(vtbl);

	return reinterpret_cast<uint32_t(__thiscall*)(void*, uint32_t)>(vtable->f[3])(ls, amount);
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

void* nfsc::BulbToys_CreatePursuitSimable(nfsc::driver_class dc)
{
	uint32_t cop_key = nfsc::GKnockoutRacer_GetPursuitVehicleKey(1);
	nfsc::Vector3 p = { 0, 0, 0 };
	nfsc::Vector3 r = { 1, 0, 0 };

	void* simable = nfsc::BulbToys_CreateSimable(ReadMemory<void*>(0xA98284), dc, cop_key, &r, &p, nfsc::vehicle_param_flags::critical, nullptr, nullptr);

	/*
	void* vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(simable);

	// PVehicle::SetBehaviorOverride(vehicle, sh32("BEHAVIOR_MECHANIC_AI"), sh32("AIVehicleCopCar"));
	reinterpret_cast<void(__thiscall*)(void*, uint32_t, uint32_t)>(0x6D8E10)(vehicle, 0x4F8F901C, 0x9F128A92);

	// PVehicle::CommitBehaviorOverrides(vehicle);
	reinterpret_cast<void(__thiscall*)(void*)>(0x6DAA60)(vehicle);
	*/

	return simable;
}

int nfsc::BulbToys_GetPVehicleTier(void* pvehicle)
{
	uintptr_t attributes = reinterpret_cast<uintptr_t>(pvehicle) + 0xF0;

	uintptr_t layout_ptr = ReadMemory<uintptr_t>(attributes + 4);
	
	if (!layout_ptr)
	{
		return 0;
	}

	return ReadMemory<int>(layout_ptr + 0x2C);
}

void* nfsc::BulbToys_GetAIVehicleGoal(void* ai_vehicle_ivehicleai)
{
	return ReadMemory<void*>(reinterpret_cast<uintptr_t>(ai_vehicle_ivehicleai) + 0x94);
}

void __fastcall nfsc::BulbToys_SwitchPTagTarget(void* race_status, bool busted)
{
	int runner_index = -1;
	void* runner_simable = nfsc::GRaceStatus_GetRacePursuitTarget(race_status, &runner_index);

	int min_index = -1;
	float min = FLT_MAX;
	void* min_simable = nullptr;

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
			min_simable = simable;
		}
	}

	// unfuck stats here?
	/*
	uintptr_t racer_info_runner = reinterpret_cast<uintptr_t>(nfsc::GRaceStatus_GetRacerInfo(race_status, runner_index));

	WriteMemory<

	uintptr_t racer_info_min = reinterpret_cast<uintptr_t>(nfsc::GRaceStatus_GetRacerInfo(race_status, min_index));

	WriteMemory
	*/

	nfsc::Game_TagPursuit(runner_index, min_index, true);
	nfsc::Game_TagPursuit(runner_index, min_index, true);

	nfsc::Game_TagPursuit(runner_index, min_index, busted);

	min_simable = nfsc::GRacerInfo_GetSimable(nfsc::GRaceStatus_GetRacerInfo(race_status, min_index));

	// Min index/simable is our new runner
	for (int i = 0; i < nfsc::GRaceStatus_GetRacerCount(race_status); i++)
	{
		void* racer_info = nfsc::GRaceStatus_GetRacerInfo(race_status, i);
		void* simable = nfsc::GRacerInfo_GetSimable(racer_info);
		void* vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(simable);
		void* ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(vehicle);
		//void* ai_goal = nfsc::BulbToys_GetAIVehicleGoal(ai_vehicle);

		if (ReadMemory<uintptr_t>(reinterpret_cast<uintptr_t>(ai_vehicle)) != 0x9C4908)
		{
			continue;
		}

		if (i == min_index)
		{
			// aivehiclepursuit flee
			//reinterpret_cast<void(__thiscall*)(void*)>(0x4198F0)(ai_vehicle);

			//BulbToys_SetRacerActions(ai_goal);
			//reinterpret_cast<void(__thiscall*)(void*)>(0x40BE60)(reinterpret_cast<void* (__thiscall*)(void*)>(0x43BC30)(ai_vehicle));

			continue;
		}


		//nfsc::Game_SetPursuitTarget(simable, min_simable);

		// aivehiclepursuit startpursuit
		//reinterpret_cast<void(__thiscall*)(void*, void*, void*)>(0x4320C0)(ai_vehicle, 0, min_simable);
		//BulbToys_SetCopActions(ai_goal);
	}
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

__declspec(noinline) nfsc::AIPlayer* nfsc::AIPlayer::CreateInstance()
{
	// void* FastMem::Alloc(&FastMem, size, 0);
	auto malloc = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint32_t, const char*)>(0x60BA70)
		(nfsc::FastMem, sizeof(nfsc::AIPlayer), 0);
	if (!malloc)
	{
		return nullptr;
	}

	// Sim::Entity::Entity(this);
	auto entity = reinterpret_cast<void*(__thiscall*)(uintptr_t)>(0x76C5A0)(malloc);

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

	BulbToys_AddToListableSet<void*>(nfsc::EntityList, &ai_player->Sim_Entity, 0x6C9900);
	BulbToys_AddToListableSet<void*>(nfsc::IPlayerList, &ai_player->IPlayer, 0x6C9890);

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

	return reinterpret_cast<void* (__thiscall*)(uintptr_t)>(vtable->f[3])(reinterpret_cast<uintptr_t>(ai_player) - 0x20);
}
