#include "shared.h"

uint32_t nfsc::ListableSet_GetGrowSizeVirtually(void* ls, uint32_t amount)
{
	return reinterpret_cast<uint32_t(__thiscall*)(void*, uint32_t)>(VirtualFunction(reinterpret_cast<uintptr_t>(ls), 3))(ls, amount);
}

uintptr_t nfsc::BulbToys_CreatePursuitSimable(nfsc::driver_class dc)
{
	uint32_t cop_key = nfsc::GKnockoutRacer_GetPursuitVehicleKey(1);
	nfsc::Vector3 p = { 0, 0, 0 };
	nfsc::Vector3 r = { 1, 0, 0 };

	uintptr_t simable = nfsc::BulbToys_CreateSimable(ReadMemory<uintptr_t>(0xA98284), dc, cop_key, &r, &p, nfsc::vehicle_param_flags::critical, 0, 0);

	return simable;
}

void nfsc::BulbToys_DrawObject(ImDrawList* draw_list, Vector3& position, Vector3& dimension, Vector3& fwd_vec, ImVec4& color, float thickness)
{
	nfsc::Matrix4 rotation;
	nfsc::Util_GenerateMatrix(&rotation, &fwd_vec, 0);

	nfsc::Vector3 dots[8] = {
		{ dimension.x, dimension.y, dimension.z },
		{ -dimension.x, dimension.y, dimension.z },
		{ dimension.x, dimension.y, -dimension.z },
		{ -dimension.x, dimension.y, -dimension.z },
		{ dimension.x, -dimension.y, dimension.z },
		{ -dimension.x, -dimension.y, dimension.z },
		{ dimension.x, -dimension.y, -dimension.z },
		{ -dimension.x, -dimension.y, -dimension.z },
	};

	for (int i = 0; i < 8; i++)
	{
		// UMath::Rotate
		reinterpret_cast<void(*)(nfsc::Vector3*, nfsc::Matrix4*, nfsc::Vector3*)>(0x401B50)(&dots[i], &rotation, &dots[i]);

		dots[i].x += position.x;
		dots[i].y += position.y;
		dots[i].z += position.z;

		nfsc::BulbToys_GetScreenPosition(dots[i], dots[i]);
	}

	ImVec2 connections[12] = {
		{0,1},
		{0,2},
		{0,4},
		{1,3},
		{1,5},
		{2,3},
		{2,6},
		{3,7},
		{4,5},
		{4,6},
		{5,7},
		{6,7}
	};

	for (int i = 0; i < 12; i++)
	{
		int p1 = (int)connections[i].x;
		int p2 = (int)connections[i].y;

		if (dots[p1].z < 1.0f && dots[p2].z < 1.0f)
		{
			draw_list->AddLine({ dots[p1].x, dots[p1].y }, { dots[p2].x, dots[p2].y }, ImGui::ColorConvertFloat4ToU32(color), thickness);
		}
	}
}

void nfsc::BulbToys_DrawVehicleInfo(ImDrawList* draw_list, uintptr_t vehicle, vehicle_list type, ImVec4& color)
{
	constexpr float max_distance = 100.f;

	// Vehicle can be either a pointer or the IVehicleList array index. If vehicle is a really small number, assume it's an array index
	// For array indexes, just grab the vehicle at that index. Otherwise, iterate the list until the iterator pointer matches our pointer
	uint32_t id = 0;
	if (vehicle < nfsc::VehicleList[type]->size)
	{
		id = vehicle;
		vehicle = nfsc::VehicleList[type]->begin[id];
	}
	else
	{
		for (id = 0; id < nfsc::VehicleList[type]->size; id++)
		{
			if (nfsc::VehicleList[type]->begin[id] == vehicle)
			{
				break;
			}
		}

		if (id == nfsc::VehicleList[type]->size)
		{
			return;
		}
	}

	uintptr_t other_rb = nfsc::PhysicsObject_GetRigidBody(nfsc::PVehicle_GetSimable(vehicle));
	Vector3 other_position = *nfsc::RigidBody_GetPosition(other_rb);

	float distance = nfsc::Sim_DistanceToCamera(&other_position);
	if (distance > max_distance)
	{
		return;
	}

	Vector3 other_dims;
	nfsc::RigidBody_GetDimension(other_rb, &other_dims);

	other_position.y += other_dims.y * 5;

	nfsc::BulbToys_GetScreenPosition(other_position, other_position);

	// why the fuck
	if (!isfinite(other_position.x) || !isfinite(other_position.y) || !isfinite(other_position.z))
	{
		return;
	}

	char text[32];
	sprintf_s(text, 32, "%s (%d)", nfsc::PVehicle_GetVehicleName(vehicle), id);

	ImVec2 text_size = ImGui::CalcTextSize(text);
	if (other_position.z < 1.0)
	{
		ImVec4 bg_color = color;
		bg_color.x /= 5;
		bg_color.y /= 5;
		bg_color.z /= 5;

		draw_list->AddRectFilled({ other_position.x - text_size.x / 2, other_position.y },
			{ other_position.x + text_size.x / 2, other_position.y + text_size.y}, ImGui::ColorConvertFloat4ToU32(bg_color), 1.0f);

		draw_list->AddText({ other_position.x - text_size.x / 2, other_position.y }, ImGui::ColorConvertFloat4ToU32(color), text);
	}
}

uintptr_t nfsc::BulbToys_GetAIVehicleGoal(uintptr_t ai_vehicle_ivehicleai)
{
	return ReadMemory<uintptr_t>(ai_vehicle_ivehicleai + 0x94);
}

const char* nfsc::BulbToys_GetCameraName()
{
	if (*nfsc::GameFlowManager_State != nfsc::gameflow_state::racing)
	{
		return "";
	}

	uintptr_t first_camera_director = ReadMemory<uintptr_t>(0xA8ACC4 + 4);
	uintptr_t camera_director = ReadMemory<uintptr_t>(first_camera_director);
	uintptr_t cd_action = ReadMemory<uintptr_t>(camera_director + 0x18);

	// CameraAI::Action::GetName(this)->mString;
	return ReadMemory<char*>(reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(VirtualFunction(cd_action, 3))(cd_action) + 4);
}

// NOTE: Returns tier 0 correctly (ie. Dump Truck). Returns -1 if it can't find its attributes.
int nfsc::BulbToys_GetPVehicleTier(uintptr_t pvehicle)
{
	uintptr_t attributes = pvehicle + 0xF0;

	uintptr_t layout_ptr = ReadMemory<uintptr_t>(attributes + 4);

	if (!layout_ptr)
	{
		return -1;
	}

	return ReadMemory<int>(layout_ptr + 0x2C);
}

void nfsc::BulbToys_GetScreenPosition(Vector3& world, Vector3& screen)
{
	nfsc::Vector4 out, input { world.z, -world.x, world.y, 1.0 };

	nfsc::Matrix4 proj = ReadMemory<nfsc::Matrix4>((0xB1A780 + 1 * 0x1A0) + 0x80);

	// D3DXVec4Transform
	reinterpret_cast<nfsc::Vector4*(__stdcall*)(nfsc::Vector4*, nfsc::Vector4*, nfsc::Matrix4*)>(0x86B29C)(&out, &input, &proj);

	float i_w = 1.0f / out.w;
	out.x *= i_w;
	out.y *= i_w;

	screen.x = (out.x + 1.0f) * ReadMemory<int>(0xAB0AC8) * 0.5f;
	screen.y = (out.y - 1.0f) * ReadMemory<int>(0xAB0ACC) * -0.5f;
	screen.z = i_w * out.z;
}

float nfsc::BulbToys_GetStreetWidth(Vector3* position, Vector3* direction, float distance, Vector3* left_pos, Vector3* right_pos)
{
	struct WRoadNav // 780u
	{ 
		uint8_t pad0[0x58];

		bool fValid = false;
		
		uint8_t pad1[0x1B]{ 0 };

		int fNavType = 0;

		uint8_t pad2[0x10]{ 0 };

		char fNodeInd = 0;

		uint8_t pad3 = 0;

		short fSegmentInd = 0;

		uint8_t pad4[0x14]{ 0 };

		nfsc::Vector3 fLeftPosition = { 0, 0, 0 };
		nfsc::Vector3 fRightPosition = { 0, 0, 0 };

		uint8_t pad5[0x254]{ 0 };

	} nav;

	// WRoadNav::WRoadNav(&nav);
	reinterpret_cast<uintptr_t(__thiscall*)(WRoadNav&)>(0x806820)(nav);

	// WRoadNav::SetCookieTrail(&nav, 1);
	reinterpret_cast<void(__thiscall*)(WRoadNav&, bool)>(0x7F7CA0)(nav, 1);

	// WRoadNav::SetPathType(&nav, kPathCop);
	reinterpret_cast<void(__thiscall*)(WRoadNav&, int)>(0x7EC0D0)(nav, 0);

	// nav.fNavType = kTypeDirection;
	nav.fNavType = 2;

	// WRoadNav::InitAtPoint(&nav, &position, &direction, 0, 0.0);
	reinterpret_cast<void(__thiscall*)(WRoadNav&, Vector3*, Vector3*, bool, float)>(0x80F180)(nav, position, direction, 0, 0.0);
	if (!nav.fValid)
	{
		return NAN;
	}

	// WRoadNav::IncNavPosition(&nav, distance, &UMath::Vector3::kZero, 0.0, 0);
	reinterpret_cast<void(__thiscall*)(WRoadNav&, float, Vector3*, float, bool)>(0x80C600)(nav, distance, nfsc::ZeroV3, 0.0, 0);

	// segment = &WRoadNetwork::fSegments[nav.fSegmentInd];
	uintptr_t segment = ReadMemory<uintptr_t>(0xB77ECC) + 0x16 * nav.fSegmentInd;
	if (!segment)
	{
		return NAN;
	}

	uint16_t segment_flags = ReadMemory<uint16_t>(segment + 0xA);

	// (segment->fFlags & 2) != 0
	if ((segment_flags & 2) != 0)
	{
		return NAN;
	}

	// (segment->fFlags & 1) != 0
	if ((segment_flags & 1) != 0)
	{
		return NAN;
	}

	// segment->nLength * flt_00A83DA8 < flt_009C1D30 )
	if (ReadMemory<uint16_t>(segment + 0x4) * 0.015259022 < 40.0)
	{
		return NAN;
	}

	// segment_profile = WRoadNetwork::GetSegmentProfile(WRoadNetwork::fgRoadNetwork, segment, nav.fNodeInd);
	uintptr_t segment_profile = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uintptr_t, int)>(0x7EB290)(ReadMemory<uintptr_t>(0xB77EC0), segment, nav.fNodeInd);

	// !segment_profile->fNumZones
	if (!segment_profile || !ReadMemory<char>(segment_profile))
	{
		return NAN;
	}

	// Return various necessary WRoadNav parameters
	if (left_pos)
	{
		*left_pos = nav.fLeftPosition;
	}
	if (right_pos)
	{
		*right_pos = nav.fRightPosition;
	}

	// nav.fRightPosition & nav.fLeftPosition
	float width = nfsc::UMath_Distance(&nav.fRightPosition, &nav.fLeftPosition);

	// WRoadNav::~WRoadNav(&nav);
	reinterpret_cast<void(__thiscall*)(WRoadNav&)>(0x7F7BF0)(nav);

	return width;
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

// Returns false if we're not in Debug Cam
bool nfsc::BulbToys_GetDebugCamCoords(nfsc::Vector3* position , nfsc::Vector3* fwd_vec)
{
	if (*nfsc::GameFlowManager_State != nfsc::gameflow_state::racing)
	{
		return false;
	}

	uintptr_t first_camera_director = ReadMemory<uintptr_t>(0xA8ACC4 + 4);
	uintptr_t camera_director = ReadMemory<uintptr_t>(first_camera_director);
	uintptr_t cd_action = ReadMemory<uintptr_t>(camera_director + 0x18);

	// Check if we're in CDActionDebug (CDActionDebug::`vftable'{for `CameraAI::Action'})
	if (ReadMemory<uintptr_t>(cd_action) != 0x9C7EE0)
	{
		return false;
	}

	uintptr_t camera_mover = ReadMemory<uintptr_t>(cd_action + 0x2BC);
	uintptr_t camera = ReadMemory<uintptr_t>(camera_mover + 0x1C);

	if (position)
	{
		// z, -x, y -> x, y, z
		position->x = -ReadMemory<float>(camera + 0x44);
		position->y = ReadMemory<float>(camera + 0x48);
		position->z = ReadMemory<float>(camera + 0x40);
	}

	if (fwd_vec)
	{
		// z, -x, y -> x, y, z
		fwd_vec->x = -ReadMemory<float>(camera + 0x54);
		fwd_vec->y = ReadMemory<float>(camera + 0x58);
		fwd_vec->z = ReadMemory<float>(camera + 0x50);
	}

	return true;
}

bool nfsc::BulbToys_GetMyVehicle(uintptr_t* my_vehicle, uintptr_t* my_simable)
{
	if (*nfsc::GameFlowManager_State == nfsc::gameflow_state::racing)
	{
		for (int i = 0; i < (int)nfsc::VehicleList[nfsc::vehicle_list::players]->size; i++)
		{
			uintptr_t vehicle = nfsc::VehicleList[nfsc::vehicle_list::players]->begin[i];

			if (vehicle)
			{
				uintptr_t simable = nfsc::PVehicle_GetSimable(vehicle);

				if (simable)
				{
					uintptr_t player = nfsc::PhysicsObject_GetPlayer(simable);

					// RecordablePlayer::`vftable'{for `IPlayer'}
					if (player && ReadMemory<uintptr_t>(player) == 0x9EC8C0)
					{
						if (my_vehicle)
						{
							*my_vehicle = vehicle;
						}
						if (my_simable)
						{
							*my_simable = simable;
						}
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool nfsc::BulbToys_IsGPSDown()
{
	auto gps = ReadMemory<uintptr_t>(0xA83E3C);

	// Check if GPS state == GPS_DOWN
	return !gps || ReadMemory<int>(gps + 0x6C) == 0;
}

void nfsc::BulbToys_PathToTarget(uintptr_t ai_vehicle, Vector3* target)
{
	auto road_nav = ReadMemory<uintptr_t>(ai_vehicle + 0x38);
	if (!road_nav)
	{
		return;
	}

	nfsc::WRoadNav_FindPath(road_nav, target, nullptr, 1);
}

bool nfsc::BulbToys_SwitchVehicle(uintptr_t simable, uintptr_t simable2, sv_mode mode)
{
	if (!simable || !simable2)
	{
		return false;
	}

	uintptr_t player = nfsc::PhysicsObject_GetPlayer(simable);
	if (!player)
	{
		return false;
	}

	uintptr_t vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(simable);
	uintptr_t vehicle2 = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(simable2);
	if (!vehicle || !vehicle2)
	{
		return false;
	}

	uintptr_t rbody = nfsc::PhysicsObject_GetRigidBody(simable);
	uintptr_t rbody2 = nfsc::PhysicsObject_GetRigidBody(simable2);
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
			uintptr_t racer_info = nfsc::GRaceStatus_GetRacerInfo2(ReadMemory<uintptr_t>(nfsc::GRaceStatus), simable);
			nfsc::GRacerInfo_SetSimable(racer_info, simable2);
		}

		/*
		int c = 0;

		// Retarget all necessary AITargets to our new vehicle
		for (int i = 0; i < nfsc::AITargetsList->size; i++)
		{
			uintptr_t ai_target = nfsc::AITargetsList->begin[i];

			uintptr_t ai_target_vehicle = nullptr;
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

	nfsc::EAXSound_StartNewGamePlay(ReadMemory<uintptr_t>(0xA8BA38));

	// Call ResetHUDType virtually (ie. our AIPlayer class uses OnlineRemotePlayer::ResetHUDType)
	reinterpret_cast<decltype(nfsc::LocalPlayer_ResetHUDType)>(VirtualFunction(player, 9))(player, 1);

	return true;
}

void __fastcall nfsc::BulbToys_SwitchPTagTarget(uintptr_t race_status, bool busted)
{
	// Store information about our current runner here
	int runner_index = -1;
	uintptr_t runner_simable = nfsc::GRaceStatus_GetRacePursuitTarget(race_status, &runner_index);

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

		uintptr_t racer_info = nfsc::GRaceStatus_GetRacerInfo(race_status, i);
		uintptr_t simable = nfsc::GRacerInfo_GetSimable(racer_info);

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
nfsc::AIPlayer* nfsc::AIPlayer::New()
{
	// uintptr_t FastMem::Alloc(&FastMem, size, 0);
	auto malloc = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint32_t, const char*)>(0x60BA70)(nfsc::FastMem, sizeof(nfsc::AIPlayer), 0);
	if (!malloc)
	{
		return nullptr;
	}

	// Sim::Entity::Entity(this);
	auto entity = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(0x76C5A0)(malloc);

	auto ai_player = reinterpret_cast<nfsc::AIPlayer*>(entity);

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

	nfsc::EntityList->Add(reinterpret_cast<uintptr_t>(&ai_player->Sim_Entity.Sim_IEntity), 0x6C9900);
	nfsc::IPlayerList->Add(reinterpret_cast<uintptr_t>(&ai_player->IPlayer), 0x6C9890);

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

uintptr_t nfsc::AIPlayer::GetSimable_IPlayer(AIPlayer* ai_player)
{
	VTable<10>* vtable = reinterpret_cast<VTable<10>*>(ai_player->Sim_Entity.Sim_IEntity.UCOM_IUnknown.vtbl);

	// Sim::Entity::GetSimable(Sim::IEntity *this)
	return reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(vtable->f[3])(reinterpret_cast<uintptr_t>(ai_player) - 0x20);
}