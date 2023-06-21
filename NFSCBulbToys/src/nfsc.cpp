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
		reinterpret_cast<void(__thiscall*)(uintptr_t, void*, uint32_t, const char*)>(0x609E80)(ReadMemory<uintptr_t>(nfsc::FastMem), ai_player, sizeof(AIPlayer), 0);
	}
	return ai_player;
}

void* nfsc::AIPlayer::GetSimable_IPlayer(AIPlayer* ai_player)
{
	VTable<10>* vtable = reinterpret_cast<VTable<10>*>(ai_player->Sim_Entity.Sim_IEntity.UCOM_IUnknown.vtbl);

	return reinterpret_cast<void* (__thiscall*)(uintptr_t)>(vtable->f[3])(reinterpret_cast<uintptr_t>(ai_player) - 0x20);
}
