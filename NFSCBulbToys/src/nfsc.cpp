#include "shared.h"

uint32_t NFSC::ListableSet_GetGrowSizeVirtually(void* ls, uint32_t amount)
{
	return reinterpret_cast<uint32_t(__thiscall*)(void*, uint32_t)>(Virtual<3>(reinterpret_cast<uintptr_t>(ls)))(ls, amount);
}

void NFSC::RoadblockSetup::operator=(const RoadblockSetupFile& rbsf)
{
	memcpy(this, (char*)&rbsf + 4, sizeof(NFSC::RoadblockSetup));
}

void NFSC::RoadblockSetupFile::operator=(const RoadblockSetup& rbs)
{
	memcpy((char*)this + 4, &rbs, sizeof(NFSC::RoadblockSetup));
}

bool NFSC::RoadblockSetupFile::Validate()
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

inline int NFSC::BulbToys_GetGameFlowState()
{
	return Read<int>(0xA99BBC);
}

void NFSC::BulbToys_DrawObject(ImDrawList* draw_list, Vector3& position, Vector3& dimension, Vector3& fwd_vec, ImVec4& color, float thickness)
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

void NFSC::BulbToys_DrawVehicleInfo(ImDrawList* draw_list, uintptr_t vehicle, int vltype, ImVec4& color)
{
	// Distance at which the text stops rendering
	constexpr float max_distance = 100.f;

	// Vehicle can be either a pointer or the IVehicleList array index. If vehicle is a really small number, assume it's an array index
	// For array indexes, just grab the vehicle at that index. Otherwise, iterate the list until the iterator pointer matches our pointer
	uint32_t id = 0;
	if (vehicle < VehicleList[vltype]->size)
	{
		id = vehicle;
		vehicle = VehicleList[vltype]->begin[id];
	}
	else
	{
		for (id = 0; id < VehicleList[vltype]->size; id++)
		{
			if (VehicleList[vltype]->begin[id] == vehicle)
			{
				break;
			}
		}

		if (id == VehicleList[vltype]->size)
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

uintptr_t NFSC::BulbToys_GetAIVehicleGoal(uintptr_t ai_vehicle)
{
	return Read<uintptr_t>(ai_vehicle + 0x94);
}

const char* NFSC::BulbToys_GetCameraName()
{
	if (NFSC::BulbToys_GetGameFlowState() != GFS::RACING)
	{
		return "";
	}

	uintptr_t first_camera_director = Read<uintptr_t>(0xA8ACC4 + 4);
	uintptr_t camera_director = Read<uintptr_t>(first_camera_director);
	uintptr_t cd_action = Read<uintptr_t>(camera_director + 0x18);

	// CameraAI::Action::GetName(this)->mString;
	return Read<char*>(reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(Virtual<3>(cd_action))(cd_action) + 4);
}

// NOTE: Returns tier 0 correctly (ie. Dump Truck). Returns -1 if it can't find its attributes.
int NFSC::BulbToys_GetVehicleTier(uintptr_t vehicle)
{
	constexpr ptrdiff_t offset = -0xD0 /* PVehicle */ + 0xF0;
	uintptr_t attributes = vehicle + offset;

	uintptr_t layout_ptr = Read<uintptr_t>(attributes + 4);
	if (!layout_ptr)
	{
		return -1;
	}

	return Read<int>(layout_ptr + 0x2C);
}

bool NFSC::BulbToys_IsPlayerLocal(uintptr_t player)
{
	// RecordablePlayer::`vftable'{for `IPlayer'}
	return Read<uintptr_t>(player) == 0x9EC8C0;
}

void NFSC::BulbToys_GetScreenPosition(Vector3& world, Vector3& screen)
{
	Vector4 out, input = { world.z, -world.x, world.y, 1.0 };

	Matrix4 proj = Read<Matrix4>((0xB1A780 + 1 * 0x1A0) + 0x80);

	// D3DXVec4Transform
	reinterpret_cast<Vector4*(__stdcall*)(Vector4*, Vector4*, Matrix4*)>(0x86B29C)(&out, &input, &proj);

	float i_w = 1.0f / out.w;
	out.x *= i_w;
	out.y *= i_w;

	screen.x = (out.x + 1.0f) * Read<int>(0xAB0AC8) * 0.5f;
	screen.y = (out.y - 1.0f) * Read<int>(0xAB0ACC) * -0.5f;
	screen.z = i_w * out.z;
}

float NFSC::BulbToys_GetStreetWidth(Vector3* position, Vector3* direction, float distance, Vector3* left_pos, Vector3* right_pos, Vector3* fwd_vec)
{
	WRoadNav nav;
	WRoadNav_WRoadNav(nav);

	// WRoadNav::SetCookieTrail(&nav, 1);
	reinterpret_cast<void(__thiscall*)(WRoadNav&, bool)>(0x7F7CA0)(nav, 1);

	// WRoadNav::SetPathType(&nav, kPathCop);
	reinterpret_cast<void(__thiscall*)(WRoadNav&, int)>(0x7EC0D0)(nav, 0);

	// nav.fNavType = kTypeDirection;
	nav.fNavType = 2;

	WRoadNav_InitAtPoint(nav, position, direction, 0, 0.0);
	if (!nav.fValid)
	{
		WRoadNav_Destructor(nav);
		return NAN;
	}

	WRoadNav_IncNavPosition(nav, distance, ZeroV3, 0.0, 0);

	// segment = &WRoadNetwork::fSegments[nav.fSegmentInd];
	uintptr_t segment = Read<uintptr_t>(0xB77ECC) + 0x16 * nav.fSegmentInd;
	if (!segment)
	{
		WRoadNav_Destructor(nav);
		return NAN;
	}

	uint16_t segment_flags = Read<uint16_t>(segment + 0xA);

	// (segment->fFlags & 2) != 0
	if ((segment_flags & 2) != 0)
	{
		WRoadNav_Destructor(nav);
		return NAN;
	}

	// (segment->fFlags & 1) != 0
	if ((segment_flags & 1) != 0)
	{
		WRoadNav_Destructor(nav);
		return NAN;
	}

	// segment->nLength * flt_00A83DA8 < flt_009C1D30 )
	if (Read<uint16_t>(segment + 0x4) * 0.015259022 < 40.0)
	{
		WRoadNav_Destructor(nav);
		return NAN;
	}

	// segment_profile = WRoadNetwork::GetSegmentProfile(WRoadNetwork::fgRoadNetwork, segment, nav.fNodeInd);
	uintptr_t segment_profile = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uintptr_t, int)>(0x7EB290)(Read<uintptr_t>(0xB77EC0), segment, nav.fNodeInd);

	// !segment_profile->fNumZones
	if (!segment_profile || !Read<char>(segment_profile))
	{
		WRoadNav_Destructor(nav);
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

	WRoadNav_Destructor(nav);

	return width;
}

int NFSC::BulbToys_GetRaceType()
{
	uintptr_t g_race_status = Read<uintptr_t>(GRaceStatus);
	if (!g_race_status)
	{
		return GRaceType::NONE;
	}

	uintptr_t race_parameters = Read<uintptr_t>(g_race_status + 0x6A1C);
	if (!race_parameters)
	{
		return GRaceType::NONE;
	}

	return reinterpret_cast<int(__thiscall*)(uintptr_t)>(0x6136A0)(race_parameters);
}

int NFSC::BulbToys_GetRacerIndex(uintptr_t racer_info)
{
	uintptr_t g_race_status = Read<uintptr_t>(NFSC::GRaceStatus);
	if (!g_race_status)
	{
		return -1;
	}

	constexpr int max_offset = 0x384 * 29;
	int offset = racer_info - (g_race_status + 0x18);
	if (offset > max_offset)
	{
		return -1;
	}
	return offset / 0x384;
}

// Returns false if we're not in Debug Cam
bool NFSC::BulbToys_GetDebugCamCoords(Vector3* position , Vector3* fwd_vec)
{
	if (NFSC::BulbToys_GetGameFlowState() != GFS::RACING)
	{
		return false;
	}

	uintptr_t first_camera_director = Read<uintptr_t>(0xA8ACC4 + 4);
	uintptr_t camera_director = Read<uintptr_t>(first_camera_director);
	uintptr_t cd_action = Read<uintptr_t>(camera_director + 0x18);

	// Check if we're in CDActionDebug (CDActionDebug::`vftable'{for `CameraAI::Action'})
	if (Read<uintptr_t>(cd_action) != 0x9C7EE0)
	{
		return false;
	}

	uintptr_t camera_mover = Read<uintptr_t>(cd_action + 0x2BC);
	uintptr_t camera = Read<uintptr_t>(camera_mover + 0x1C);

	if (position)
	{
		// z, -x, y -> x, y, z
		position->x = -Read<float>(camera + 0x44);
		position->y = Read<float>(camera + 0x48);
		position->z = Read<float>(camera + 0x40);
	}

	if (fwd_vec)
	{
		// z, -x, y -> x, y, z
		fwd_vec->x = -Read<float>(camera + 0x54);
		fwd_vec->y = Read<float>(camera + 0x58);
		fwd_vec->z = Read<float>(camera + 0x50);
	}

	return true;
}

bool NFSC::BulbToys_GetMyVehicle(uintptr_t* my_vehicle, uintptr_t* my_simable)
{
	auto gfs = NFSC::BulbToys_GetGameFlowState();
	if (gfs == GFS::RACING || gfs == GFS::LOADING_REGION || gfs == GFS::LOADING_TRACK)
	{
		for (int i = 0; i < (int)VehicleList[VLType::PLAYERS]->size; i++)
		{
			uintptr_t vehicle = VehicleList[VLType::PLAYERS]->begin[i];

			if (vehicle)
			{
				uintptr_t simable = PVehicle_GetSimable(vehicle);

				if (simable)
				{
					uintptr_t player = PhysicsObject_GetPlayer(simable);

					if (player && BulbToys_IsPlayerLocal(player))
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

void NFSC::BulbToys_PathToTarget(uintptr_t ai_vehicle, Vector3* target)
{
	auto road_nav = Read<uintptr_t>(ai_vehicle + 0x38);
	if (!road_nav)
	{
		return;
	}

	WRoadNav_FindPath(road_nav, target, nullptr, 1);
}

// Does not return RoadblockInfo if street width calculation is unsuccessful (ie. width = NAN)
void* NFSC::BulbToys_RoadblockCalculations(RoadblockSetup* setup, uintptr_t rigid_body)
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
	float distance = Read<float>(0x44529C);

	Vector3 nav_left_pos, nav_right_pos, nav_fwd_vec;
	float width = BulbToys_GetStreetWidth(&sw_position, &sw_fwd_vec, distance, &nav_left_pos, &nav_right_pos, &nav_fwd_vec) + 1.0f;
	
	// Street width calculation successful
	if (!isnan(width))
	{
		// Create rendering/spawning roadblock info adapter
		GUI::RoadblockInfo* ri = new GUI::RoadblockInfo();

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
					RoadblockElement::Type type = setup->contents[i].type;

					if (type == RoadblockElement::Type::NONE)
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

					if (type == RoadblockElement::Type::CAR)
					{
						// Using copmidsize dimensions here as it is the largest by width and length
						dimension = { 1.09f, 0.72f, 2.80f };
						ri->object[i].color = ImVec4(.25f, .25f, 1, 1); // light blue
					}
					else if (type == RoadblockElement::Type::BARRIER)
					{
						dimension = { 1.58f, 0.70f, 0.63f };
						ri->object[i].color = ImVec4(1, 0, 0, 1); // red
					}
					else if (type == RoadblockElement::Type::SPIKESTRIP)
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

bool NFSC::BulbToys_SwitchVehicle(uintptr_t simable, uintptr_t simable2, bool kill_old)
{
	if (!simable || !simable2)
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

	// Copy driver class. Deactivate
	int dc = PVehicle_GetDriverClass(vehicle);
	PVehicle_SetDriverClass(vehicle, DriverClass::NONE);
	PVehicle_ReleaseBehaviorAudio(vehicle);
	PVehicle_Deactivate(vehicle);

	// Paste driver class
	PVehicle_SetDriverClass(vehicle2, dc);

	// Copy and paste position/rotation
	Vector3 fwd_vec = { 1, 0, 0 };
	Vector3* pos = RigidBody_GetPosition(rbody);
	RigidBody_GetForwardVector(rbody, &fwd_vec);
	PVehicle_SetVehicleOnGround(vehicle2, pos, &fwd_vec);

	// Copy and paste speed
	float speed = PVehicle_GetSpeed(vehicle);
	PVehicle_SetSpeed(vehicle2, speed);

	// Copy and paste IPlayer. Activate
	uintptr_t player = PhysicsObject_GetPlayer(simable);
	if (player)
	{
		PhysicsObject_Attach(simable2, player);
	}
	PVehicle_GlareOn(vehicle2, 0x7); // headlights
	PVehicle_Activate(vehicle2);

	// todo: useless? (supposed to prevent race start unfreeze? but doesn't)
	uint8_t force_stop = PVehicle_GetForceStop(vehicle);
	if ((force_stop & 0x10) != 0)
	{
		PVehicle_ForceStopOn(vehicle, 0x10);
	}
	else
	{
		PVehicle_ForceStopOff(vehicle, 0x10);
	}

	// Copy and paste RacerInfo simable
	if (BulbToys_GetRaceType() != GRaceType::NONE)
	{
		uintptr_t racer_info = GRaceStatus_GetRacerInfo2(Read<uintptr_t>(GRaceStatus), simable);
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
	// todo: eloadingscreenoff/on?

	if (kill_old)
	{
		PhysicsObject_Kill(simable);
	}

	// Restart the sound, regardless if we were involved with the switch or not
	BulbToys_RestartSound();

	// Fix car camera and reset HUD
	if (player && BulbToys_IsPlayerLocal(player))
	{
		ChangeLocalPlayerCameraInfo();
		LocalPlayer_ResetHUDType(player, 1);
	}

	return true;
}

void __fastcall NFSC::BulbToys_SwitchPTagTarget(uintptr_t race_status, bool busted)
{
	// Store information about our current runner here. Simable needs to be static in order for the sort function to work.
	int runner_index = -1;
	static uintptr_t runner_simable = 0;
	runner_simable = GRaceStatus_GetRacePursuitTarget(race_status, &runner_index);

	// Create array of RacerInfos for chasers (racer count minus the one runner)
	size_t len = GRaceStatus_GetRacerCount(race_status) - 1;
	uintptr_t* chasers = new uintptr_t[len];

	// Fill the chaser array, skipping the runner of course
	int racer_index = 0;
	for (int i = 0; i < len; i++, racer_index++)
	{
		if (racer_index == runner_index)
		{
			i--;
			continue;
		}
		chasers[i] = GRaceStatus_GetRacerInfo(race_status, racer_index);
	}

	// First sort by pursuit contribution
	qsort(chasers, len, sizeof(uintptr_t), +[](const void* a, const void* b)
	{
		float diff = Read<float>(*(uintptr_t*)a + 0x1A4) - Read<float>(*(uintptr_t*)b + 0x1A4);

		if (diff < .0f)
		{
			return 1;
		}
		else if (diff > .0f)
		{
			return -1;
		}
		return 0;
	});

	// If our highest contributor didn't do anything, assume none of the other chasers did anything either, and we sort by distance to runner instead
	// This check is NOT done in vanilla, but target switching will be biased towards the lowest index racer if the array remains unsorted
	if (Read<float>(chasers[0] + 0x1A4) == .0f)
	{
		qsort(chasers, len, sizeof(uintptr_t), +[](const void* a, const void* b)
		{
			uintptr_t simable_a = GRacerInfo_GetSimable(*(uintptr_t*)a);
			uintptr_t simable_b = GRacerInfo_GetSimable(*(uintptr_t*)b);

			float diff = BulbToys_GetDistanceBetween(runner_simable, simable_a) - BulbToys_GetDistanceBetween(runner_simable, simable_b);
			if (diff < .0f)
			{
				return -1;
			}
			else if (diff > .0f)
			{
				return 1;
			}
			return 0;
		});
	}

	/*
	for (int i = 0; i < len; i++)
	{
		uintptr_t sim = GRacerInfo_GetSimable(chasers[i]);
		LOG(3, "%d. %s (%.2f, %.2f)", i + 1, reinterpret_cast<char*>(chasers[i] + 0x8), Read<float>(chasers[i] + 0x1A4), BulbToys_GetDistanceBetween(runner_simable, sim));
	}
	*/

	// New runner is the one with the best score
	int new_runner = NFSC::BulbToys_GetRacerIndex(chasers[0]);

	// TODO: just fucking replace Game_SwitchPursuit entirely lmfao

	// FIXME: Tagging is fucked. The first time the vehicle switch is performed, the game "softlocks"
	// Likely because a vehicle is deactivated when it shouldn't be, or (more likely) a player/entity does not have a simable
	// As a workaround, we're calling Game_TagPursuit two extra times here
	// We specifically set busted to true because it doesn't increment the runner's time incorrectly here (false indicates an evasion, which gives the racer a time bonus)
	// However, we do fuck up the "number of busts/number of times busted" stats for these racers, which will need to be unfucked (TODO?)
	Game_TagPursuit(runner_index, new_runner, true);
	Game_TagPursuit(runner_index, new_runner, true);

	// Call Game_TagPursuit as intended
	Game_TagPursuit(runner_index, new_runner, busted);

	// FIXME: AI RETARGETING LOGIC GOES HERE(?)
}

void NFSC::BulbToys_UpdateWorldMapCursor(uintptr_t fe_state_manager)
{
	// Revert snap radius
	Write<uintptr_t>(0x5C344D, 0x9D10A4);

	FEColor color = { 255, 255, 255, 255 }; // white/none
	auto world_map = Read<uintptr_t>(WorldMap);

	if (world_map)
	{
		// WorldMap->mEventIconGlow
		uintptr_t cursor = Read<uintptr_t>(world_map + 0x28);

		if (g::world_map::shift_held)
		{
			// Only change the color for the "normal" mode of the world map (not quick list, etc...)
			if (Read<int>(fe_state_manager + 4) == FESM::WorldMap::NORMAL)
			{
				// (Un)Patch non-conforming !!!
				constexpr float zero = .0f;
				Write<const float*>(0x5C344D, &zero);

				color = { 0, 0, 255, 255 }; // red

				// Get the current position of the cursor relative to the screen
				float x, y;
				FE_Object_GetCenter(cursor, &x, &y);

				// Account for WorldMap pan
				Vector2 temp;
				temp.x = x;
				temp.y = y;

				WorldMap_GetPanFromMapCoordLocation(world_map, &temp, &temp);

				x = temp.x;
				y = temp.y;

				// Account for WorldMap zoom
				Vector2 top_left = Read<Vector2>(world_map + 0x44);
				Vector2 size = Read<Vector2>(world_map + 0x4C);

				x = x * size.x + top_left.x;
				y = y * size.y + top_left.y;

				// TrackInfo::GetTrackInfo(5000) - it just works tm
				auto track_info = reinterpret_cast<uintptr_t(*)(int)>(0x7990C0)(5000);

				// Inverse WorldMap::ConvertPos to get world coordinates
				float calibration_width = Read<float>(track_info + 0xB4);
				float calibration_offset_x = Read<float>(track_info + 0xAC);
				float calibration_offset_y = Read<float>(track_info + 0xB0);

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

		FE_Object_SetColor(cursor, &color);
	}
}

void NFSC::BulbToys_WorldMap_UpdateFLM()
{
	if (g::world_map::flm)
	{
		// dtor
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x7597C0)(g::world_map::flm);
		NFSC::free(g::world_map::flm);
		g::world_map::flm = 0;
	}

	int event_type = -1;
	int event_hash = -1;

	// don't make FLM if we're not in a race or if it's an encounter race
	if (!NFSC::DALManager_GetInt(DALManager, 3021, &event_hash, -1, -1, -1) || (NFSC::DALManager_GetInt(DALManager, 3011, &event_type, -1, -1, -1) && event_type == 0xC))
	{
		return;
	}
	
	// almost everything below this point is a recreation of Minimap::UpdateRaceRoute
	// how does it work? no idea! it came to me in a dream. glhf

	/*
	// TEMP: we need the minimap
	auto minimap = Read<uintptr_t>(0xA97DE4);
	if (!minimap)
	{
		return;
	}
	*/

	float track_map_size_x = 128.0f;//Read<float>(minimap + 0xA0);

	for (int i = 0; i < Read<uint32_t>(NFSC::WRoadNetwork::fNumSegments); i++)
	{
		auto road_segment = Read<NFSC::WRoadSegment*>(NFSC::WRoadNetwork::fSegments) + i;

		if (road_segment && ((road_segment->fFlags & WRoadSegmentFlags::IS_IN_RACE) != 0) && ((road_segment->fFlags & WRoadSegmentFlags::IS_SIDE_ROUTE) == 0))
		{
			bool new_segment = true;

			if ((road_segment->fFlags & WRoadSegmentFlags::IS_DECISION) != 0)
			{
				for (int k = 0; k < 2; k++)
				{
					auto road_node = Read<NFSC::WRoadNode*>(NFSC::WRoadNetwork::fNodes) + road_segment->fNodeIndex[k];

					for (int j = 0; j < road_node->fNumSegments; j++)
					{
						uint16_t segment_index = road_node->fSegmentIndex[j];

						if (segment_index != i)
						{
							auto other_segment = Read<NFSC::WRoadSegment*>(NFSC::WRoadNetwork::fSegments) + segment_index;
							if (((other_segment->fFlags & WRoadSegmentFlags::IS_IN_RACE) != 0) && ((other_segment->fFlags & WRoadSegmentFlags::IS_SIDE_ROUTE) != 0))
							{
								new_segment = false;
								break;
							}
						}
					}
				}
			}

			if (new_segment)
			{
				NFSC::Vector2 points[4] { {0, 0}, {0, 0}, {0, 0}, {0, 0} };
				NFSC::Vector3 controls[2] { {0, 0, 0}, {0, 0, 0} };
				NFSC::WRoadNode* nodes[2] {
					Read<NFSC::WRoadNode*>(NFSC::WRoadNetwork::fNodes) + road_segment->fNodeIndex[0],
					Read<NFSC::WRoadNode*>(NFSC::WRoadNetwork::fNodes) + road_segment->fNodeIndex[1]
				};

				// get start/end control
				reinterpret_cast<void(__thiscall*)(NFSC::WRoadSegment*, NFSC::Vector3*)>(0x404D80)(road_segment, &controls[0]);
				reinterpret_cast<void(__thiscall*)(NFSC::WRoadSegment*, NFSC::Vector3*)>(0x5D1CA0)(road_segment, &controls[1]);

				points[0].y = -nodes[0]->fPosition.x;
				points[0].x = nodes[0]->fPosition.z;
				points[1].y = -(controls[0].x + nodes[0]->fPosition.x);
				points[1].x = controls[0].z + nodes[0]->fPosition.z;
				points[2].y = -(controls[1].x + nodes[1]->fPosition.x);
				points[2].x = controls[1].z + nodes[1]->fPosition.z;
				points[3].y = -nodes[1]->fPosition.x;
				points[3].x = nodes[1]->fPosition.z;

				float scaled[2] { track_map_size_x * 16.0f, track_map_size_x * -0.5f };

				for (int j = 0; j < 4; j++)
				{
					points[j].x = (points[j].x - -7300.0f) * 0.000062500003f;
					points[j].y = (-2000.0f - points[j].y) * 0.000062500003f + 1.0f;

					points[j].x = scaled[0] * points[j].x + scaled[1];
					points[j].y = scaled[0] * points[j].y + scaled[1];

					// account for world map scale
					if (NFSC::BulbToys_IsGameNFSCO())
					{
						points[j].x -= 1247.f;
					}
					else
					{
						points[j].x -= 951.f;
					}
					points[j].y = points[j].y - 1411.f;

					points[j].x *= 1.27f;
					points[j].y *= 1.27f;
				}

				if (!g::world_map::flm)
				{
					// ctor
					g::world_map::flm = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, bool)>(0x759690)(NFSC::malloc(0x160), false);
				}

				/*
				Error("A: { %.2f, %.2f }\nB: { %.2f, %.2f }\nC: { %.2f, %.2f }\nC: { %.2f, %.2f }", 
					points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, points[3].x, points[3].y);
				*/

				// add bezier
				reinterpret_cast<void(__thiscall*)(uintptr_t, NFSC::Vector2*, NFSC::Vector2*, NFSC::Vector2*, NFSC::Vector2*, float)>(0x7598E0)
					(g::world_map::flm, &points[0], &points[1], &points[2], &points[3], 0.25);
			}
		}
	}

	if (g::world_map::flm)
	{
		// generate
		reinterpret_cast<void(__thiscall*)(uintptr_t, float)>(0x75BA60)(g::world_map::flm, 6.25f);

		// optimize
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x74ADC0)(g::world_map::flm);

		// TODO: are the next 5 functions even necessary?
		// center and edge color (0xC0 - 75% opacity)
		reinterpret_cast<void(__thiscall*)(uintptr_t, uint32_t)>(0x740B00)(g::world_map::flm, 0xC02DC2FF);
		reinterpret_cast<void(__thiscall*)(uintptr_t, uint32_t)>(0x740B20)(g::world_map::flm, 0xC0030B0D);

		// alpha scale
		reinterpret_cast<void(__thiscall*)(uintptr_t, float)>(0x740BD0)(g::world_map::flm, 1.0f);

		// width
		reinterpret_cast<void(__thiscall*)(uintptr_t, float)>(0x740A70)(g::world_map::flm, 6.25f);

		// sharpness
		reinterpret_cast<void(__thiscall*)(uintptr_t, float)>(0x740AD0)(g::world_map::flm, 0.0f);

		NFSC::Vector2 min = { -100000, -100000 };
		NFSC::Vector2 max = { +100000, +100000 };

		// mask bbox
		reinterpret_cast<void(__thiscall*)(uintptr_t, NFSC::Vector2*, NFSC::Vector2*)>(0x740940)(g::world_map::flm, &min, &max);

		// tuned fx
		reinterpret_cast<void(__thiscall*)(uintptr_t, const char*)>(0x74B140)(g::world_map::flm, "mini_map_route");
	}
}

/* ===== AIPLAYER ===== */

// Most of this shit is probably useless garbage the compiler spit out due to inheritance but i'm replicating it for consistency
NFSC::AIPlayer* NFSC::AIPlayer::New()
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

NFSC::AIPlayer* NFSC::AIPlayer::VecDelDtor(AIPlayer* ai_player, int edx, uint8_t flags)
{
	Destructor(ai_player);
	if (ai_player && (flags & 1) != 0)
	{
		// FastMem::Free(&FastMem, this, size, 0);
		reinterpret_cast<void(__thiscall*)(uintptr_t, void*, uint32_t, const char*)>(0x609E80)(FastMem, ai_player, sizeof(AIPlayer), 0);
	}
	return ai_player;
}

uintptr_t NFSC::AIPlayer::GetSimable_IPlayer(AIPlayer* ai_player)
{
	VTable<10>* vtable = reinterpret_cast<VTable<10>*>(ai_player->Sim_Entity.Sim_IEntity.UCOM_IUnknown.vtbl);

	// Sim::Entity::GetSimable(Sim::IEntity *this)
	return reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(vtable->f[3])(reinterpret_cast<uintptr_t>(ai_player) - 0x20);
}