#include "shared.h"

uint32_t nfsc::ListableSet_GetGrowSizeVirtually(void* ls, uint32_t amount)
{
	return reinterpret_cast<uint32_t(__thiscall*)(void*, uint32_t)>(VirtualFunction(reinterpret_cast<uintptr_t>(ls), 3))(ls, amount);
}

void nfsc::RoadblockSetup::operator=(const RoadblockSetupFile& rbsf)
{
	memcpy(this, (char*)&rbsf + 4, sizeof(nfsc::RoadblockSetup));
}

void nfsc::RoadblockSetupFile::operator=(const RoadblockSetup& rbs)
{
	memcpy((char*)this + 4, &rbs, sizeof(nfsc::RoadblockSetup));
}

bool nfsc::RoadblockSetupFile::Validate()
{
	int i;
	for (i = 0; i < 6; i++)
	{
		if (i == 0 && (this->rbs.minimum_width < .0f || this->rbs.minimum_width > 100.0f || this->rbs.required_vehicles < 0 || this->rbs.required_vehicles > 6))
		{
			break;
		}

		if ((int)this->rbs.contents[i].type < 0 || (int)this->rbs.contents[i].type > 3 ||
			this->rbs.contents[i].offset_x < -50.0f || this->rbs.contents[i].offset_x > 50.0f ||
			this->rbs.contents[i].offset_z < -50.0f || this->rbs.contents[i].offset_z > 50.0f ||
			this->rbs.contents[i].angle < -1.0f || this->rbs.contents[i].angle > 1.0f)
		{
			break;
		}

		else if (i == 5)
		{
			return true;
		}
	}

	return false;
}

void nfsc::BulbToys_DrawObject(ImDrawList* draw_list, Vector3& position, Vector3& dimension, Vector3& fwd_vec, ImVec4& color, float thickness)
{
	Matrix4 rotation;
	Util_GenerateMatrix(&rotation, &fwd_vec, 0);

	Vector3 dots[8] = {
		{ dimension.x, dimension.y, dimension.z },
		{ -dimension.x, dimension.y, dimension.z },
		{ dimension.x, dimension.y, -dimension.z },
		{ -dimension.x, dimension.y, -dimension.z },
		{ dimension.x, -dimension.y, dimension.z },
		{ -dimension.x, -dimension.y, dimension.z },
		{ dimension.x, -dimension.y, -dimension.z },
		{ -dimension.x, -dimension.y, -dimension.z },
	};

	Vector3 front = {
		(dots[0].x + dots[1].x) / 2,
		(dots[0].y + dots[1].y) / 2 - dimension.y,
		(dots[0].z + dots[1].z) / 2
	};
	reinterpret_cast<void(*)(Vector3*, Matrix4*, Vector3*)>(0x401B50)(&front, &rotation, &front);
	front.x += position.x;
	front.y += position.y;
	front.z += position.z;

	for (int i = 0; i < 8; i++)
	{
		// UMath::Rotate
		reinterpret_cast<void(*)(Vector3*, Matrix4*, Vector3*)>(0x401B50)(&dots[i], &rotation, &dots[i]);

		dots[i].x += position.x;
		dots[i].y += position.y;
		dots[i].z += position.z;

		BulbToys_GetScreenPosition(dots[i], dots[i]);
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

	// Draw the rectangle
	for (int i = 0; i < 12; i++)
	{
		int p1 = (int)connections[i].x;
		int p2 = (int)connections[i].y;

		if (dots[p1].z < 1.0f && dots[p2].z < 1.0f)
		{
			draw_list->AddLine({ dots[p1].x, dots[p1].y }, { dots[p2].x, dots[p2].y }, ImGui::ColorConvertFloat4ToU32(color), thickness);
		}
	}

	// Draw the forward line
	Vector3 center;
	BulbToys_GetScreenPosition(position, center);
	BulbToys_GetScreenPosition(front, front);

	if (center.z < 1.0f && front.z < 1.0f)
	{
		draw_list->AddLine({ center.x, center.y }, { front.x, front.y }, ImGui::ColorConvertFloat4ToU32(color), thickness / 2);
	}
}

void nfsc::BulbToys_DrawVehicleInfo(ImDrawList* draw_list, uintptr_t vehicle, vehicle_list type, ImVec4& color)
{
	// Distance at which the text stops rendering
	constexpr float max_distance = 100.f;

	// Vehicle can be either a pointer or the IVehicleList array index. If vehicle is a really small number, assume it's an array index
	// For array indexes, just grab the vehicle at that index. Otherwise, iterate the list until the iterator pointer matches our pointer
	uint32_t id = 0;
	if (vehicle < VehicleList[type]->size)
	{
		id = vehicle;
		vehicle = VehicleList[type]->begin[id];
	}
	else
	{
		for (id = 0; id < VehicleList[type]->size; id++)
		{
			if (VehicleList[type]->begin[id] == vehicle)
			{
				break;
			}
		}

		if (id == VehicleList[type]->size)
		{
			return;
		}
	}

	uintptr_t other_rb = PhysicsObject_GetRigidBody(PVehicle_GetSimable(vehicle));
	Vector3 other_position = *RigidBody_GetPosition(other_rb);

	float distance = Sim_DistanceToCamera(&other_position);
	if (distance > max_distance)
	{
		return;
	}

	Vector3 other_dims;
	RigidBody_GetDimension(other_rb, &other_dims);

	// Put the text higher above the vehicle
	other_position.y += other_dims.y * 5;

	BulbToys_GetScreenPosition(other_position, other_position);

	// why the fuck
	if (!isfinite(other_position.x) || !isfinite(other_position.y) || !isfinite(other_position.z))
	{
		return;
	}

	char text[32];
	sprintf_s(text, 32, "%s (%d)", PVehicle_GetVehicleName(vehicle), id);

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
	if (*GameFlowManager_State != gameflow_state::racing)
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
	Vector4 out, input = { world.z, -world.x, world.y, 1.0 };

	Matrix4 proj = ReadMemory<Matrix4>((0xB1A780 + 1 * 0x1A0) + 0x80);

	// D3DXVec4Transform
	reinterpret_cast<Vector4*(__stdcall*)(Vector4*, Vector4*, Matrix4*)>(0x86B29C)(&out, &input, &proj);

	float i_w = 1.0f / out.w;
	out.x *= i_w;
	out.y *= i_w;

	screen.x = (out.x + 1.0f) * ReadMemory<int>(0xAB0AC8) * 0.5f;
	screen.y = (out.y - 1.0f) * ReadMemory<int>(0xAB0ACC) * -0.5f;
	screen.z = i_w * out.z;
}

float nfsc::BulbToys_GetStreetWidth(Vector3* position, Vector3* direction, float distance, Vector3* left_pos, Vector3* right_pos, Vector3* fwd_vec)
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

		Vector3 fLeftPosition = { 0, 0, 0 };
		Vector3 fRightPosition = { 0, 0, 0 };
		Vector3 fForwardVector = { 0, 0, 0 };

		uint8_t pad5[0x248]{ 0 };

	} nav;

	constexpr int a = sizeof(nav);

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
	reinterpret_cast<void(__thiscall*)(WRoadNav&, float, Vector3*, float, bool)>(0x80C600)(nav, distance, ZeroV3, 0.0, 0);

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
	if (fwd_vec)
	{
		*fwd_vec = nav.fForwardVector;
	}

	// nav.fRightPosition & nav.fLeftPosition
	float width = UMath_Distance(&nav.fRightPosition, &nav.fLeftPosition);

	// WRoadNav::~WRoadNav(&nav);
	reinterpret_cast<void(__thiscall*)(WRoadNav&)>(0x7F7BF0)(nav);

	return width;
}

nfsc::race_type nfsc::BulbToys_GetRaceType()
{
	uintptr_t g_race_status = ReadMemory<uintptr_t>(GRaceStatus);
	if (!g_race_status)
	{
		return race_type::none;
	}

	uintptr_t race_parameters = ReadMemory<uintptr_t>(g_race_status + 0x6A1C);
	if (!race_parameters)
	{
		return race_type::none;
	}

	return reinterpret_cast<race_type(__thiscall*)(uintptr_t)>(0x6136A0)(race_parameters);
}

// Returns false if we're not in Debug Cam
bool nfsc::BulbToys_GetDebugCamCoords(Vector3* position , Vector3* fwd_vec)
{
	if (*GameFlowManager_State != gameflow_state::racing)
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
	if (*GameFlowManager_State == gameflow_state::racing)
	{
		for (int i = 0; i < (int)VehicleList[vehicle_list::players]->size; i++)
		{
			uintptr_t vehicle = VehicleList[vehicle_list::players]->begin[i];

			if (vehicle)
			{
				uintptr_t simable = PVehicle_GetSimable(vehicle);

				if (simable)
				{
					uintptr_t player = PhysicsObject_GetPlayer(simable);

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

void nfsc::BulbToys_PathToTarget(uintptr_t ai_vehicle, Vector3* target)
{
	auto road_nav = ReadMemory<uintptr_t>(ai_vehicle + 0x38);
	if (!road_nav)
	{
		return;
	}

	WRoadNav_FindPath(road_nav, target, nullptr, 1);
}

// Does not return RoadblockInfo if street width calculation is unsuccessful (ie. width = NAN)
void* nfsc::BulbToys_RoadblockCalculations(RoadblockSetup* setup, uintptr_t rigid_body)
{
	// Reference point for calculating street width
	Vector3 sw_position;
	Vector3 sw_fwd_vec;
	if (!rigid_body)
	{
		if (!BulbToys_GetDebugCamCoords(&sw_position, &sw_fwd_vec))
		{
			return nullptr;
		}
	}
	else
	{
		sw_position = *RigidBody_GetPosition(rigid_body);
		RigidBody_GetForwardVector(rigid_body, &sw_fwd_vec);
	}

	// Distance from reference point to where the roadblock should be calculated
	// This uses the same value as gameplay logic and is adjustable in the GUI
	float distance = ReadMemory<float>(0x44529C);

	Vector3 nav_left_pos, nav_right_pos, nav_fwd_vec;
	float width = BulbToys_GetStreetWidth(&sw_position, &sw_fwd_vec, distance, &nav_left_pos, &nav_right_pos, &nav_fwd_vec) + 1.0f;
	
	// Street width calculation successful
	if (!isnan(width))
	{
		// Create rendering/spawning roadblock info adapter
		gui::RoadblockInfo* ri = new gui::RoadblockInfo();

		ri->width = width;
		ri->line_color = ImVec4(1, 0, 0, 1); // red

		Vector3 left_pos_sp, right_pos_sp;
		BulbToys_GetScreenPosition(nav_left_pos, left_pos_sp);
		BulbToys_GetScreenPosition(nav_right_pos, right_pos_sp);

		// Only valid if the Z coordinates are under 1
		if (left_pos_sp.z < 1.0f && right_pos_sp.z < 1.0f)
		{
			ri->line_center = {
				(nav_left_pos.x + nav_right_pos.x) / 2,
				(nav_left_pos.y + nav_right_pos.y) / 2,
				(nav_left_pos.z + nav_right_pos.z) / 2
			};

			ri->line_min = ImVec2(left_pos_sp.x, left_pos_sp.y);
			ri->line_max = ImVec2(right_pos_sp.x, right_pos_sp.y);

			if (setup && ri->width > setup->minimum_width)
			{
				ri->line_color = ImVec4(0, 1, 0, 1); // green
			}

			ri->line_valid = true;
		}

		if (setup)
		{
			int count = setup->required_vehicles;
			if (count > 0)
			{
				// the next like 100 lines or so are taken straight from AICopManager::CreateRoadblock
				// i have little to no fucking clue what's going on here. godspeed. o7
				Vector3 some_vector, other_vector;

				float left_to_target = UMath_DistanceNoSqrt(&nav_left_pos, &sw_position);
				float right_to_target = UMath_DistanceNoSqrt(&nav_right_pos, &sw_position);
				if (left_to_target <= right_to_target)
				{
					other_vector = nav_left_pos;

					some_vector.x = nav_right_pos.x - nav_left_pos.x;
					some_vector.y = nav_right_pos.y - nav_left_pos.y;
					some_vector.z = nav_right_pos.z - nav_left_pos.z;
				}
				else
				{
					other_vector = nav_right_pos;

					some_vector.x = nav_left_pos.x - nav_right_pos.x;
					some_vector.y = nav_left_pos.y - nav_right_pos.y;
					some_vector.z = nav_left_pos.z - nav_right_pos.z;
				}

				float width_thing = setup->minimum_width / ri->width * 0.5f;

				Vector3 vecB;
				vecB.x = some_vector.x * width_thing + other_vector.x;
				vecB.y = some_vector.y * width_thing + other_vector.y;
				vecB.z = some_vector.z * width_thing + other_vector.z;

				// ?????????????????????????????????????
				float what = sw_position.x;
				float clamp_thing = reinterpret_cast<float(*)(float, float, float)>(0x4010E0)(what, 1.0f, 1.14f);

				Matrix4 rotation;
				Util_GenerateMatrix(&rotation, &nav_fwd_vec, 0);

				for (int i = 0; i < 6; i++)
				{
					rbelem_t type = setup->contents[i].type;

					if (type == rbelem_t::none)
					{
						// Note we have a break here and not a continue - the roadblock ends as soon as it finds the first "rbelem_t::none" element!!
						break;
					}

					Vector3 vecA;
					vecA.x = clamp_thing * setup->contents[i].offset_x;
					vecA.y = 0.0;
					vecA.z = setup->contents[i].offset_z;
					UMath_Rotate(&vecA, &rotation, &vecA);

					Vector3 forward;
					forward = nav_fwd_vec;
					VU0_v3add(setup->contents[i].angle, &forward, &forward);

					Vector3 position;
					position.x = vecA.x + vecB.x;
					position.y = vecA.y + vecB.y;
					position.z = vecA.z + vecB.z;

					Vector3 dimension;

					if (type == rbelem_t::car)
					{
						// Using copmidsize dimensions here as it is the largest by width and length
						dimension = { 1.09f, 0.72f, 2.80f };
						ri->object[i].color = ImVec4(.25f, .25f, 1, 1); // light blue
					}
					else if (type == rbelem_t::barrier)
					{
						dimension = { 1.58f, 0.70f, 0.63f };
						ri->object[i].color = ImVec4(1, 0, 0, 1); // red
					}
					else if (type == rbelem_t::spikestrip)
					{
						dimension = { 3.27f, 0.25f, 0.38f };
						ri->object[i].color = ImVec4(1, 1, 0, 1); // yellow
					}

					position.y += dimension.y;

					ri->object[i].position = position;
					ri->object[i].dimension = dimension;
					ri->object[i].fwd_vec = forward;
					ri->object[i].valid = true;
				}
			}
		}

		return ri;
	}

	// Street width calculation unsuccessful
	return nullptr;
}

bool nfsc::BulbToys_SwitchVehicle(uintptr_t simable, uintptr_t simable2, sv_mode mode)
{
	if (!simable || !simable2)
	{
		return false;
	}

	uintptr_t player = PhysicsObject_GetPlayer(simable);
	if (!player)
	{
		return false;
	}

	uintptr_t vehicle = BulbToys_FindInterface<IVehicle>(simable);
	uintptr_t vehicle2 = BulbToys_FindInterface<IVehicle>(simable2);
	if (!vehicle || !vehicle2)
	{
		return false;
	}

	uintptr_t rbody = PhysicsObject_GetRigidBody(simable);
	uintptr_t rbody2 = PhysicsObject_GetRigidBody(simable2);
	if (!rbody || !rbody2)
	{
		return false;
	}

	if (mode == sv_mode::one_way)
	{
		driver_class dc = PVehicle_GetDriverClass(vehicle);
		PVehicle_SetDriverClass(vehicle, driver_class::none);
		PVehicle_ReleaseBehaviorAudio(vehicle);
		PVehicle_Deactivate(vehicle);

		Vector3 fwd_vec = { 1, 0, 0 };
		Vector3* pos = RigidBody_GetPosition(rbody);
		RigidBody_GetForwardVector(rbody, &fwd_vec);
		PVehicle_SetVehicleOnGround(vehicle2, pos, &fwd_vec);

		float speed = PVehicle_GetSpeed(vehicle);
		PVehicle_SetSpeed(vehicle2, speed);

		PhysicsObject_Attach(simable2, player);
		PVehicle_GlareOn(vehicle2, 0x7); // headlights
		PVehicle_Activate(vehicle2);

		uint8_t force_stop = PVehicle_GetForceStop(vehicle);

		// todo: useless? (supposed to prevent race start unfreeze? but doesn't)
		if ((force_stop & 0x10) != 0)
		{
			PVehicle_ForceStopOn(vehicle, 0x10);
		}
		else
		{
			PVehicle_ForceStopOff(vehicle, 0x10);
		}

		if (BulbToys_GetRaceType() != race_type::none)
		{
			uintptr_t racer_info = GRaceStatus_GetRacerInfo2(ReadMemory<uintptr_t>(GRaceStatus), simable);
			GRacerInfo_SetSimable(racer_info, simable2);
		}

		/*
		int c = 0;

		// Retarget all necessary AITargets to our new vehicle
		for (int i = 0; i < AITargetsList->size; i++)
		{
			uintptr_t ai_target = AITargetsList->begin[i];

			uintptr_t ai_target_vehicle = nullptr;
			AITarget_GetVehicleInterface(ai_target, &ai_target_vehicle);
			if (vehicle == ai_target_vehicle)
			{
				AITarget_Acquire(ai_target, simable2);
				c++;
			}
		}

		Error("Retargeted %d/%d", c, AITargetsList->size);
		*/

		// todo: fix pursuits here
		// todo: fix camera
		// todo: eloadingscreenoff/on

		PhysicsObject_Kill(simable);
	}

	EAXSound_StartNewGamePlay(ReadMemory<uintptr_t>(0xA8BA38));

	// Call ResetHUDType virtually (ie. our AIPlayer class uses OnlineRemotePlayer::ResetHUDType)
	reinterpret_cast<decltype(LocalPlayer_ResetHUDType)>(VirtualFunction(player, 9))(player, 1);

	return true;
}

void __fastcall nfsc::BulbToys_SwitchPTagTarget(uintptr_t race_status, bool busted)
{
	// Store information about our current runner here
	int runner_index = -1;
	uintptr_t runner_simable = GRaceStatus_GetRacePursuitTarget(race_status, &runner_index);

	// Store information about our soon-to-be runner here
	int min_index = -1;
	float min = FLT_MAX;

	// Find the cop car closest to the current runner's car (minimum distance between the racer and each of the cop cars)
	// TODO: use pursuit contribution instead?
	for (int i = 0; i < GRaceStatus_GetRacerCount(race_status); i++)
	{
		if (i == runner_index)
		{
			continue;
		}

		uintptr_t racer_info = GRaceStatus_GetRacerInfo(race_status, i);
		uintptr_t simable = GRacerInfo_GetSimable(racer_info);

		float distance = BulbToys_GetDistanceBetween(runner_simable, simable);
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
	Game_TagPursuit(runner_index, min_index, true);
	Game_TagPursuit(runner_index, min_index, true);

	// Call Game_TagPursuit as intended
	Game_TagPursuit(runner_index, min_index, busted);

	// FIXME: AI TARGETING LOGIC GOES HERE
}

void nfsc::BulbToys_UpdateWorldMapCursor(uintptr_t fe_state_manager)
{
	// Revert snap radius
	WriteMemory<uintptr_t>(0x5C344D, 0x9D10A4);

	FEColor color = { 255, 255, 255, 255 }; // white/none
	auto world_map = ReadMemory<uintptr_t>(WorldMap);

	if (world_map)
	{
		if (g::world_map::shift_held)
		{
			// Only change the color for the "normal" mode of the world map (not quick list, etc...)
			if (ReadMemory<world_map_state>(fe_state_manager + 4) == world_map_state::normal)
			{
				// (Un)Patch non-conforming !!!
				constexpr float zero = .0f;
				WriteMemory<const float*>(0x5C344D, &zero);

				color = { 0, 0, 255, 255 }; // red

				// Get the current position of the cursor relative to the screen
				float x, y;
				FE_Object_GetCenter(ReadMemory<uintptr_t>(world_map + 0x28), &x, &y);

				// Account for WorldMap pan
				Vector2 temp;
				temp.x = x;
				temp.y = y;

				WorldMap_GetPanFromMapCoordLocation(world_map, &temp, &temp);

				x = temp.x;
				y = temp.y;

				// Account for WorldMap zoom
				Vector2 top_left = ReadMemory<Vector2>(world_map + 0x44);
				Vector2 size = ReadMemory<Vector2>(world_map + 0x4C);

				x = x * size.x + top_left.x;
				y = y * size.y + top_left.y;

				// it just works tm
				auto track_info = reinterpret_cast<uintptr_t(*)(int)>(0x7990C0)(5000);

				// Inverse WorldMap::ConvertPos to get world coordinates
				float calibration_width = ReadMemory<float>(track_info + 0xB4);
				float calibration_offset_x = ReadMemory<float>(track_info + 0xAC);
				float calibration_offset_y = ReadMemory<float>(track_info + 0xB0);

				x = x - top_left.x;
				y = y - top_left.y;
				x = x / size.x;
				y = y / size.y;
				y = y - 1.0f;
				y = y * calibration_width;
				x = x * calibration_width;
				x = x + calibration_offset_x;
				y = -y;
				y = y - calibration_offset_y - calibration_width;

				// Inverse GetVehicleVectors to get position from world coordinates
				g::world_map::location.x = -y;
				g::world_map::location.y = 0; // z
				g::world_map::location.z = x;

				// Attempt to get world height at given position
				WCollisionMgr mgr;
				mgr.fSurfaceExclusionMask = 0;
				mgr.fPrimitiveMask = 3;

				float height = NAN;
				if (WCollisionMgr_GetWorldHeightAtPointRigorous(mgr, &g::world_map::location, &height, nullptr))
				{
					// Successful
					color = { 0, 255, 0, 255 }; // green
				}
				g::world_map::location.y = height;
			}
		}

		// WorldMap->mEventIconGlow
		FE_Object_SetColor(ReadMemory<uintptr_t>(world_map + 0x28), &color);
	}
}

/* ===== AIPLAYER ===== */

// Most of this shit is probably useless garbage the compiler spit out due to inheritance but i'm replicating it for consistency
nfsc::AIPlayer* nfsc::AIPlayer::New()
{
	// uintptr_t FastMem::Alloc(&FastMem, size, 0);
	auto malloc = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint32_t, const char*)>(0x60BA70)(FastMem, sizeof(AIPlayer), 0);
	if (!malloc)
	{
		return nullptr;
	}

	// Sim::Entity::Entity(this);
	auto entity = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(0x76C5A0)(malloc);

	auto ai_player = reinterpret_cast<AIPlayer*>(entity);

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

	EntityList->Add(reinterpret_cast<uintptr_t>(&ai_player->Sim_Entity.Sim_IEntity), 0x6C9900);
	IPlayerList->Add(reinterpret_cast<uintptr_t>(&ai_player->IPlayer), 0x6C9890);

	return ai_player;
}

nfsc::AIPlayer* nfsc::AIPlayer::VecDelDtor(AIPlayer* ai_player, int edx, uint8_t flags)
{
	Destructor(ai_player);
	if (ai_player && (flags & 1) != 0)
	{
		// FastMem::Free(&FastMem, this, size, 0);
		reinterpret_cast<void(__thiscall*)(uintptr_t, void*, uint32_t, const char*)>(0x609E80)(FastMem, ai_player, sizeof(AIPlayer), 0);
	}
	return ai_player;
}

uintptr_t nfsc::AIPlayer::GetSimable_IPlayer(AIPlayer* ai_player)
{
	VTable<10>* vtable = reinterpret_cast<VTable<10>*>(ai_player->Sim_Entity.Sim_IEntity.UCOM_IUnknown.vtbl);

	// Sim::Entity::GetSimable(Sim::IEntity *this)
	return reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(vtable->f[3])(reinterpret_cast<uintptr_t>(ai_player) - 0x20);
}