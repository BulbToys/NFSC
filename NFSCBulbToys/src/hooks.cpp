#include "shared.h"

MH_STATUS hooks::CreateHook(uintptr_t address, void* hook, void* call)
{
	auto status = MH_CreateHook(reinterpret_cast<void*>(address), hook, reinterpret_cast<void**>(call));

	if (status != MH_OK)
	{
		return status;
	}

	return MH_EnableHook(reinterpret_cast<void*>(address));
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
	auto device = ReadMemory<uintptr_t>(nfsc::Direct3DDevice9);
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

bool hooks::SetupPart2(uintptr_t device)
{
	gui::SetupMenu(reinterpret_cast<IDirect3DDevice9*>(device));

	auto status = CreateHook(VirtualFunction(device, 42), &EndSceneHook, &EndScene);
	if (status != MH_OK)
	{
		Error("%08X", VirtualFunction(device, 42));
		Error("Failed to hook EndScene() with MH_STATUS code %d.", status);
		return false;
	}

	status = CreateHook(VirtualFunction(device, 16), &ResetHook, &Reset);
	if (status != MH_OK)
	{
		Error("Failed to hook Reset() with MH_STATUS code %d.", status);
		return false;
	}

	/* Non-critical hooks go here */

	// Optionally override encounter spawn requirement
	CreateHook(0x422BF0, &NeedsEncounterHook, &NeedsEncounter);

	// Optionally override traffic spawn requirement
	CreateHook(0x422990, &NeedsTrafficHook, &NeedsTraffic);

	// Optionally override whether racers should be pursued or not
	CreateHook(0x423F40, &PursueRacersHook, &PursueRacers);

	// Smart AI hooks
	// - Make autopilot drive to the location marked by the GPS
	// - Make autopilot drive to the location after a navigation reset
	CreateHook(0x433930, &GpsEngageHook, &GpsEngage);
	CreateHook(0x427AD0, &ResetDriveToNavHook, &ResetDriveToNav);

	// Calculate world positions from map positions
	// Disable snapping to world map icons if we're holding shift
	// GPS only mode
	// Wrong warp fix
	CreateHook(0x5B3850, &WorldMapPadAcceptHook, &WorldMapPadAccept);
	//CreateHook(0x5C3330, &WorldMapSnapHook, &WorldMapSnap);
	CreateHook(0x5CF890, &WorldMapShowDialogHook, &WorldMapShowDialog);
	CreateHook(0x5B3570, &WorldMapButtonPressedHook, &WorldMapButtonPressed);
	CreateHook(0x5B3A10, &WorldMapStateChangeHook, &WorldMapStateChange);
	CreateHook(0x5CAED0, &WorldMapScreenTickHook, &WorldMapScreenTick);
	CreateHook(0x5B3CF0, &WorldMapButton4Hook, &WorldMapButton4);

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

	// Custom encounter vehicles
	CreateHook(0x42CB70, &GetAvailablePresetVehicleHook, &GetAvailablePresetVehicle);

	// Replace "Dump Preset" with "Add to My Cars" for DebugCarCustomize
	CreateHook(0x854890, &DebugCarPadButton3Hook, &DebugCarPadButton3);

	// Reset GRaceStatus vehicle count to 0 when the race ends
	CreateHook(0x641310, &SetRoamingHook, &SetRoaming);

	// Add health icons above vehicles
	CreateHook(0x7AEFD0, &UpdateIconHook, &UpdateIcon);

	// FOV overrides
	CreateHook(0x4822F0, &SetCameraMatrixHook, &SetCameraMatrix);

	// Make Neville a buggy "boss" if he's racing cuz it's funny lol
	CreateHook(0x643A50, &GetMainBossHook, &GetMainBoss);
	
	// Add racers and GPS icon to the world map
	CreateHook(0x5ACB50, &AddPlayerCarHook, &AddPlayerCar);

	// PIP tests
	CreateHook(0x44BCE0, &MLaunchPIPHook, &MLaunchPIP);

	CreateHook(0x444D90, &SpawnEncounterHook, &SpawnEncounter);

	// Override the chosen roadblock with our own when manually spawning roadblocks
	// TODO: uncomment when i've unfucked roadblock creation (might even be useless)
	//CreateHook(0x407040, &PickRoadblockSetupHook, &PickRoadblockSetup);

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

	// Better debug cam teleport (now properly on ground and takes forward vector into consideration)
	PatchJmp(0x49319E, DebugActionDropCarHook);

	// Fix crash when spawning a wingman lacking speech
	PatchJmp(0x793914, NoWingmanSoundHook, 6);
	
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
	if (FAILED(result))
	{
		return result;
	}

	MH_DisableHook(reinterpret_cast<void*>(0x710220));
	MH_RemoveHook(reinterpret_cast<void*>(0x710220));

	// The initial setup has completed and thus the thread is operational
	// Force it to exit in case our final setup fails
	exitMainLoop = !SetupPart2(ReadMemory<uintptr_t>(nfsc::Direct3DDevice9));

	return result;
}

long __stdcall hooks::EndSceneHook(IDirect3DDevice9* device)
{
	const auto result = EndScene(device);

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//static uint32_t frame_count[2] {0, 0};

	//frame_count[0] = frame_count[1];
	//frame_count[1] = ReadMemory<uint32_t>(0xA996F0); // RealLoopCounter

	// Certain loading screens render the game twice, in which case we shouldn't render the GUI
	// TODO: uncomment when i crash here ig lol (window will be null)
	//if (frame_count[0] != frame_count[1])

	gui::Render();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return result;
}

HRESULT __stdcall hooks::ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = Reset(device, params);
	ImGui_ImplDX9_CreateDeviceObjects();
	return result;
}

bool __fastcall hooks::NeedsEncounterHook(uintptr_t traffic_manager)
{
	// If we've overridden the value, use our own. Instead, let the game decide normally
	return g::needs_encounter::overridden ? g::needs_encounter::value : NeedsEncounter(traffic_manager);
}

bool __fastcall hooks::NeedsTrafficHook(uintptr_t traffic_manager)
{
	// Ditto
	return g::needs_traffic::overridden ? g::needs_traffic::value : NeedsTraffic(traffic_manager);
}

bool __fastcall hooks::PursueRacersHook(uintptr_t ai_cop_manager)
{
	// Ditto
	return g::pursue_racers::overridden ? g::pursue_racers::value : PursueRacers(ai_cop_manager);
}

bool __fastcall hooks::GpsEngageHook(uintptr_t gps, uintptr_t edx, nfsc::Vector3* target, float max_deviation, bool re_engage, bool always_re_establish)
{
	bool result = GpsEngage(gps, edx, target, max_deviation, re_engage, always_re_establish);

	// GPS failed to establish
	if (!result)
	{
		return result;
	}

	uintptr_t my_vehicle = 0;
	nfsc::BulbToys_GetMyVehicle(&my_vehicle, nullptr);
	if (!my_vehicle)
	{
		return result;
	}

	auto ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(my_vehicle);
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

void __fastcall hooks::ResetDriveToNavHook(uintptr_t ai_vehicle, uintptr_t edx, int lane_selection)
{
	ResetDriveToNav(ai_vehicle, edx, lane_selection);

	if (!nfsc::GPS_IsEngaged())
	{
		return;
	}

	uintptr_t my_vehicle = 0;
	nfsc::BulbToys_GetMyVehicle(&my_vehicle, nullptr);
	if (!my_vehicle)
	{
		return;
	}

	auto local_ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(my_vehicle);
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
uintptr_t __fastcall hooks::RacerInfoCreateVehicleHook(uintptr_t racer_info, uintptr_t edx, uint32_t key, int racer_index, uint32_t seed)
{
	nfsc::race_type type = nfsc::BulbToys_GetRaceType();

	if (type == nfsc::race_type::pursuit_ko)
	{
		uintptr_t stored_vehicle = RacerInfoCreateVehicle(racer_info, edx, nfsc::GKnockoutRacer_GetPursuitVehicleKey(0), racer_index, seed);
		Error("%p", stored_vehicle);
		nfsc::PVehicle_SetDriverClass(stored_vehicle, nfsc::driver_class::none);
		nfsc::PVehicle_Deactivate(stored_vehicle);
		WriteMemory<uintptr_t>(nfsc::ThePursuitSimables + 4 * racer_index, nfsc::PVehicle_GetSimable(stored_vehicle));

		return RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
	}
	else if (type == nfsc::race_type::pursuit_tag)
	{
		if (racer_index == 1)
		{
			uintptr_t player_pursuit_simable = nfsc::BulbToys_CreatePursuitSimable(nfsc::driver_class::none);
			WriteMemory<uintptr_t>(nfsc::ThePursuitSimables, player_pursuit_simable);
			nfsc::PVehicle_Deactivate(nfsc::BulbToys_FindInterface<nfsc::IVehicle>(player_pursuit_simable));
		}

		uintptr_t stored_vehicle = RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
		nfsc::PVehicle_SetDriverClass(stored_vehicle, nfsc::driver_class::none);
		nfsc::PVehicle_Deactivate(stored_vehicle);
		WriteMemory<uintptr_t>(nfsc::ThePursuitSimables + 4 * racer_index, nfsc::PVehicle_GetSimable(stored_vehicle));

		return RacerInfoCreateVehicle(racer_info, edx, nfsc::GKnockoutRacer_GetPursuitVehicleKey(0), racer_index, seed);
	}

	return RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
}
*/

uintptr_t __fastcall hooks::RacerInfoCreateVehicleHook(uintptr_t racer_info, uintptr_t edx, uint32_t key, int racer_index, uint32_t seed)
{
	uintptr_t vehicle = 0;

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
				for (int i = 1; i < nfsc::GRaceStatus_GetRacerCount(ReadMemory<uintptr_t>(nfsc::GRaceStatus)); i++)
				{
					uintptr_t pursuit_simable = nfsc::BulbToys_CreatePursuitSimable();
					WriteMemory<uintptr_t>(nfsc::ThePursuitSimables + 4 * i, pursuit_simable);

					uintptr_t pursuit_vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(pursuit_simable);
					nfsc::PVehicle_Deactivate(pursuit_vehicle);
				}

				// Then our own pursuit simable
				uintptr_t pursuit_simable = nfsc::BulbToys_CreatePursuitSimable();
				WriteMemory<uintptr_t>(nfsc::ThePursuitSimables, pursuit_simable);

				uintptr_t pursuit_vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(pursuit_simable);
				nfsc::PVehicle_Deactivate(pursuit_vehicle);
			}

			// We need to create our racer vehicle and player before switching first
			uintptr_t racer_vehicle = RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
			uintptr_t racer_simable = nfsc::PVehicle_GetSimable(racer_vehicle);

			nfsc::AIPlayer* ai_player = nfsc::AIPlayer::New();

			// bool (PhysicsObject) ISimable::Attach(ISimable*, IPlayer*)
			reinterpret_cast<bool(__thiscall*)(uintptr_t, void*)>(0x6C6740)(racer_simable, &ai_player->IPlayer);
			
			// In PTAG, opponents start as cops first
			int _;
			vehicle = nfsc::Game_PursuitSwitch(racer_index, true, &_);

			// Copy pursuit simable's handle into our racer info
			uintptr_t simable = nfsc::PVehicle_GetSimable(vehicle);
		}

		// For PKO, AI racers start as racers, so create our racer vehicles/simables first (so the vehicle start grid warping works properly)
		else if (type == nfsc::race_type::pursuit_ko)
		{
			// Online gamemodes skip the intro NIS, but it's fine here since it starts off as a normal circuit race and thus the normal intro NIS fits
			*nfsc::SkipNIS = false;

			// Create and use our own racer vehicle and player
			vehicle = RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);

			nfsc::AIPlayer* ai_player = nfsc::AIPlayer::New();

			nfsc::PhysicsObject_Attach(nfsc::PVehicle_GetSimable(vehicle), reinterpret_cast<uintptr_t>(&ai_player->IPlayer));

			int racer_count = nfsc::GRaceStatus_GetRacerCount(ReadMemory<uintptr_t>(nfsc::GRaceStatus));

			// Final racer's vehicle was created, now we can create our pursuit simables
			// If we created one per racer in this function, the start grid vehicles would get fucked
			if (racer_index == racer_count - 1)
			{
				for (int i = 0; i < racer_count; i++)
				{
					uintptr_t pursuit_simable = nfsc::BulbToys_CreatePursuitSimable();
					
					// Store and deactivate
					WriteMemory<uintptr_t>(nfsc::ThePursuitSimables + 4 * i, pursuit_simable);

					uintptr_t pursuit_vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(pursuit_simable);
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
	uintptr_t my_vehicle = 0;
	nfsc::BulbToys_GetMyVehicle(&my_vehicle, nullptr);
	if (!my_vehicle)
	{
		// Use fallback
		return "player_cop";
	}

	uintptr_t my_pvehicle = my_vehicle - 0xD0;

	int tier = nfsc::BulbToys_GetPVehicleTier(my_pvehicle);
	if (tier < 1 || tier > 3)
	{
		// Use fallback
		return "player_cop";
	}

	// TODO: apparently, PD2 is the T1 and PD1 is the T2 car, but that's not what game code dictates?
	return nfsc::player_cop_cars[tier - 1];
}

void __fastcall hooks::RaceStatusUpdateHook(uintptr_t race_status, uintptr_t edx, float dt)
{
	RaceStatusUpdate(race_status, edx, dt);

	nfsc::race_type type = nfsc::BulbToys_GetRaceType();

	if (type == nfsc::race_type::pursuit_tag)
	{
		int runner_index = -1;
		uintptr_t runner_simable = nfsc::GRaceStatus_GetRacePursuitTarget(race_status, &runner_index);

		// FIXME: AI does not pursue you under any of the following conditions
		// - Goal set to AIGoalPursuit, AIGoalHassle, AIGoalPursuitEncounter, AIGoalRam, AIGoalPit; combined with Game_SetPursuitTarget
		// - AIVehicle set to CopCar or Pursuit (using BEHAVIOR_MECHANIC_AI and committing)
		// - AIAction manipulation, particularly removing AIActionRace (necessary for all non-traffic vehicles as it's directly responsible for their driving)
		// - Setting DriverClass to cop (game crashes by stack overflow in ExtrapolatedRacer dtor ?????)
		// - Probably a couple other things i forgot to mention here
		// As a temporary horrible hack, we path to the pursuee every race update. This is very inaccurate and the AI does not behave properly
		uintptr_t runner_rigidbody = nfsc::PhysicsObject_GetRigidBody(runner_simable);
		nfsc::Vector3* runner_pos = nfsc::RigidBody_GetPosition(runner_rigidbody);

		for (int i = 0; i < nfsc::GRaceStatus_GetRacerCount(race_status); i++)
		{
			if (i == runner_index)
			{
				continue;
			}

			uintptr_t racer_info = nfsc::GRaceStatus_GetRacerInfo(race_status, i);
			uintptr_t simable = nfsc::GRacerInfo_GetSimable(racer_info);
			uintptr_t vehicle = nfsc::BulbToys_FindInterface<nfsc::IVehicle>(simable);
			uintptr_t ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(vehicle);
			nfsc::BulbToys_PathToTarget(ai_vehicle, runner_pos);
		}

		// Check the racer's 60 second fleeing timer. If we've run out, switch to our nearest cop
		// TODO: use pursuit contributions instead?
		float runner_timer = ReadMemory<float>(race_status + 0xBFB0);
		if (runner_timer <= 0)
		{
			nfsc::BulbToys_SwitchPTagTarget(race_status, false);
		}
	}
}

/*
uintptr_t __cdecl hooks::PursuitSwitchHook(int racer_index, bool is_busted, int* result)
{
	uintptr_t vehicle = PursuitSwitch(racer_index, is_busted, result);

	uintptr_t simable = nfsc::PVehicle_GetSimable(vehicle);

	uintptr_t player = reinterpret_cast<uintptr_t (__thiscall*)(uintptr_t)>(0x6D6C40)(simable);

	Error("SWITCH!\n\nIndex: %d\nBusted: %s\nResult: %d\n\nVehicle: %p\nPlayer: %p", racer_index, is_busted? "true" : "false", *result, vehicle, player);

	return vehicle;
}
*/

float __fastcall hooks::GetTimeLimitHook(uintptr_t race_parameters)
{
	if (nfsc::BulbToys_GetRaceType() == nfsc::race_type::pursuit_tag)
	{
		// FEManager::GetUserProfile(FEManager::mInstance, 0);
		uintptr_t user_profile = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, int)>(0x572B90)(ReadMemory<uintptr_t>(0xA97A7C), 0);

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

void __fastcall hooks::CareerManagerChildFlowDoneHook(uintptr_t fe_career_state_manager, uintptr_t edx, int unk)
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

uintptr_t __fastcall hooks::GetAvailablePresetVehicleHook(uintptr_t ai_traffic_manager, uintptr_t edx, uint32_t skin_key, uint32_t encounter_key)
{
	if (g::encounter::overridden)
	{
		skin_key = 0;
		encounter_key = nfsc::Attrib_StringToKey(g::encounter::vehicle);
	}

	return GetAvailablePresetVehicle(ai_traffic_manager, edx, skin_key, encounter_key);
}

void __fastcall hooks::DebugCarPadButton3Hook(uintptr_t fe_debugcar_state_manager)
{
	// DALCareer::GetPodiumVehicle(&index);
	uint32_t index = 0;
	reinterpret_cast<char(__stdcall*)(uint32_t*)>(0x4A0890)(&index);
	
	// DALFeVehicle::AddCarToMyCarsDB(index)
	reinterpret_cast<char(__stdcall*)(uint32_t)>(0x4D1DE0)(index);
}

void __fastcall hooks::SetRoamingHook(uintptr_t g_race_status)
{
	SetRoaming(g_race_status);

	WriteMemory<uint32_t>(g_race_status + 0x6A08, 0);
}

void __fastcall hooks::UpdateIconHook(uintptr_t car_render_conn, uintptr_t edx, uintptr_t pkt)
{
	UpdateIcon(car_render_conn, edx, pkt);

	if (!g::health_icon::show)
	{
		return;
	}

	// Don't do anything if we have an icon already
	if (ReadMemory<uintptr_t>(car_render_conn + 0x1AC) != 0)
	{
		return;
	}

	uint32_t world_id = ReadMemory<uint32_t>(car_render_conn + 0x2C);

	uintptr_t this_vehicle = 0;
	for (size_t i = 0; i < nfsc::VehicleList[nfsc::vehicle_list::aicops]->size; i++)
	{
		uintptr_t vehicle = nfsc::VehicleList[nfsc::vehicle_list::aicops]->begin[i];
		uintptr_t simable = nfsc::PVehicle_GetSimable(vehicle);

		uint32_t simable_wid = reinterpret_cast<uint32_t(__thiscall*)(uintptr_t)>(0x6D6D10)(simable);

		if (simable_wid == world_id)
		{
			this_vehicle = vehicle;
			break;
		}
	}
	if (!this_vehicle)
	{
		return;
	}
	uintptr_t i_damageable = ReadMemory<uintptr_t>(this_vehicle + 0x44);

	// Set icon
	constexpr uint32_t INGAME_ICON_PLAYERCAR = 0x3E9CCFFA;
	WriteMemory<uintptr_t>(car_render_conn + 0x1AC, reinterpret_cast<uintptr_t(*)(uint32_t, int, int)>(0x55CFD0)(INGAME_ICON_PLAYERCAR, 1, 0));

	// Set scale
	WriteMemory<float>(car_render_conn + 0x1B4, 0.5f);

	// DamageVehicle::GetHealth
	float health = reinterpret_cast<float(__thiscall*)(uintptr_t)>(0x6F7790)(i_damageable);

	struct color_
	{
		uint8_t a = 0xFF;
		uint8_t b = 0;
		uint8_t g = 0;
		uint8_t r = 0;
	} color;

	if (health > 0)
	{
		if (health > 0.5)
		{
			health -= 0.5;
			color.g = 0xFF;
			color.r = 0xFF - (uint8_t)(0x1FF * health);
		}
		else
		{
			color.g = (uint8_t)(0x1FF * health);
			color.r = 0xFF;
		}
	}

	// Set color
	WriteMemory<color_>(car_render_conn + 0x1B0, color);
}

void __fastcall hooks::SetCameraMatrixHook(uintptr_t camera, uintptr_t edx, void* matrix4, float dt)
{
	uintptr_t fov = camera + 0xE4;
	if (camera == g::fov::player)
	{
		if (g::fov::player_override)
		{
			WriteMemory<int>(fov, g::fov::player_fov);
			SetCameraMatrix(camera, edx, matrix4, dt);
		}
		else
		{
			SetCameraMatrix(camera, edx, matrix4, dt);
			g::fov::player_fov = ReadMemory<int>(fov);
		}
	}
	else if (camera == g::fov::rvm)
	{
		if (g::fov::rvm_override)
		{
			WriteMemory<int>(fov, g::fov::rvm_fov);
			SetCameraMatrix(camera, edx, matrix4, dt);
		}
		else
		{
			SetCameraMatrix(camera, edx, matrix4, dt);
			g::fov::rvm_fov = ReadMemory<int>(fov);
		}
	}
	else if (camera == g::fov::pip)
	{
		if (g::fov::pip_override)
		{
			WriteMemory<int>(fov, g::fov::pip_fov);
			SetCameraMatrix(camera, edx, matrix4, dt);
		}
		else
		{
			SetCameraMatrix(camera, edx, matrix4, dt);
			g::fov::pip_fov = ReadMemory<int>(fov);
		}
	}
	else
	{
		SetCameraMatrix(camera, edx, matrix4, dt);
	}
}

uintptr_t __fastcall hooks::GetMainBossHook(uintptr_t g_race_status)
{
	uintptr_t racer_info = GetMainBoss(g_race_status);

	if (!racer_info)
	{
		uintptr_t my_simable = 0;
		nfsc::BulbToys_GetMyVehicle(nullptr, &my_simable);
		if (!my_simable)
		{
			return 0;
		}

		uintptr_t wingman_simable = reinterpret_cast<uintptr_t(*)(uintptr_t)>(0x6517E0)(my_simable);
		if (!wingman_simable)
		{
			return 0;
		}

		uintptr_t wingman_racer_info = nfsc::GRaceStatus_GetRacerInfo2(g_race_status, wingman_simable);
		if (!wingman_racer_info)
		{
			return 0;
		}

		char* wingman_name = reinterpret_cast<char*>(wingman_racer_info + 8);
		if (strncmp(wingman_name, "NEVILLE", 8))
		{
			return 0;
		}

		return wingman_racer_info;
	}

	return racer_info;
}
void __fastcall hooks::AddPlayerCarHook(uintptr_t world_map)
{
	constexpr uint32_t PLAYERCARINDICATOR = 0xDD9EF5FF;

	constexpr uint32_t MMICON_ROADBLOCK_4 = 0xCBD81AE5;

	// FEStateManager::IsGameMode
	if (!reinterpret_cast<bool(__thiscall*)(uintptr_t, int)>(0x5792A0)(ReadMemory<uintptr_t>(0xA97A7C), 0))
	{
		// this->PackageFilename
		char* package_name = ReadMemory<char*>(world_map + 0xC);
		if (package_name)
		{
			// FE::Object::FindObject
			uintptr_t object = reinterpret_cast<uintptr_t(*)(char*, uint32_t)>(0x5A0250)(package_name, PLAYERCARINDICATOR);

			// Type == FE_Image
			if (object && ReadMemory<int>(object + 0x18) == 1)
			{
				uintptr_t my_vehicle = 0;
				nfsc::BulbToys_GetMyVehicle(&my_vehicle, nullptr);

				auto list = nfsc::VehicleList[nfsc::vehicle_list::racers];
				for (int i = 0; i < (int)list->size; i++)
				{
					// malloc
					uintptr_t map_item = nfsc::malloc(0x30);
					if (map_item)
					{
						uintptr_t new_object = 0;

						auto racer = list->begin[i];
						if (racer == my_vehicle)
						{
							new_object = object;

							nfsc::FEColor cyan = { 255, 255, 115, 255 };
							nfsc::FE_Object_SetColor(new_object, &cyan);
						}
						else
						{
							// WorldMap::IsInPursuit
							if (reinterpret_cast<bool(__thiscall*)(uintptr_t)>(0x582E60)(world_map))
							{
								continue;
							}
							if (!nfsc::PVehicle_IsActive(racer))
							{
								continue;
							}

							// HIDING_SPOT_0 -> HIDING_SPOT_99
							new_object = nfsc::FE_Object_FindObject(package_name, nfsc::FE_String_HashString("HIDING_SPOT_%d", i));

							// FE::Image::SetTextureHash
							nfsc::FE_Image_SetTextureHash(new_object, ReadMemory<uint32_t>(object + 0x24));

							nfsc::FEColor orange = { 115, 195, 255, 255 };
							nfsc::FE_Object_SetColor(new_object, &orange);
						}

						nfsc::Vector2 position, direction;

						// GetVehicleVectors
						reinterpret_cast<void(*)(nfsc::Vector2&, nfsc::Vector2&, uintptr_t)>(0x5D89B0)(position, direction, nfsc::PVehicle_GetSimable(racer));

						nfsc::WorldMap_ConvertPos(position.x, position.y, *reinterpret_cast<nfsc::Vector2*>(world_map + 0x44),
							*reinterpret_cast<nfsc::Vector2*>(world_map + 0x4C));

						// bATan
						float rotation = reinterpret_cast<uint16_t(*)(float, float)>(0x46DFD0)(direction.y, direction.x) * 0.0054931641f;

						map_item = nfsc::MapItem_MapItem(map_item, 1, new_object, position, rotation, 0, 0);
						if (map_item)
						{
							// Add node to map items
							uintptr_t node = map_item + 4;
							uintptr_t prev = ReadMemory<uintptr_t>(world_map + 0x64 + 0x4);

							WriteMemory<uintptr_t>(prev, node);
							WriteMemory<uintptr_t>(world_map + 0x64 + 0x4, node);
							WriteMemory<uintptr_t>(node + 4, prev);
							WriteMemory<uintptr_t>(node, world_map + 0x64);
						}
					}
				}
			}
			/*
			uintptr_t gps_icon = ReadMemory<uintptr_t>(0xA977EC);
			if (gps_icon)
			{
				object = reinterpret_cast<uintptr_t(*)(char*, uint32_t)>(0x5A0250)(package_name, MMICON_ROADBLOCK_4);
				if (object && ReadMemory<int>(object + 0x18) == 1)
				{
					uintptr_t map_item = nfsc::malloc(0x30);
					if (map_item)
					{
						nfsc::Vector2 position = { ReadMemory<float>(gps_icon + 0x10), ReadMemory<float>(gps_icon + 0x10 + 4) };

						nfsc::WorldMap_ConvertPos(position.x, position.y, *reinterpret_cast<nfsc::Vector2*>(world_map + 0x44),
							*reinterpret_cast<nfsc::Vector2*>(world_map + 0x4C));

						nfsc::FEColor white = { 255, 255, 255, 255 };
						nfsc::FE_Object_SetColor(object, &white);

						// MINIMAP_ICON_EVENT
						nfsc::FE_Image_SetTextureHash(object, 0xB9358813);

						map_item = nfsc::MapItem_MapItem(map_item, 1, object, position, 0.0, 0, gps_icon);
						if (map_item)
						{
							// Add node to map items
							uintptr_t node = map_item + 4;
							uintptr_t prev = ReadMemory<uintptr_t>(world_map + 0x64 + 0x4);

							WriteMemory<uintptr_t>(prev, node);
							WriteMemory<uintptr_t>(world_map + 0x64 + 0x4, node);
							WriteMemory<uintptr_t>(node + 4, prev);
							WriteMemory<uintptr_t>(node, world_map + 0x64);
						}
					}
				}
			}
			*/
		}
	}
}

const char* pip_names[]
{
	"PLAYER_HIT_BOSS",
	"PLAYER_HIT_RIVAL",
	"PLAYER_CRASHES",
	"PLAYER_NEAR_FINISH",
	"PLAYER_PASS_BOSS_LEFT",
	"PLAYER_PASS_BOSS_RIGHT",
	"PLAYER_ENTER_COOLDOWN",
	"PLAYER_DRAFT_PAST_BOSS",
	"PLAYER_HIT_RACEBREAKER",
	"PLAYER_STAND_STILL",
	"BOSS_HIT_PLAYER",
	"BOSS_CRASHES",
	"BOSS_CROSS_FINISH",
	"BOSS_NEAR_FINISH",
	"BOSS_PASS_PLAYER_LEFT",
	"BOSS_PASS_PLAYER_RIGHT",
	"RIVAL_CRASHES",
	"RIVAL_CROSS_FINISH",
	"WINGMAN_HIT_RIVAL",
	"WINGMAN_CRASHES",
	"WINGMAN_PASS_RIVAL",
	"WINGMAN_NOS",
	"WINGMAN_ENTER_FIRSTPLACE",
	"WINGMAN_ENTER_LASTPLACE",
	"WINGMAN_JUMPS",
	"BLOCKER_ACTIVATE",
	"BLOCKER_ATTACK_BEHIND",
	"BLOCKER_ATTACK_FRONT",
	"BLOCKER_HIT_BOSS",
	"SCOUT_ACTIVATE",
	"SCOUT_ENTER_SHORTCUT",
	"DRAFTER_ACTIVATE",
	"COP_JOIN_PURSUIT",
	"CANYON_PLAYER_FALLBEHIND",
	"CANYON_PLAYER_FALLBEHIND_REPEAT",
	"CANYON_PLAYER_PULLAHEAD",
	"CANYON_BOSS_BROKE_GUARDRAIL",
	"CANYON_PLAYER_LOW_SCORE",
	"CANYON_PLAYER_LOW_SCORE_REPEAT",
	"CANYON_PLAYER_HIGH_SCORE",
	"CANYON_PLAYER_HIGH_SCORE_REPEAT",
	"CANYON_PLAYER_FAST_SCORE",
	"CANYON_BOSS_OFF_CLIFF",
	"TUTORIAL_IGNORED_COMMANDS",
	"TUTORIAL_REPEAT_HIT",
	"TUTORIAL_FAREWELL",
	"TUTORIAL_START"
};

uintptr_t __fastcall hooks::MLaunchPIPHook(uintptr_t m_launch_pip, uintptr_t edx, int id, uintptr_t simable_handle)
{
	gui::logger.Add(new gui::Log(0, "PIP: %d (%s)", id, pip_names[id - 1]));

	return MLaunchPIP(m_launch_pip, edx, id, simable_handle);
}

bool __fastcall hooks::SpawnEncounterHook(uintptr_t traffic_manager)
{
	// Save the time since the last encounter spawn for later calculations
	uintptr_t time_since_last_encounter_spawn = traffic_manager + 0x130;
	float time = ReadMemory<float>(time_since_last_encounter_spawn);

	// Tweak_TrafficRandomEncounter
	if (!ReadMemory<bool>(0xA4BF80))
	{
		return false;
	}

	// GCareer::mObj && (GCareer::mObj->mPendingAcitivity || *&GCareer::mObj->data_00_20[12]) - checks if we're in a race?
	uintptr_t g_career = ReadMemory<uintptr_t>(0xA982B8);
	if (g_career && (ReadMemory<uint32_t>(g_career + 0x118) || ReadMemory<int>(g_career + 0x110)))
	{
		return false;
	}

	uintptr_t vehicle, simable;
	if (!nfsc::BulbToys_GetMyVehicle(&vehicle, &simable))
	{
		return false;
	}

	uintptr_t vehicle_ai = nfsc::PVehicle_GetAIVehiclePtr(vehicle);
	if (!vehicle_ai)
	{
		return false;
	}

	// AIVehicle::GetPursuit
	uintptr_t pursuit = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(0x43BD80)(vehicle_ai);
	if (pursuit)
	{
		return false;
	}

	// AITrafficManager::NeedsEncounter - returns false if a SPAWNED/ACTIVE racer already exists
	if (!reinterpret_cast<bool(__thiscall*)(uintptr_t)>(0x422BF0)(traffic_manager))
	{
		return false;
	}

	// AITrafficManager::NextEncounterSpawn - is it time for us to spawn? returns false if next_encounter_key is null (gets generated in this function)
	// next_encounter_key only gets set to null after a successful spawn - it will remain intact should the vehicle still be loading
	if (!reinterpret_cast<bool(__thiscall*)(uintptr_t)>(0x444AC0)(traffic_manager))
	{
		return false;
	}

	// Time for us to spawn. If we fail for any reason (other than the vehicle loading), we start over at half our timer
	// TODO: fucked?
	WriteMemory<float>(time_since_last_encounter_spawn, time / 2);

	nfsc::WRoadNav nav;
	nfsc::WRoadNav_WRoadNav(nav);

	uintptr_t rigid_body = nfsc::PhysicsObject_GetRigidBody(simable);

	nfsc::Vector3 position = *nfsc::RigidBody_GetPosition(rigid_body);
	nfsc::Vector3 fwd_vec;
	nfsc::RigidBody_GetForwardVector(rigid_body, &fwd_vec);

	float distance = 300.f;

	// 50% chance to spawn an encounter behind us
	if (std::rand() % 2)
	{
		distance = 80.f;

		fwd_vec.x = -fwd_vec.x;
		fwd_vec.y = -fwd_vec.y;
		fwd_vec.z = -fwd_vec.z;
	}

	nav.fNavType = 2; // kTypeDirection

	nfsc::WRoadNav_InitAtPoint(nav, &position, &fwd_vec, false, 1.0);
	if (!nav.fValid)
	{
		nfsc::WRoadNav_Destructor(nav);
		return false;
	}

	nfsc::WRoadNav_IncNavPosition(nav, distance, &fwd_vec, 0.0, 0);

	nfsc::WCollisionMgr mgr;
	mgr.fPrimitiveMask = 3;
	mgr.fSurfaceExclusionMask = 0;

	float _;
	if (!nfsc::WCollisionMgr_GetWorldHeightAtPointRigorous(mgr, &nav.fPosition, &_, nullptr))
	{
		nfsc::WRoadNav_Destructor(nav);
		return 0;
	}

	fwd_vec.x = -nav.fForwardVector.x;
	fwd_vec.y = -nav.fForwardVector.y;
	fwd_vec.z = -nav.fForwardVector.z;

	uintptr_t encounter_vehicle = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint32_t, uint32_t)>(0x42CB70)
		(traffic_manager, ReadMemory<uint32_t>(traffic_manager + 0x120), ReadMemory<uint32_t>(traffic_manager + 0x128));
	if (!encounter_vehicle)
	{
		return false;
	}

	// PVehicle::IsLoading
	if (reinterpret_cast<bool(__thiscall*)(uintptr_t)>(0x6C0A00)(encounter_vehicle))
	{
		// Give it some time to load - try again after 1 second
		WriteMemory<float>(time_since_last_encounter_spawn, time - 1.0f);
		return false;
	}

	// Encounter has successfully spawned - reset timer and skin/vehicle keys to 0
	WriteMemory<float>(time_since_last_encounter_spawn, 0.0);
	WriteMemory<uint32_t>(traffic_manager + 0x120, 0);
	WriteMemory<uint32_t>(traffic_manager + 0x128, 0);

	nfsc::PVehicle_SetVehicleOnGround(encounter_vehicle, &nav.fPosition, &fwd_vec);

	uintptr_t encounter_ai = nfsc::PVehicle_GetAIVehiclePtr(encounter_vehicle);
	
	// AIVehicle::SetSpawned
	reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x418600)(encounter_ai);

	// AIVehicle::SetGoal
	constexpr uint32_t AIGoalEncounterPursuit = 0xD4272692;
	reinterpret_cast<void(__thiscall*)(uintptr_t, const uint32_t&)>(0x427A60)(encounter_ai, AIGoalEncounterPursuit);

	nfsc::PVehicle_Activate(encounter_vehicle);
	nfsc::PVehicle_SetSpeed(encounter_vehicle, 15.64605); // 35 mph

	nfsc::WRoadNav_Destructor(nav);

	return true;
}

/*
void* __cdecl hooks::PickRoadblockSetupHook(float width, int num_vehicles, bool use_spikes)
{
	return g::roadblock_setups::force ? g::roadblock_setups::force : PickRoadblockSetup(width, num_vehicles, use_spikes);
}
*/
void __fastcall hooks::WorldMapPadAcceptHook(uintptr_t fe_state_manager)
{
	// mCurrentState
	auto state = ReadMemory<nfsc::world_map_state>(fe_state_manager + 4);

	if (g::world_map::shift_held && state == nfsc::world_map_state::normal)
	{
		reinterpret_cast<void(__thiscall*)(uintptr_t, bool)>(0x582D90)(ReadMemory<uintptr_t>(nfsc::WorldMap), true);
		nfsc::FEStateManager_ShowDialog(fe_state_manager, (int)nfsc::world_map_state::click_tp);
		return;
	}

	WorldMapPadAccept(fe_state_manager);
}

void __fastcall hooks::WorldMapButtonPressedHook(uintptr_t fe_state_manager, uintptr_t edx, uint32_t unk)
{
	// mCurrentState
	auto state = ReadMemory<nfsc::world_map_state>(fe_state_manager + 4);

	uintptr_t dialog_screen = ReadMemory<uintptr_t>(0xA97B14);
	if (!dialog_screen)
	{
		WorldMapButtonPressed(fe_state_manager, edx, unk);
		return;
	}

	uint32_t* button_hashes = ReadMemory<uint32_t*>(dialog_screen + 0x2C);

	if (state == nfsc::world_map_state::click_tp)
	{
		if (*nfsc::GameFlowManager_State == nfsc::gameflow_state::racing && !isnan(g::world_map::location.y))
		{
			// First button - Jump to Location
			if (unk == button_hashes[0])
			{
				nfsc::FEStateManager_ChangeState(fe_state_manager, (int)nfsc::world_map_state::click_tp_jump);
			}

			// Second button - Activate GPS
			else if (unk == button_hashes[1])
			{
				nfsc::FEStateManager_ChangeState(fe_state_manager, (int)nfsc::world_map_state::click_tp_gps);
			}

			// Third button - Cancel
			else if (unk == button_hashes[2])
			{
				nfsc::FEStateManager_PopBack(fe_state_manager, 3);
			}
		}

		// First button - OK
		else if (unk == button_hashes[0])
		{
			nfsc::FEStateManager_PopBack(fe_state_manager, 3);
		}

		return;
	}

	// Only offer GPS if the option is enabled and we're not in FE
	if (g::world_map::gps_only && *nfsc::GameFlowManager_State == nfsc::gameflow_state::racing)
	{
		if (state == nfsc::world_map_state::race_event || state == nfsc::world_map_state::car_lot || state == nfsc::world_map_state::safehouse)
		{
			// For these types of dialogs, use the button hashes of the GPS to safehouse prompt during pursuits
			//WriteMemory<int>(fe_state_manager + 4, 18);

			// ???

			// First button - Activate GPS
			if (unk == button_hashes[0])
			{
				nfsc::FEStateManager_ChangeState(fe_state_manager, 21);
			}

			// Second button - Cancel
			else if (unk == button_hashes[1])
			{
				nfsc::FEStateManager_PopBack(fe_state_manager, 3);
			}

			return;
		}
	}

	WorldMapButtonPressed(fe_state_manager, edx, unk);
}

void __fastcall hooks::WorldMapStateChangeHook(uintptr_t fe_state_manager)
{
	// mCurrentState
	auto state = ReadMemory<nfsc::world_map_state>(fe_state_manager + 4);

	if (state == nfsc::world_map_state::click_tp_jump)
	{
		uintptr_t vehicle = 0;
		uintptr_t simable = 0;
		nfsc::BulbToys_GetMyVehicle(&vehicle, &simable);
		if (!vehicle || !simable)
		{
			return;
		}

		uintptr_t rigid_body = nfsc::PhysicsObject_GetRigidBody(simable);

		nfsc::Vector3 fwd_vec;
		nfsc::RigidBody_GetForwardVector(rigid_body, &fwd_vec);

		nfsc::Vector3 dimensions;
		nfsc::RigidBody_GetDimension(rigid_body, &dimensions);
		g::world_map::location.y += dimensions.y + 0.5f;

		nfsc::PVehicle_SetVehicleOnGround(vehicle, &g::world_map::location, &fwd_vec);

		// this->mNextManager = this->mParentManager;
		WriteMemory<uintptr_t>(fe_state_manager + 0xB4, ReadMemory<uintptr_t>(fe_state_manager + 0xAC));

		// this->mExitPoint = 2;
		WriteMemory<int>(fe_state_manager + 0xC, 2);

		// this->mSubState = 3;
		WriteMemory<int>(fe_state_manager + 0x18, 3);

		// FEStateManager::ProcessScreenTransition(this);
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x59B1B0)(fe_state_manager);

		return;
	}
	else if (state == nfsc::world_map_state::click_tp_gps)
	{
		if (nfsc::GPS_Engage(&g::world_map::location, 0.0, false))
		{
			nfsc::Vector3 position = { g::world_map::location.z, -g::world_map::location.x, g::world_map::location.y };
			auto icon = nfsc::GManager_AllocIcon(ReadMemory<uintptr_t>(0xA98294), 0x15, &position, 0, false);

			// Set flag to ShowOnSpawn
			//WriteMemory<uint8_t>(icon_addr + 1, 0x40);

			// Set flag to ShowInWorld + ShowOnMap
			WriteMemory<uint8_t>(icon + 0x1, 3);

			// Set color to white
			WriteMemory<uint32_t>(icon + 0x20, g::world_map::gps_color);

			// Set tex hash
			WriteMemory<uint32_t>(icon + 0x24, nfsc::bStringHash("MINIMAP_ICON_EVENT"));

			nfsc::GIcon_Spawn(icon);
			nfsc::WorldMap_SetGPSIng(icon);

			// Set flag to previous + Spawned + Enabled + GPSing
			WriteMemory<uint8_t>(icon + 1, 0x8F);
		}

		// this->mNextManager = this->mParentManager;
		WriteMemory<uintptr_t>(fe_state_manager + 0xB4, ReadMemory<uintptr_t>(fe_state_manager + 0xAC));

		// this->mExitPoint = 2;
		WriteMemory<int>(fe_state_manager + 0xC, 2);

		// this->mSubState = 3;
		WriteMemory<int>(fe_state_manager + 0x18, 3);

		// FEStateManager::ProcessScreenTransition(this);
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x59B1B0)(fe_state_manager);

		return;
	}

	WorldMapStateChange(fe_state_manager);
}

void __fastcall hooks::WorldMapScreenTickHook(uintptr_t fe_state_manager)
{
	nfsc::BulbToys_UpdateWorldMapCursor(fe_state_manager);

	WorldMapScreenTick(fe_state_manager);
}

// The "2" key
void __fastcall hooks::WorldMapButton4Hook(uintptr_t fe_state_manager)
{
	// Correctly set FEManager's event key if we press the "Select wingman" button when interacting with an event's engagement ring
	if (g::wrong_warp_fix::enabled && ReadMemory<nfsc::world_map_state>(fe_state_manager + 4) == nfsc::world_map_state::engage_event)
	{
		// FEManager::mInstance->mEventKey = WorldMap::GetEventHash(WorldMap::mInstance);
		WriteMemory<uint32_t>(ReadMemory<uintptr_t>(0xA97A7C) + 0xEC,
			reinterpret_cast<uint32_t(__thiscall*)(uintptr_t)>(0x58FDC0)(ReadMemory<uintptr_t>(nfsc::WorldMap)));
	}

	WorldMapButton4(fe_state_manager);
}

void __fastcall hooks::WorldMapShowDialogHook(uintptr_t fe_state_manager)
{	
	const char* COMMON_CANCEL = nfsc::GetLocalizedString(0x1A294DAD);
	const char* COMMON_OK = nfsc::GetLocalizedString(0x417B2601);	
	const char* DIALOG_MSG_ACTIVATE_GPS = nfsc::GetLocalizedString(0x6EB0EACE);

	// mCurrentState
	auto type = ReadMemory<nfsc::world_map_state>(fe_state_manager + 4);

	if (type == nfsc::world_map_state::click_tp)
	{
		WriteMemory<bool>(0xA97B38, false);

		char title[64];
		if (isnan(g::world_map::location.y))
		{
			sprintf_s(title, 64, "Selected coordinates: (%.2f, N/A, %.2f)", g::world_map::location.x, g::world_map::location.z);

			nfsc::FEDialogScreen_ShowDialog(title, COMMON_OK, nullptr, nullptr);
		}
		else
		{
			sprintf_s(title, 64, "Selected coordinates: (%.2f, %.2f, %.2f)", g::world_map::location.x, g::world_map::location.y, g::world_map::location.z);

			nfsc::FEDialogScreen_ShowDialog(title, "Jump to Location", DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL);
		}

		return;
	}

	// Only offer GPS if the option is enabled and we're not in FE
	if (g::world_map::gps_only && *nfsc::GameFlowManager_State == nfsc::gameflow_state::racing)
	{
		if (type == nfsc::world_map_state::race_event)
		{
			// Unhide the cursor and show the dialog with its respective string
			WriteMemory<bool>(0xA97B38, false);

			// DIALOG_MSG_WORLDMAP_EVENT
			nfsc::FEDialogScreen_ShowDialog(nfsc::GetLocalizedString(0xCF93709B), DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL, nullptr);
			return;
		}
		else if (type == nfsc::world_map_state::car_lot)
		{
			WriteMemory<bool>(0xA97B38, false);
			
			// DIALOG_MSG_WORLDMAP_CARLOT
			nfsc::FEDialogScreen_ShowDialog(nfsc::GetLocalizedString(0xBBE2483E), DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL, nullptr);

			return;
		}
		else if (type == nfsc::world_map_state::safehouse)
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

__declspec(naked) void hooks::PTagBustedHook()
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

__declspec(naked) void hooks::DebugActionDropCarHook()
{
	__asm
	{
		call    nfsc::BulbToys_DebugActionDropCar

		// Return to the end of the case instead of where we left off
		push    0x49321D
		ret
	}
}

__declspec(naked) void hooks::NoWingmanSoundHook()
{
	__asm
	{
		// Check if null, gtfo if it is to avoid a null dereference
		test    eax, eax
		jz      skip

		// Redo what we've overwritten
		mov     [esi + 0xDC], eax
		push    0x79391A
		ret

	skip:
		push    0x7939AB
		ret
	}
}
