#include "shared.h"

MH_STATUS hooks::CreateHook(uintptr_t address, void* hook, void* call)
{
	auto status = MH_CreateHook(reinterpret_cast<LPVOID>(address), hook, reinterpret_cast<void**>(call));

	if (status != MH_OK)
	{
		return status;
	}

	return MH_EnableHook(reinterpret_cast<LPVOID>(address));
}

bool hooks::Setup()
{
	if (MH_Initialize() != MH_OK)
	{
		Error("Unable to initialize minhook.");
		return false;
	}

	// Check if there's a device in case we late-loaded (via injection instead of an ASI loader, for example)
	// Otherwise, assume it wasn't created yet and wait
	auto device = ReadMemory<IDirect3DDevice9*>(0xAB0ABC);
	if (device)
	{
		return SetupPart2(device);
	}
	else
	{
		auto status = CreateHook(0x710220, &DxInitHook, &DxInit);
		if (status != MH_OK)
		{
			Error("Failed to hook DirectX_Init() with MH_STATUS code %d.", status);
			return false;
		}
	}

	return true;
}

bool hooks::SetupPart2(IDirect3DDevice9* device)
{
	auto status = CreateHook(VirtualFunction(device, 42), &EndSceneHook, &EndScene);
	if (status != MH_OK)
	{
		Error("Failed to hook EndScene() with MH_STATUS code %d.", status);
		return false;
	}

	status = CreateHook(VirtualFunction(device, 16), &ResetHook, &Reset);
	if (status != MH_OK)
	{
		Error("Failed to hook Reset() with MH_STATUS code %d.", status);
		return false;
	}

	gui::SetupMenu(device);

	/* Non-critical hooks go here */

	// Optionally override encounter spawn requirement
	if (CreateHook(0x422BF0, &NeedsEncounterHook, &NeedsEncounter) == MH_OK)
	{
		g::needs_encounter::hooked = true;
	}

	// Optionally override traffic spawn requirement
	if (CreateHook(0x422990, &NeedsTrafficHook, &NeedsTraffic) == MH_OK)
	{
		g::needs_traffic::hooked = true;
	}

	// Smart AI hooks
	// - Make autopilot drive to the location marked by the GPS
	// - Make autopilot drive to the location after a navigation reset
	if (CreateHook(0x433930, &GpsEngageHook, &GpsEngage) == MH_OK &&
		CreateHook(0x427AD0, &ResetDriveToNavHook, &ResetDriveToNav) == MH_OK)
	{
		g::smart_ai::hooked = true;
	}

	// Calculate world positions from map positions
	CreateHook(0x5B3850, &WorldMapPadAcceptHook, &WorldMapPadAccept);

	// Create player (Sim::Entity, IPlayer) instances for AI if we're playing PTag/PKO, necessary for vehicle switching to work
	CreateHook(0x6298C0, &RacerInfoCreateVehicleHook, &RacerInfoCreateVehicle);

	// The original function checks GKnockoutRacer::mPursuitTier, which is always 0 and gives us a destroyable copmidsize every time
	// Use our own hook to assign pursuit vehicles based on the local player (us) vehicle's car's tier, exactly how the game intended in the code
	CreateHook(0x616EF0, &GetPursuitVehicleNameHook, &GetPursuitVehicleName);

	// For PTag, check if the racer's 60 second timer has elapsed and switch vehicles accordingly
	CreateHook(0x646B00, &RaceStatusUpdateHook, &RaceStatusUpdate);

	// For testing purposes
	//CreateHook(0x65D620, &PursuitSwitchHook, &PursuitSwitch);

	// Make sure it takes the Quick Race timelimit (race lap) setting into consideration, instead of that particular race's attributes (always 2 minutes iirc)
	CreateHook(0x63E6C0, &GetTimeLimitHook, &GetTimeLimit);

	// Prevent double race end screen softlock
	CreateHook(0x65E240, &ShowLosingScreenHook, &ShowLosingScreen);
	CreateHook(0x65E270, &ShowWinningScreenHook, &ShowWinningScreen);

	// Instead of resuming career, reload the Career menu if we load a save (or if we create a new one (patches::MemcardManagement))
	CreateHook(0x5BD860, &CareerManagerChildFlowDoneHook, &CareerManagerChildFlowDone);

	CreateHook(0x5CF890, &WorldMapShowDialogHook, &WorldMapShowDialog);
	CreateHook(0x5B3570, &WorldMapButtonPressedHook, &WorldMapButtonPressed);

	// Increment cop counter by 1 per roadblock vehicle
	// TODO: if re-enabling this, make sure roadblock cops that get attached don't increment again
	//WriteJmp(0x445A9D, CreateRoadBlockHook, 6);

	// Fix dead roadblock vehicles not turning off their lights
	PatchJmp(0x5D8C10, UpdateCopElementsHook1, 6);
	PatchJmp(0x5D8CFB, UpdateCopElementsHook2, 11);

	// Attach roadblock cops to the pursuit once it's been dodged
	// TODO: ideally detach from roadblock and destroy said roadblock afterwards because of several bugs
	// - roadblock crash cam nag
	// - no new roadblocks will spawn until the cops have been killed
	// - when the roadblock gets destroyed (ie. last cop dies), all cops and their corpses instantly despawn
	//WriteJmp(0x4410D4, UpdateRoadBlocksHook, 6);
	//WriteJmp(0x4411AD, UpdateRoadBlocksHook, 6);

	// Add ability to increase vinyl move step size to move vinyls faster
	PatchJmp(0x7B0F63, MoveVinylVerticalHook, 9);
	PatchJmp(0x7B0F94, MoveVinylHorizontalHook, 9);

	// Use "GRaceStatus" as vehicle cache for NFSCO compatibility
	PatchJmp(0x7D4EBB, VehicleChangeCacheHook, 7);

	// Add AI Players to Player list
	PatchJmp(0x6D40A6, UpdateAIPlayerListingHook, 5);

	// Check who got busted in pursuit tag and switch vehicles accordingly
	PatchJmp(0x44A596, PTagBustedHook, 5);
	
	return true;
}

void hooks::Destroy()
{
	// UpdateCopElements hooks
	Unpatch(0x5D8C10);
	Unpatch(0x5D8CFB);

	// MoveVinyl hooks
	Unpatch(0x7B0F63);
	Unpatch(0x7B0F94);

	// VehicleChangeCacheHook
	Unpatch(0x7D4EBB);

	// UpdateAIPlayerListingHook
	Unpatch(0x6D40A6);

	// PTagBustedHook
	Unpatch(0x44A596);

	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

HRESULT __cdecl hooks::DxInitHook()
{
	const auto result = DxInit();
	if (result < 0)
	{
		return result;
	}

	MH_DisableHook(reinterpret_cast<LPVOID>(0x710220));
	MH_RemoveHook(reinterpret_cast<LPVOID>(0x710220));

	// The initial setup has completed and thus the thread is operational
	// Force it to exit in case our final setup fails
	exitMainLoop = !SetupPart2(ReadMemory<IDirect3DDevice9*>(0xAB0ABC));

	return result;
}

long __stdcall hooks::EndSceneHook(IDirect3DDevice9* device)
{
	const auto result = EndScene(device);

	//static uint32_t frame_count[2] {0, 0};

	//frame_count[0] = frame_count[1];
	//frame_count[1] = ReadMemory<uint32_t>(0xA996F0); // RealLoopCounter

	// Certain loading screens render the game twice, in which case we shouldn't render the GUI
	// TODO: uncomment when i crash here ig lol (window will be null)
	if (gui::menu_open/* && frame_count[0] != frame_count[1]*/)
	{
		gui::Render();
	}

	return result;
}

HRESULT __stdcall hooks::ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = Reset(device, params);
	ImGui_ImplDX9_CreateDeviceObjects();
	return result;
}

bool __fastcall hooks::NeedsEncounterHook(void* traffic_manager)
{
	// If we've overridden the value, use our own. Instead, let the game decide normally
	return g::needs_encounter::overridden ? g::needs_encounter::value : NeedsEncounter(traffic_manager);
}

bool __fastcall hooks::NeedsTrafficHook(void* traffic_manager)
{
	// Ditto
	return g::needs_traffic::overridden ? g::needs_traffic::value : NeedsTraffic(traffic_manager);
}

bool __fastcall hooks::GpsEngageHook(void* gps, void* edx, nfsc::Vector3* target, float max_deviation, bool re_engage, bool always_re_establish)
{
	bool result = GpsEngage(gps, edx, target, max_deviation, re_engage, always_re_establish);

	// GPS failed to establish
	if (!result)
	{
		return result;
	}

	auto p_vehicle = *nfsc::IVehicleList->begin;
	if (!p_vehicle)
	{
		return result;
	}

	auto ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(p_vehicle);
	if (!ai_vehicle)
	{
		return result;
	}

	g::smart_ai::target.x = target->x;
	g::smart_ai::target.y = target->y;
	g::smart_ai::target.z = target->z;
	nfsc::BulbToys_PathToTarget(ai_vehicle, &g::smart_ai::target);

	return result;
}

void __fastcall hooks::ResetDriveToNavHook(void* ai_vehicle, void* edx, int lane_selection)
{
	ResetDriveToNav(ai_vehicle, edx, lane_selection);

	if (nfsc::BulbToys_IsGPSDown())
	{
		return;
	}

	auto p_vehicle = *nfsc::IVehicleList->begin;
	if (!p_vehicle)
	{
		return;
	}

	auto local_ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(p_vehicle);
	if (!local_ai_vehicle)
	{
		return;
	}

	// Only do it for our vehicle
	if (local_ai_vehicle != ai_vehicle)
	{
		return;
	}

	nfsc::BulbToys_PathToTarget(ai_vehicle, &g::smart_ai::target);
}

/*
void* __fastcall hooks::RacerInfoCreateVehicleHook(uintptr_t racer_info, void* edx, uint32_t key, int racer_index, uint32_t seed)
{
	nfsc::race_type type = nfsc::BulbToys_GetRaceType();

	if (type == nfsc::race_type::pursuit_ko)
	{
		void* stored_vehicle = RacerInfoCreateVehicle(racer_info, edx, nfsc::GKnockoutRacer_GetPursuitVehicleKey(0), racer_index, seed);
		Error("%p", stored_vehicle);
		nfsc::PVehicle_SetDriverClass(stored_vehicle, nfsc::driver_class::none);
		nfsc::PVehicle_Deactivate(stored_vehicle);
		WriteMemory<void*>(nfsc::ThePursuitSimables + 4 * racer_index, nfsc::PVehicle_GetSimable(stored_vehicle));

		return RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
	}
	else if (type == nfsc::race_type::pursuit_tag)
	{
		if (racer_index == 1)
		{
			void* player_pursuit_simable = nfsc::BulbToys_CreatePursuitSimable(nfsc::driver_class::none);
			WriteMemory<void*>(nfsc::ThePursuitSimables, player_pursuit_simable);
			nfsc::PVehicle_Deactivate(nfsc::BulbToys_FindInterface<nfsc::IVehicle>(player_pursuit_simable));
		}

		void* stored_vehicle = RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
		nfsc::PVehicle_SetDriverClass(stored_vehicle, nfsc::driver_class::none);
		nfsc::PVehicle_Deactivate(stored_vehicle);
		WriteMemory<void*>(nfsc::ThePursuitSimables + 4 * racer_index, nfsc::PVehicle_GetSimable(stored_vehicle));

		return RacerInfoCreateVehicle(racer_info, edx, nfsc::GKnockoutRacer_GetPursuitVehicleKey(0), racer_index, seed);
	}

	return RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
}
*/

void* __fastcall hooks::RacerInfoCreateVehicleHook(uintptr_t racer_info, void* edx, uint32_t key, int racer_index, uint32_t seed)
{
	void* vehicle = nullptr;

	nfsc::race_type type = nfsc::BulbToys_GetRaceType();

	// Only create players/entities (using our in-house AIPlayer class) for racers if we're playing PKO/PTAG, since they're not really needed elsewhere
	// NFSC uses player/entity related interfaces for attaching vehicles to (and detaching vehicles from) them
	// Since AI does not implement any player/entity related interfaces, we must make our own instead
	if (type == nfsc::race_type::pursuit_ko || type == nfsc::race_type::pursuit_tag)
	{
		// For PTAG, AI racers start as cops, so create our pursuit simables first (so the vehicle start grid warping works properly)
		if (type == nfsc::race_type::pursuit_tag)
		{
			// Online gamemodes skip the intro NIS, but the main reason is because there is no suitable NIS ever made for PTAG's custom start grid positions
			*nfsc::SkipNIS = true;

			// Create our pursuit simables first (so the vehicle start grid warping works properly)
			if (racer_index == 1)
			{
				// Create AI pursuit simables first
				for (int i = 1; i < nfsc::GRaceStatus_GetRacerCount(ReadMemory<void*>(nfsc::GRaceStatus)); i++)
				{
					void* pursuit_simable = nfsc::BulbToys_CreatePursuitSimable(nfsc::driver_class::none);
					WriteMemory<void*>(nfsc::ThePursuitSimables + 4 * i, pursuit_simable);

					void* pursuit_vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(pursuit_simable);
					nfsc::PVehicle_Deactivate(pursuit_vehicle);
				}

				// Then our own pursuit simable
				void* pursuit_simable = nfsc::BulbToys_CreatePursuitSimable(nfsc::driver_class::none);
				WriteMemory<void*>(nfsc::ThePursuitSimables, pursuit_simable);

				void* pursuit_vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(pursuit_simable);
				nfsc::PVehicle_Deactivate(pursuit_vehicle);
			}

			// We need to create our racer vehicle and player before switching first
			void* racer_vehicle = RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
			void* racer_simable = nfsc::PVehicle_GetSimable(racer_vehicle);

			nfsc::AIPlayer* ai_player = nfsc::AIPlayer::CreateInstance();

			// bool (PhysicsObject) ISimable::Attach(ISimable*, IPlayer*)
			reinterpret_cast<bool(__thiscall*)(void*, void*)>(0x6C6740)(racer_simable, &ai_player->IPlayer);
			
			// In PTAG, opponents start as cops first
			int _;
			vehicle = nfsc::Game_PursuitSwitch(racer_index, true, &_);

			// Copy pursuit simable's handle into our racer info
			void* simable = nfsc::PVehicle_GetSimable(vehicle);
		}

		// For PKO, AI racers start as racers, so create our racer vehicles/simables first (so the vehicle start grid warping works properly)
		else if (type == nfsc::race_type::pursuit_ko)
		{
			// Online gamemodes skip the intro NIS, but it's fine here since it starts off as a normal circuit race and thus the normal intro NIS fits
			*nfsc::SkipNIS = false;

			// Create and use our own racer vehicle and player
			vehicle = RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);

			nfsc::AIPlayer* ai_player = nfsc::AIPlayer::CreateInstance();

			nfsc::PhysicsObject_Attach(nfsc::PVehicle_GetSimable(vehicle), &ai_player->IPlayer);

			int racer_count = nfsc::GRaceStatus_GetRacerCount(ReadMemory<void*>(nfsc::GRaceStatus));

			// Final racer's vehicle was created, now we can create our pursuit simables
			// If we created one per racer in this function, the start grid vehicles would get fucked
			if (racer_index == racer_count - 1)
			{
				for (int i = 0; i < racer_count; i++)
				{
					void* pursuit_simable = nfsc::BulbToys_CreatePursuitSimable(nfsc::driver_class::none);
					
					// Store and deactivate
					WriteMemory<void*>(nfsc::ThePursuitSimables + 4 * i, pursuit_simable);

					void* pursuit_vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(pursuit_simable);
					nfsc::PVehicle_Deactivate(pursuit_vehicle);
				}
			}
		}
	}
	else // Not a PKO/PTAG race
	{
		*nfsc::SkipNIS = false;
		vehicle = RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
	}

	return vehicle;
}

const char* __cdecl hooks::GetPursuitVehicleNameHook(bool is_player)
{
	void* my_ivehicle = *nfsc::IVehicleList->begin;
	if (!my_ivehicle)
	{
		// Use fallback
		return "player_cop";
	}

	void* my_pvehicle = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(my_ivehicle) - 0xD0);

	int tier = nfsc::BulbToys_GetPVehicleTier(my_pvehicle);
	if (tier < 1 || tier > 3)
	{
		// Use fallback
		return "player_cop";
	}

	// TODO: apparently, PD2 is the T1 and PD1 is the T2 car, but that's not what game code dictates?
	return nfsc::player_cop_cars[tier - 1];
}

void __fastcall hooks::RaceStatusUpdateHook(void* race_status, void* edx, float dt)
{
	RaceStatusUpdate(race_status, edx, dt);

	nfsc::race_type type = nfsc::BulbToys_GetRaceType();

	if (type == nfsc::race_type::pursuit_tag)
	{
		int runner_index = -1;
		void* runner_simable = nfsc::GRaceStatus_GetRacePursuitTarget(race_status, &runner_index);

		// FIXME: AI does not pursue you under any of the following conditions
		// - Goal set to AIGoalPursuit, AIGoalHassle, AIGoalPursuitEncounter, AIGoalRam, AIGoalPit; combined with Game_SetPursuitTarget
		// - AIVehicle set to CopCar or Pursuit (using BEHAVIOR_MECHANIC_AI and committing)
		// - AIAction manipulation, particularly removing AIActionRace (necessary for all non-traffic vehicles as it's directly responsible for their driving)
		// - Setting DriverClass to cop (game crashes by stack overflow in ExtrapolatedRacer dtor ?????)
		// - Probably a couple other things i forgot to mention here
		// As a temporary horrible hack, we path to the pursuee every race update. This is very inaccurate and the AI does not behave properly
		void* runner_rigidbody = nfsc::PhysicsObject_GetRigidBody(runner_simable);
		nfsc::Vector3* runner_pos = nfsc::RigidBody_GetPosition(runner_rigidbody);

		for (int i = 0; i < nfsc::GRaceStatus_GetRacerCount(race_status); i++)
		{
			if (i == runner_index)
			{
				continue;
			}

			void* racer_info = nfsc::GRaceStatus_GetRacerInfo(race_status, i);
			void* simable = nfsc::GRacerInfo_GetSimable(racer_info);
			void* vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(simable);
			void* ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(vehicle);
			nfsc::BulbToys_PathToTarget(ai_vehicle, runner_pos);
		}

		// Check the racer's 60 second fleeing timer. If we've run out, switch to our nearest cop
		// TODO: use pursuit contributions instead?
		float runner_timer = ReadMemory<float>(reinterpret_cast<uintptr_t>(race_status) + 0xBFB0);
		if (runner_timer <= 0)
		{
			nfsc::BulbToys_SwitchPTagTarget(race_status, false);
		}
	}
}

/*
void* __cdecl hooks::PursuitSwitchHook(int racer_index, bool is_busted, int* result)
{
	void* vehicle = PursuitSwitch(racer_index, is_busted, result);

	void* simable = nfsc::PVehicle_GetSimable(vehicle);

	void* player = reinterpret_cast<void* (__thiscall*)(void*)>(0x6D6C40)(simable);

	Error("SWITCH!\n\nIndex: %d\nBusted: %s\nResult: %d\n\nVehicle: %p\nPlayer: %p", racer_index, is_busted? "true" : "false", *result, vehicle, player);

	return vehicle;
}
*/

float __fastcall hooks::GetTimeLimitHook(void* race_parameters)
{
	if (nfsc::BulbToys_GetRaceType() == nfsc::race_type::pursuit_tag)
	{
		// FEManager::GetUserProfile(FEManager::mInstance, 0);
		uintptr_t user_profile = reinterpret_cast<uintptr_t(__thiscall*)(void*, int)>(0x572B90)(ReadMemory<void*>(0xA97A7C), 0);

		// user_profile->mRaceSettings[11 (== PTag)].lap_count;
		int num_laps = ReadMemory<uint8_t>(user_profile + 0x2B258);

		// Time limit is in seconds, so we multiply by 60 to get minutes
		return static_cast<float>(num_laps * 60);
	}

	return GetTimeLimit(race_parameters);
}

void __cdecl hooks::ShowLosingScreenHook()
{
	nfsc::race_type type = nfsc::BulbToys_GetRaceType();

	// PTag appears to utilize FE_ShowLosingPostRaceScreen, which calls FE_ShowPostRaceScreen
	// But FE_ShowPostRaceScreen also gets called in Game_EnterPostRaceFlow
	// To prevent the infamous double end screen softlock, we're blocking this screen for the former function (this hook)
	if (type != nfsc::race_type::pursuit_tag && type != nfsc::race_type::pursuit_ko)
	{
		ShowLosingScreen();
	}
}

void __cdecl hooks::ShowWinningScreenHook()
{
	nfsc::race_type type = nfsc::BulbToys_GetRaceType();

	// Ditto, but for FE_ShowWinningPostRaceScreen
	if (type != nfsc::race_type::pursuit_tag && type != nfsc::race_type::pursuit_ko)
	{
		ShowWinningScreen();
	}
}

void __fastcall hooks::CareerManagerChildFlowDoneHook(uintptr_t fe_career_state_manager, void* edx, int unk)
{
	int cur_state = ReadMemory<uintptr_t>(fe_career_state_manager + 4);

	if (cur_state == 15)
	{
		// FEStateManager::Switch(this, "FeMainMenu_Sub.fng", 0x93E8A57C, 1, 1);
		reinterpret_cast<void(__thiscall*)(uintptr_t, const char*, uint32_t, int, int)>(0x59B140)(fe_career_state_manager, "FeMainMenu_Sub.fng", 0x93E8A57C, 1, 1);
	}
	else
	{
		CareerManagerChildFlowDone(fe_career_state_manager, edx, unk);
	}
}

void __fastcall hooks::WorldMapPadAcceptHook(void* fe_state_manager)
{
	WorldMapPadAccept(fe_state_manager);

	// Get current WorldMap and TrackInfo
	auto world_map = ReadMemory<void*>(0xA977F0);
	auto track_info = ReadMemory<void*>(0xB69BA0);
	if (!world_map || !track_info)
	{
		g::location[0] = NAN;
		g::location[1] = NAN;
		g::location[2] = NAN;
		return;
	}

	// Get the current position of the cursor relative to the screen
	float x, y;
	nfsc::FE_Object_GetCenter(ReadMemory<void*>(reinterpret_cast<uintptr_t>(world_map) + 0x28), &x, &y);

	// Account for WorldMap pan
	nfsc::Vector2 temp;
	temp.x = x;
	temp.y = y;

	nfsc::WorldMap_GetPanFromMapCoordLocation(world_map, &temp, &temp);

	x = temp.x;
	y = temp.y;

	// Account for WorldMap zoom
	nfsc::Vector2 top_left = ReadMemory<nfsc::Vector2>(reinterpret_cast<uintptr_t>(world_map) + 0x44);
	nfsc::Vector2 size = ReadMemory<nfsc::Vector2>(reinterpret_cast<uintptr_t>(world_map) + 0x4C);

	x = x * size.x + top_left.x;
	y = y * size.y + top_left.y;

	// Inverse WorldMap::ConvertPos to get world coordinates
	float calibration_width = ReadMemory<float>(reinterpret_cast<uintptr_t>(track_info) + 0xB4);
	float calibration_offset_x = ReadMemory<float>(reinterpret_cast<uintptr_t>(track_info) + 0xAC);
	float calibration_offset_y = ReadMemory<float>(reinterpret_cast<uintptr_t>(track_info) + 0xB0);

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
	nfsc::Vector3 position;
	position.x = -y;
	position.y = 0; // z
	position.z = x;

	// Attempt to get world height at given position. If it can't (returns false), height will be NaN
	nfsc::WCollisionMgr mgr;
	mgr.fSurfaceExclusionMask = 0;
	mgr.fPrimitiveMask = 3;

	float height = NAN;
	nfsc::WCollisionMgr_GetWorldHeightAtPointRigorous(&mgr, &position, &height, nullptr);
	position.y = height;

	// Return
	g::location[0] = position.x;
	g::location[1] = position.y + g::extra_height;
	g::location[2] = position.z;
}

void __fastcall hooks::WorldMapButtonPressedHook(uintptr_t fe_state_manager, void* edx, uint32_t unk)
{
	// Only offer GPS if the option is enabled and we're not in FE
	if (g::gps_only::enabled && *nfsc::GameFlowManager_State == nfsc::gameflow_state::racing)
	{
		// mCurrentState
		auto type = ReadMemory<g::gps_only::dialog_type>(fe_state_manager + 4);

		if (type == g::gps_only::dialog_type::race_event || type == g::gps_only::dialog_type::car_lot || type == g::gps_only::dialog_type::safehouse)
		{
			// For these types of dialogs, use the button hashes of the GPS to safehouse prompt during pursuits
			WriteMemory<int>(fe_state_manager + 4, 18);

			uintptr_t dialog_screen = ReadMemory<uintptr_t>(0xA97B14);
			uint32_t* button_hashes = ReadMemory<uint32_t*>(dialog_screen + 0x2C);

			// First button - Activate GPS
			if (unk == button_hashes[0])
			{
				nfsc::FEStateManager_ChangeState(reinterpret_cast<void*>(fe_state_manager), 21);
			}

			// Second button - Cancel
			else if (unk == button_hashes[1])
			{
				nfsc::FEStateManager_PopBack(reinterpret_cast<void*>(fe_state_manager), 3);
			}

			return;
		}
	}

	WorldMapButtonPressed(fe_state_manager, edx, unk);
}

void __fastcall hooks::WorldMapShowDialogHook(uintptr_t fe_state_manager)
{
	// Only offer GPS if the option is enabled and we're not in FE
	if (g::gps_only::enabled && *nfsc::GameFlowManager_State == nfsc::gameflow_state::racing)
	{
		// mCurrentState
		auto type = ReadMemory<g::gps_only::dialog_type>(fe_state_manager + 4);

		const char* DIALOG_MSG_ACTIVATE_GPS = nfsc::GetLocalizedString(0x6EB0EACE);
		const char* COMMON_CANCEL = nfsc::GetLocalizedString(0x1A294DAD);

		if (type == g::gps_only::dialog_type::race_event)
		{
			// Unhide the cursor and show the dialog with its respective string
			WriteMemory<bool>(0xA97B38, false);

			// DIALOG_MSG_WORLDMAP_EVENT
			nfsc::FEDialogScreen_ShowDialog(nfsc::GetLocalizedString(0xCF93709B), DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL, nullptr);
			return;
		}
		else if (type == g::gps_only::dialog_type::car_lot)
		{
			WriteMemory<bool>(0xA97B38, false);
			
			// DIALOG_MSG_WORLDMAP_CARLOT
			nfsc::FEDialogScreen_ShowDialog(nfsc::GetLocalizedString(0xBBE2483E), DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL, nullptr);

			return;
		}
		else if (type == g::gps_only::dialog_type::safehouse)
		{
			WriteMemory<bool>(0xA97B38, false);

			// DIALOG_MSG_WORLDMAP_SAFEHOUSE
			nfsc::FEDialogScreen_ShowDialog(nfsc::GetLocalizedString(0x8B776D3C), DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL, nullptr);

			return;
		}
	}

	WorldMapShowDialog(fe_state_manager);
}

/*__declspec(naked) void hooks::CreateRoadBlockHook()
{
	__asm
	{
		// Redo what we've overwritten
		inc		dword ptr[eax + 0x90]

		// Increment HUD count by 1 (per roadblock vehicle)
		mov     eax, [esp + 0x558]
		inc     dword ptr[eax + 0x194]

		// Continue as normal (return to just after the jump)
		push    0x445AA3
		ret
	}
}*/

uintptr_t IVehicle_temp;
__declspec(naked) void hooks::UpdateCopElementsHook1()
{
	__asm
	{
		// Redo what we've overwritten
		mov     esi, [ecx]
		mov     edx, [esi]
		mov     ecx, esi

		// In the first part, we store the cop vehicle in question
		mov     IVehicle_temp, ecx
		push    0x5D8C16
		ret
	}
}

// This is wasteful, but MASM lacks a JNZ instruction, so there really is no better way
constexpr uintptr_t AIVehicle_GetVehicle = 0x406700;
constexpr uintptr_t PVehicle_IsDestroyed = 0x6D8030;
constexpr uintptr_t Minimap_GetCurrCopElementColor = 0x5D2200;
__declspec(naked) void hooks::UpdateCopElementsHook2()
{
	__asm
	{
		// Skip changing the color if the cop vehicle is destroyed
		mov     ecx, IVehicle_temp
		call    PVehicle_IsDestroyed
		test    eax, eax
		jz      not_destroyed
		push    0x5D8D06
		ret

	not_destroyed:
		// Redo what we've overwritten
		mov     ecx, [esp + 0xC]
		call    Minimap_GetCurrCopElementColor
		mov     edi, eax
		push    0x5D8D06
		ret
	}
}

/*
//constexpr uintptr_t Sim_Activity_DetachAll = 0x75DFA0;
//constexpr uintptr_t Sim_Activity_Detach = 0x407260;
constexpr uintptr_t AIPursuit_Attach = 0x412850;
uintptr_t AIPursuit;
uintptr_t AIRoadBlock;
//uintptr_t IVehicle;
__declspec(naked) void hooks::UpdateRoadBlocksHook()
{
	__asm
	{
		// Redo what we've overwritten
		//inc     dword ptr[edi + 0x1B4]
		add     [edi + 0x1D0], ecx

		// Detach everything from the roadblock
		//mov     ecx, esi
		//call    Sim_Activity_DetachAll

		// Preserve pursuit and roadblock
		mov     AIPursuit, edi
		mov     AIRoadBlock, esi

		push    ebx
		push    ebp
		push    esi
		push    edi

		// Check if the vehicle list is empty, bail if so, iterate otherwise
		mov     ebx, AIRoadBlock
		mov     eax, [ebx + 0x40]
		mov     edi, [ebx + 0x44]
		cmp     edi, eax
		jz      done_iterating

	iterate:
		mov     esi, [edi]
		//test    esi, esi
		//jz      next

		//mov     IVehicle, esi

		//mov     ecx, AIRoadBlock
		//add     ecx, 0x24
		//push    esi
		//call    Sim_Activity_Detach

		// Detach from roadblock
		//mov     eax, IVehicle
		//mov     ecx, AIRoadBlock
		//mov     edx, [ecx + 0x24]
		//add     ecx, 0x24
		//push    eax
		//call    dword ptr[edx + 0xC]

		// Attach to pursuit
		//mov     eax, IVehicle
		mov     ecx, AIPursuit
		//push    eax
		push    esi
		call    AIPursuit_Attach

	//next:
		// Next vehicle, check if it's the last vehicle, stop iterating if so, continue iterating otherwise
		//mov     ebx, AIRoadBlock
		mov     eax, [ebx + 0x40]
		sub     edi, 4
		cmp     edi, eax
		jz      done_iterating
		jmp     iterate

	done_iterating:
		pop     edi
		pop     esi
		pop     ebp
		pop     ebx
		//push    0x4410DA
		push    0x4411B3
		ret
	}
}
*/

__declspec(naked) void hooks::MoveVinylVerticalHook()
{
	__asm
	{
		// [esp + 4] contains the step size (normally -1 or 1, depending on direction)
		mov     edx, [esp + 4]
		cmp     edx, 0
		jl      negative

		add     eax, g::move_vinyl::step_size
		jmp     done

	negative:
		sub     eax, g::move_vinyl::step_size

	done:
		// Redo what we've overwritten
		cmp     eax, 0xFFFFFE00
		push    0x7B0F6C
		ret
	}
}

__declspec(naked) void hooks::MoveVinylHorizontalHook()
{
	__asm
	{
		// [esp + 4] contains the step size (normally -1 or 1, depending on direction)
		mov     edx, [esp + 4]
		cmp     edx, 0
		jl      negative

		add     eax, g::move_vinyl::step_size
		jmp     done

	negative:
		sub     eax, g::move_vinyl::step_size

	done:
		// Redo what we've overwritten
		cmp     eax, 0xFFFFFE00
		push    0x7B0F9D
		ret
	}
}

__declspec(naked) void hooks::VehicleChangeCacheHook()
{
	__asm
	{
		// Redo what we've overwritten
		add     esp, 4
		push    eax
		push    0

		// use GRaceStatus::fObj instead of DebugVehicleSelection as vehicle cache
		mov     ecx, 0xA98284 
		mov     ecx, [ecx]
		push    ecx

		push    0x7D4EC2
		ret
	}
}

constexpr uintptr_t PhysicsObject_IsPlayer = 0x6D6C30;
constexpr uintptr_t PVehicle_GetSimable = 0x6D7EC0;
// void __thiscall UTL::ListableSet<IVehicle,12,enum eVehicleList,12>::AddToList(char *this, eVehicleList list)
constexpr uintptr_t AddToList = 0x6CFBA0;
__declspec(naked) void hooks::UpdateAIPlayerListingHook()
{
	__asm
	{
		// Redo what we've overwritten
		call    AddToList

		lea     ecx, [esi - 8]
		call    PVehicle_GetSimable
		mov     ecx, eax
		test    eax, eax
		jz      skip

		call    PhysicsObject_IsPlayer
		test    eax, eax
		jz      skip

		// Add to player list
		push    1
		mov     ecx, esi
		call    AddToList

		// Skip adding to the player list if this vehicle doesn't have a simable and/or is not a player
	skip:
		push    0x6D40AB
		ret
	}
}

void hooks::PTagBustedHook()
{
	__asm
	{
		// If we've been busted as a racer, switch vehicles with our nearest cop
		// TODO: use pursuit contributions instead?

		// In the __fastcall calling convention, the first argument is stored in ecx (pointer to GRaceStatus instance)
		mov     ecx, nfsc::GRaceStatus
		mov     ecx, [ecx]

		// The second argument is stored in edx (is_busted (= true))
		mov     edx, 1
		call    nfsc::BulbToys_SwitchPTagTarget

		push    0x44A59B
		ret

		// No need to redo what we've overwritten cuz it's useless lol (we overwrote some call to an online-related function)
	}
}