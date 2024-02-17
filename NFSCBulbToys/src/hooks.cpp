#include "shared.h"

MH_STATUS Hooks::CreateHook(void* address, void* hook, void* call = nullptr)
{
	ASSERT(address != nullptr);

	auto status = MH_CreateHook(reinterpret_cast<void*>(address), hook, reinterpret_cast<void**>(call));
	if (status != MH_OK)
	{
		return status;
	}
	return MH_EnableHook(reinterpret_cast<void*>(address));
}

MH_STATUS Hooks::CreateHook(uintptr_t address, void* hook, void* call = nullptr)
{
	return Hooks::CreateHook(reinterpret_cast<void*>(address), hook, call);
}

//#define CREATE_HOOK(name) { bool func_not_virtual = name != VIRTUAL; ASSERT(func_not_virtual); } CreateHook(name, &name##_##, &name)
#define CREATE_HOOK(name) CreateHook(name, &name##_##, &name)

void Hooks::CreateVTablePatch(uintptr_t vtbl_func_addr, void* hook, void* call = nullptr)
{
	if (call)
	{
		*reinterpret_cast<void**>(call) = Read<void*>(vtbl_func_addr);
	}
	Patch<void*>(vtbl_func_addr, hook);
}

#define CREATE_VTABLE_PATCH(addr, name) CreateVTablePatch(addr, &name##_##, &name);

bool Hooks::Setup()
{
	auto status = MH_Initialize();
	if (status != MH_OK)
	{
		Error("Unable to initialize MinHook: %s", MH_StatusToString(status));
		return false;
	}

	// Check if there's a device in case we late-loaded (via injection instead of an ASI loader, for example)
	// Otherwise, assume it wasn't created yet and wait
	auto device = Read<uintptr_t>(NFSC::Direct3DDevice9);
	if (device)
	{
		return SetupPart2(device);
	}
	else
	{
		auto status = CREATE_HOOK(DirectX_Init);
		if (status != MH_OK)
		{
			Error("Failed to hook DirectX_Init(): %s.", MH_StatusToString(status));
			return false;
		}
	}

	return true;
}

bool Hooks::SetupPart2(uintptr_t device)
{
	GUI::SetupMenu(reinterpret_cast<IDirect3DDevice9*>(device));

	// todo vtable patching doesn't work for this?
	CreateHook(Virtual<42>(device), &ID3DDevice9_EndScene_, &ID3DDevice9_EndScene);
	CreateHook(Virtual<16>(device), &ID3DDevice9_Reset_, &ID3DDevice9_Reset);

	/* Non-critical hooks go here */

	// Optionally override encounter spawn requirement
	CREATE_HOOK(AITrafficManager_NeedsEncounter);

	// Optionally override traffic spawn requirement
	CREATE_HOOK(AITrafficManager_NeedsTraffic);

	// Optionally override whether racers should be pursued or not
	CREATE_VTABLE_PATCH(0x9C3B58, AICopManager_CanPursueRacers);

	// Smart AI hooks
	// - Make autopilot drive to the location marked by the GPS
	// - Make autopilot drive to the location after a navigation reset
	CREATE_HOOK(Gps_Engage);
	CREATE_VTABLE_PATCH(0x9C517C, AIVehicleHuman_ResetDriveToNav); // AIVehicleHuman #21

	// Calculate world positions from map positions
	// Disable snapping to world map icons if we're holding shift
	// GPS only mode
	// Wrong warp fix
	CREATE_VTABLE_PATCH(0x9D2E88, FEWorldMapStateManager_HandlePadAccept);
	//CreateHook(0x5C3330, &WorldMapSnapHook, &WorldMapSnap);
	CREATE_VTABLE_PATCH(0x9D2F1C, FEWorldMapStateManager_HandleShowDialog);
	CREATE_VTABLE_PATCH(0x9D2E38, FEWorldMapStateManager_HandleButtonPressed);
	CREATE_VTABLE_PATCH(0x9D2F24, FEWorldMapStateManager_HandleStateChange);
	CREATE_VTABLE_PATCH(0x9D2F14, FEWorldMapStateManager_HandleScreenTick);
	CREATE_VTABLE_PATCH(0x9D2EA4, FEWorldMapStateManager_HandlePadButton4);

	// Create player (Sim::Entity, IPlayer) instances for AI if we're playing PTag/PKO, necessary for vehicle switching to work
	CREATE_HOOK(GRacerInfo_CreateVehicle);

	// The original function checks GKnockoutRacer::mPursuitTier, which is always 0 and gives us a destroyable copmidsize every time
	// Use our own hook to assign pursuit vehicles based on the local player (us) vehicle's car's tier, exactly how the game intended in the code
	CREATE_HOOK(GKnockoutRacer_GetPursuitVehicleName);

	// For PTag, check if the racer's 60 second timer has elapsed and switch vehicles accordingly
	CREATE_HOOK(GRaceStatus_Update);

	// For testing purposes
	//CreateHook(0x65D620, &PursuitSwitchHook, &PursuitSwitch);

	// Make sure it takes the Quick Race timelimit (race lap) setting into consideration, instead of that particular race's attributes (always 2 minutes iirc)
	CREATE_HOOK(GRaceParameters_GetTimeLimit);

	// Prevent double race end screen softlock
	CREATE_HOOK(FE_ShowLosingPostRaceScreen);
	CREATE_HOOK(FE_ShowWinningPostRaceScreen);

	// Instead of resuming career, reload the Career menu if we load a save (or if we create a new one (Patches::MemcardManagement))
	CREATE_VTABLE_PATCH(0x9D26BC, FECareerStateManager_HandleChildFlowDone);

	// Custom encounter vehicles
	CREATE_HOOK(AITrafficManager_GetAvailablePresetVehicle);

	// Replace "Dump Preset" with "Add to My Cars" for DebugCarCustomize
	CREATE_VTABLE_PATCH(0x9FB518, FEDebugCarStateManager_HandlePadButton3);

	// Reset GRaceStatus vehicle count to 0 when the race ends
	CREATE_HOOK(GRaceStatus_SetRoaming);

	// Add health icons above vehicles
	CREATE_HOOK(CarRenderConn_UpdateIcon);

	// FOV overrides
	CREATE_HOOK(Camera_SetCameraMatrix);

	// Make Neville a buggy "boss" if he's racing cuz it's funny lol
	CREATE_HOOK(GRaceStatus_GetMainBoss);
	
	// Add racers and GPS icon to the world map
	CREATE_HOOK(WorldMap_AddPlayerCar);

	// PIP tests
	CREATE_HOOK(MLaunchPIP_MLaunchPIP);

	// Prevent "encounter bleeding" - spawning encounters into the void endlessly
	CREATE_HOOK(AITrafficManager_SpawnEncounter);

	// Custom SMS test 1 - DAL getters
	CREATE_HOOK(DALCareer_GetSMSHandle);
	CREATE_HOOK(DALCareer_GetSMSIsAvailable);
	CREATE_HOOK(DALCareer_GetSMSWasRead);
	CREATE_HOOK(DALCareer_GetSMSIsTip);
	CREATE_HOOK(DALCareer_GetSMSSortOrder);
	CREATE_HOOK(DALCareer_GetSMSHashMessage);
	CREATE_HOOK(DALCareer_GetSMSIsVoice);

	// Custom SMS part 2 - DAL setters
	CREATE_HOOK(DALCareer_SetSMSWasRead);
	CREATE_HOOK(DALCareer_SetSMSIsAvailable);
	CREATE_HOOK(DALCareer_SetSMSHandle);

	// Custom SMS test 2 - FE rendering
	CREATE_HOOK(FESMSMessage_RefreshHeader);
	CREATE_HOOK(CTextScroller_SetTextHash);
	CREATE_HOOK(SMSSlot_Update);

	// Spectator HUD
	CREATE_HOOK(DALVehicle_GetIVehicle);
	CREATE_VTABLE_PATCH(0x9D6C50 + 0x4, Tachometer_Update);
	CREATE_VTABLE_PATCH(0x9D6A64 + 0x4, NitrousGauge_Update);
	CREATE_VTABLE_PATCH(0x9D6ADC + 0x4, SpeedbreakerMeter_Update);

	// TODO: FLM stuff
	//CREATE_HOOK(DALWorldMap_GetBool);

	CREATE_HOOK(cFEngRender_RenderTerritoryBorder);

	//CreateHook(0x5D23E0, &UpdateRaceRouteHook, &UpdateRaceRoute);

	//CreateHook(0x5E5050, &MinimapDestructorHook, &MinimapDestructor);
	//CREATE_HOOK(Minimap_dtor);

	//CreateHook(0x7598E0, &FLMMoveMaybeHook, &FLMMoveMaybe);
	//CREATE_HOOK(FatLineMesh_AddBezier);

	// Override the chosen roadblock with our own when manually spawning roadblocks
	// TODO: uncomment when i've unfucked roadblock creation (might even be useless)
	//CreateHook(0x407040, &PickRoadblockSetupHook, &PickRoadblockSetup);

	// Adds a "Customize" button to Photo Mode when in the Quick Race car selection screen, for vehicles in the My Cars garage only
	CREATE_VTABLE_PATCH(0x9D2FB4, FEPhotoModeStateManager_Start);
	CREATE_VTABLE_PATCH(0x9D3010, FEPhotoModeStateManager_HandlePadAccept);
	CREATE_VTABLE_PATCH(0x9D2FCC, FEPhotoModeStateManager_HandleChildFlowDone)

	// Fixes an issue where your crew member's car will not render in Photo Mode if you've last used Photo Mode outside FE/in the world
	CREATE_VTABLE_PATCH(0x9F8344, FECrewManagementStateManager_HandleOptionSelected);

	// In drift races, give the player racer a default "Player" name if they create a nameless alias, exactly as other races do it
	CREATE_HOOK(DriftScoring_AddRacer);

	// Increment cop counter by 1 per roadblock vehicle
	// TODO: if re-enabling this, make sure roadblock cops that get attached don't increment again
	//WriteJmp(0x445A9D, CreateRoadBlockHook, 6);

	// Fix dead roadblock vehicles not turning off their lights
	PatchJMP(0x5D8C10, UpdateCopElementsHook1, 6);
	PatchJMP(0x5D8CFB, UpdateCopElementsHook2, 11);

	// Attach roadblock cops to the pursuit once it's been dodged
	// TODO: ideally detach from roadblock and destroy said roadblock afterwards because of several bugs
	// - roadblock crash cam nag
	// - no new roadblocks will spawn until the cops have been killed
	// - when the roadblock gets destroyed (ie. last cop dies), all cops and their corpses instantly despawn
	//WriteJmp(0x4410D4, UpdateRoadBlocksHook, 6);
	//WriteJmp(0x4411AD, UpdateRoadBlocksHook, 6);

	// Add ability to increase vinyl move step size to move vinyls faster
	PatchJMP(0x7B0F63, MoveVinylVerticalHook, 9);
	PatchJMP(0x7B0F94, MoveVinylHorizontalHook, 9);

	// Use "GRaceStatus" as vehicle cache for NFSCO compatibility
	PatchJMP(0x7D4EBB, VehicleChangeCacheHook, 7);

	// Add AI Players to Player list
	PatchJMP(0x6D40A6, UpdateAIPlayerListingHook, 5);

	// Check who got busted in pursuit tag and switch vehicles accordingly
	PatchJMP(0x44A596, PTagBustedHook, 5);

	// Better debug cam teleport (now properly on ground and takes forward vector into consideration)
	PatchJMP(0x49319E, DebugActionDropCarHook);

	// Fix crash when spawning a wingman lacking speech
	PatchJMP(0x793914, NoWingmanSoundHook, 6);

	//PatchJMP(0x5CD975, NoIconsWorldMapHook, 6);
	
	return true;
}

void Hooks::Destroy()
{
	// TODO move to patches
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

HRESULT __cdecl Hooks::DirectX_Init_()
{
	const auto result = Hooks::DirectX_Init();
	if (FAILED(result))
	{
		return result;
	}

	MH_DisableHook(reinterpret_cast<void*>(0x710220));
	MH_RemoveHook(reinterpret_cast<void*>(0x710220));

	// The initial setup has completed and thus the thread is operational
	// Force it to exit in case our final setup fails
	exitMainLoop = !SetupPart2(Read<uintptr_t>(NFSC::Direct3DDevice9));

	return result;
}

HRESULT __stdcall Hooks::ID3DDevice9_EndScene_(IDirect3DDevice9* device)
{
	const auto result = Hooks::ID3DDevice9_EndScene(device);

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//static uint32_t frame_count[2] {0, 0};

	//frame_count[0] = frame_count[1];
	//frame_count[1] = Read<uint32_t>(0xA996F0); // RealLoopCounter

	// Certain loading screens render the game twice, in which case we shouldn't render the GUI
	// TODO: uncomment when i crash here ig lol (window will be null)
	//if (frame_count[0] != frame_count[1])

	GUI::Render();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return result;
}

HRESULT __stdcall Hooks::ID3DDevice9_Reset_(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = Hooks::ID3DDevice9_Reset(device, params);
	ImGui_ImplDX9_CreateDeviceObjects();
	return result;
}

bool __fastcall Hooks::AITrafficManager_NeedsEncounter_(uintptr_t traffic_manager)
{
	// If we've overridden the value, use our own. Instead, let the game decide normally
	return g::needs_encounter::overridden ? g::needs_encounter::value : Hooks::AITrafficManager_NeedsEncounter(traffic_manager);
}

bool __fastcall Hooks::AITrafficManager_NeedsTraffic_(uintptr_t traffic_manager)
{
	// Ditto
	return g::needs_traffic::overridden ? g::needs_traffic::value : Hooks::AITrafficManager_NeedsTraffic(traffic_manager);
}

bool __fastcall Hooks::AICopManager_CanPursueRacers_(uintptr_t ai_cop_manager)
{
	// Ditto
	return g::pursue_racers::overridden ? g::pursue_racers::value : Hooks::AICopManager_CanPursueRacers(ai_cop_manager);
}

bool __fastcall Hooks::Gps_Engage_(uintptr_t gps, uintptr_t edx, NFSC::Vector3* target, float max_deviation, bool re_engage, bool always_re_establish)
{
	bool result = Hooks::Gps_Engage(gps, edx, target, max_deviation, re_engage, always_re_establish);

	// GPS failed to establish
	if (!result)
	{
		return result;
	}

	uintptr_t my_vehicle = 0;
	NFSC::BulbToys_GetMyVehicle(&my_vehicle, nullptr);
	if (!my_vehicle)
	{
		return result;
	}

	auto ai_vehicle = NFSC::PVehicle_GetAIVehiclePtr(my_vehicle);
	if (!ai_vehicle)
	{
		return result;
	}

	g::smart_ai::target.x = target->x;
	g::smart_ai::target.y = target->y;
	g::smart_ai::target.z = target->z;
	NFSC::BulbToys_PathToTarget(ai_vehicle, &g::smart_ai::target);

	return result;
}

void __fastcall Hooks::AIVehicleHuman_ResetDriveToNav_(uintptr_t ai_vehicle, uintptr_t edx, int lane_selection)
{
	Hooks::AIVehicleHuman_ResetDriveToNav(ai_vehicle, edx, lane_selection);

	if (!NFSC::GPS_IsEngaged())
	{
		return;
	}

	/*
	uintptr_t my_vehicle = 0;
	NFSC::BulbToys_GetMyVehicle(&my_vehicle, nullptr);
	if (!my_vehicle)
	{
		return;
	}

	auto local_ai_vehicle = NFSC::PVehicle_GetAIVehiclePtr(my_vehicle);
	if (!local_ai_vehicle)
	{
		return;
	}

	// Only do it for our vehicle
	if (local_ai_vehicle != ai_vehicle)
	{
		return;
	}
	*/

	NFSC::BulbToys_PathToTarget(ai_vehicle, &g::smart_ai::target);
}

/*
uintptr_t __fastcall Hooks::RacerInfoCreateVehicleHook(uintptr_t racer_info, uintptr_t edx, uint32_t key, int racer_index, uint32_t seed)
{
	NFSC::race_type type = NFSC::BulbToys_GetRaceType();

	if (type == NFSC::race_type::pursuit_ko)
	{
		uintptr_t stored_vehicle = RacerInfoCreateVehicle(racer_info, edx, NFSC::GKnockoutRacer_GetPursuitVehicleKey(0), racer_index, seed);
		Error("%p", stored_vehicle);
		NFSC::PVehicle_SetDriverClass(stored_vehicle, NFSC::driver_class::none);
		NFSC::PVehicle_Deactivate(stored_vehicle);
		Write<uintptr_t>(NFSC::ThePursuitSimables + 4 * racer_index, NFSC::PVehicle_GetSimable(stored_vehicle));

		return RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
	}
	else if (type == NFSC::race_type::pursuit_tag)
	{
		if (racer_index == 1)
		{
			uintptr_t player_pursuit_simable = NFSC::BulbToys_CreatePursuitSimable(NFSC::driver_class::none);
			Write<uintptr_t>(NFSC::ThePursuitSimables, player_pursuit_simable);
			NFSC::PVehicle_Deactivate(NFSC::BulbToys_FindInterface<NFSC::IVehicle>(player_pursuit_simable));
		}

		uintptr_t stored_vehicle = RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
		NFSC::PVehicle_SetDriverClass(stored_vehicle, NFSC::driver_class::none);
		NFSC::PVehicle_Deactivate(stored_vehicle);
		Write<uintptr_t>(NFSC::ThePursuitSimables + 4 * racer_index, NFSC::PVehicle_GetSimable(stored_vehicle));

		return RacerInfoCreateVehicle(racer_info, edx, NFSC::GKnockoutRacer_GetPursuitVehicleKey(0), racer_index, seed);
	}

	return RacerInfoCreateVehicle(racer_info, edx, key, racer_index, seed);
}
*/

uintptr_t __fastcall Hooks::GRacerInfo_CreateVehicle_(uintptr_t racer_info, uintptr_t edx, uint32_t key, int racer_index, uint32_t seed)
{
	uintptr_t vehicle = 0;

	auto type = NFSC::BulbToys_GetRaceType();

	// Only create players/entities (using our in-house AIPlayer class) for racers if we're playing PKO/PTAG, since they're not really needed elsewhere
	// NFSC uses player/entity related interfaces for attaching vehicles to (and detaching vehicles from) them
	// Since AI does not implement any player/entity related interfaces, we must make our own instead
	if (type == NFSC::GRaceType::PURSUIT_KO || type == NFSC::GRaceType::PURSUIT_TAG)
	{
		// Which tier do we use?
		uintptr_t my_vehicle = 0;
		NFSC::BulbToys_GetMyVehicle(&my_vehicle, nullptr);

		int tier = NFSC::BulbToys_GetVehicleTier(my_vehicle);
		if (tier < 1 || tier > 3)
		{
			tier = 1;
		}
		NFSC::player_cop_tier = tier - 1;

		// For PTAG, AI racers start as cops, so create our pursuit simables first (so the vehicle start grid warping works properly)
		if (type == NFSC::GRaceType::PURSUIT_TAG)
		{
			// Online gamemodes skip the intro NIS, but the main reason is because there is no suitable NIS ever made for PTAG's custom start grid positions
			//*NFSC::SkipNIS = true;

			// Create our pursuit simables first (so the vehicle start grid warping works properly)
			if (racer_index == 1)
			{
				// Create AI pursuit simables first
				for (int i = 1; i < NFSC::GRaceStatus_GetRacerCount(Read<uintptr_t>(NFSC::GRaceStatus)); i++)
				{
					uintptr_t pursuit_simable = NFSC::BulbToys_CreatePursuitSimable();
					Write<uintptr_t>(NFSC::ThePursuitSimables + 4 * i, pursuit_simable);

					uintptr_t pursuit_vehicle = NFSC::BulbToys_FindInterface<NFSC::IVehicle>(pursuit_simable);
					NFSC::PVehicle_Deactivate(pursuit_vehicle);
				}

				// Then our own pursuit simable
				uintptr_t pursuit_simable = NFSC::BulbToys_CreatePursuitSimable();
				Write<uintptr_t>(NFSC::ThePursuitSimables, pursuit_simable);

				uintptr_t pursuit_vehicle = NFSC::BulbToys_FindInterface<NFSC::IVehicle>(pursuit_simable);
				NFSC::PVehicle_Deactivate(pursuit_vehicle);
			}

			// We need to create our racer vehicle and player before switching first
			uintptr_t racer_vehicle = Hooks::GRacerInfo_CreateVehicle(racer_info, edx, key, racer_index, seed);
			uintptr_t racer_simable = NFSC::PVehicle_GetSimable(racer_vehicle);

			NFSC::AIPlayer* ai_player = NFSC::AIPlayer::New();

			// bool (PhysicsObject) ISimable::Attach(ISimable*, IPlayer*)
			NFSC::PhysicsObject_Attach(racer_simable, reinterpret_cast<uintptr_t>(&ai_player->IPlayer));
			
			// In PTAG, opponents start as cops first
			int _;
			vehicle = NFSC::Game_PursuitSwitch(racer_index, true, &_);

			// Copy pursuit simable's handle into our racer info
			uintptr_t simable = NFSC::PVehicle_GetSimable(vehicle);
		}

		// For PKO, AI racers start as racers, so create our racer vehicles/simables first (so the vehicle start grid warping works properly)
		else if (type == NFSC::GRaceType::PURSUIT_KO)
		{
			// Online gamemodes skip the intro NIS, but it's fine here since it starts off as a normal circuit race and thus the normal intro NIS fits
			//*NFSC::SkipNIS = false;

			// Create and use our own racer vehicle and player
			vehicle = Hooks::GRacerInfo_CreateVehicle(racer_info, edx, key, racer_index, seed);

			NFSC::AIPlayer* ai_player = NFSC::AIPlayer::New();

			NFSC::PhysicsObject_Attach(NFSC::PVehicle_GetSimable(vehicle), reinterpret_cast<uintptr_t>(&ai_player->IPlayer));

			int racer_count = NFSC::GRaceStatus_GetRacerCount(Read<uintptr_t>(NFSC::GRaceStatus));

			// Final racer's vehicle was created, now we can create our pursuit simables
			// If we created one per racer in this function, the start grid vehicles would get fucked
			if (racer_index == racer_count - 1)
			{
				for (int i = 0; i < racer_count; i++)
				{
					uintptr_t pursuit_simable = NFSC::BulbToys_CreatePursuitSimable();
					
					// Store and deactivate
					Write<uintptr_t>(NFSC::ThePursuitSimables + 4 * i, pursuit_simable);

					uintptr_t pursuit_vehicle = NFSC::BulbToys_FindInterface<NFSC::IVehicle>(pursuit_simable);
					NFSC::PVehicle_Deactivate(pursuit_vehicle);
				}
			}
		}
	}
	else // Not a PKO/PTAG race
	{
		//*NFSC::SkipNIS = false;
		vehicle = Hooks::GRacerInfo_CreateVehicle(racer_info, edx, key, racer_index, seed);
	}

	return vehicle;
}

const char* __cdecl Hooks::GKnockoutRacer_GetPursuitVehicleName_(bool is_player)
{
	return NFSC::player_cop_cars[NFSC::player_cop_tier];
}

void __fastcall Hooks::GRaceStatus_Update_(uintptr_t race_status, uintptr_t edx, float dt)
{
	Hooks::GRaceStatus_Update(race_status, edx, dt);

	auto type = NFSC::BulbToys_GetRaceType();

	if (type == NFSC::GRaceType::PURSUIT_TAG)
	{
		int runner_index = -1;
		uintptr_t runner_simable = NFSC::GRaceStatus_GetRacePursuitTarget(race_status, &runner_index);

		// FIXME: AI does not pursue you under any of the following conditions
		// - Goal set to AIGoalPursuit, AIGoalHassle, AIGoalPursuitEncounter, AIGoalRam, AIGoalPit; combined with Game_SetPursuitTarget
		// - AIVehicle set to CopCar or Pursuit (using BEHAVIOR_MECHANIC_AI and committing)
		// - AIAction manipulation, particularly removing AIActionRace (necessary for all non-traffic vehicles as it's directly responsible for their driving)
		// - Setting DriverClass to cop (game crashes by stack overflow in ExtrapolatedRacer dtor ?????)
		// - Probably a couple other things i forgot to mention here
		// As a temporary horrible hack, we path to the pursuee every race update. This is very inaccurate and the AI does not behave properly
		uintptr_t runner_rigidbody = NFSC::PhysicsObject_GetRigidBody(runner_simable);
		NFSC::Vector3* runner_pos = NFSC::RigidBody_GetPosition(runner_rigidbody);

		for (int i = 0; i < NFSC::GRaceStatus_GetRacerCount(race_status); i++)
		{
			if (i == runner_index)
			{
				continue;
			}

			uintptr_t racer_info = NFSC::GRaceStatus_GetRacerInfo(race_status, i);
			uintptr_t simable = NFSC::GRacerInfo_GetSimable(racer_info);
			uintptr_t vehicle = NFSC::BulbToys_FindInterface<NFSC::IVehicle>(simable);
			uintptr_t ai_vehicle = NFSC::PVehicle_GetAIVehiclePtr(vehicle);
			NFSC::BulbToys_PathToTarget(ai_vehicle, runner_pos);
		}

		// Check the racer's 60 second fleeing timer. If we've run out, switch to our nearest cop

		float runner_timer = Read<float>(race_status + 0xBFB0);
		if (runner_timer <= 0)
		{
			NFSC::BulbToys_SwitchPTagTarget(race_status, false);
		}
	}
}

/*
uintptr_t __cdecl Hooks::PursuitSwitchHook(int racer_index, bool is_busted, int* result)
{
	uintptr_t vehicle = PursuitSwitch(racer_index, is_busted, result);

	uintptr_t simable = NFSC::PVehicle_GetSimable(vehicle);

	uintptr_t player = reinterpret_cast<uintptr_t (__thiscall*)(uintptr_t)>(0x6D6C40)(simable);

	Error("SWITCH!\n\nIndex: %d\nBusted: %s\nResult: %d\n\nVehicle: %p\nPlayer: %p", racer_index, is_busted? "true" : "false", *result, vehicle, player);

	return vehicle;
}
*/

float __fastcall Hooks::GRaceParameters_GetTimeLimit_(uintptr_t race_parameters)
{
	if (NFSC::BulbToys_GetRaceType() == NFSC::GRaceType::PURSUIT_TAG)
	{
		uintptr_t user_profile = NFSC::FEManager_GetUserProfile(Read<uintptr_t>(NFSC::FEManager), 0);

		// user_profile->mRaceSettings[11 (== PTag)].lap_count;
		int num_laps = Read<uint8_t>(user_profile + 0x2B258);

		// Time limit is in seconds, so we multiply by 60 to get minutes
		return static_cast<float>(num_laps * 60);
	}

	return Hooks::GRaceParameters_GetTimeLimit(race_parameters);
}

void __cdecl Hooks::FE_ShowLosingPostRaceScreen_()
{
	auto type = NFSC::BulbToys_GetRaceType();

	// PTag appears to utilize FE_ShowLosingPostRaceScreen, which calls FE_ShowPostRaceScreen
	// But FE_ShowPostRaceScreen also gets called in Game_EnterPostRaceFlow
	// To prevent the infamous double end screen softlock, we're blocking this screen for the former function (this hook)
	if (type != NFSC::GRaceType::PURSUIT_TAG && type != NFSC::GRaceType::PURSUIT_KO)
	{
		Hooks::FE_ShowLosingPostRaceScreen();
	}
}

void __cdecl Hooks::FE_ShowWinningPostRaceScreen_()
{
	auto type = NFSC::BulbToys_GetRaceType();

	// Ditto, but for FE_ShowWinningPostRaceScreen
	if (type != NFSC::GRaceType::PURSUIT_TAG && type != NFSC::GRaceType::PURSUIT_KO)
	{
		Hooks::FE_ShowWinningPostRaceScreen();
	}
}

void __fastcall Hooks::FECareerStateManager_HandleChildFlowDone_(uintptr_t fe_career_state_manager, uintptr_t edx, int unk)
{
	int cur_state = Read<uintptr_t>(fe_career_state_manager + 4);

	if (cur_state == 15)
	{
		NFSC::FEStateManager_Switch(fe_career_state_manager, "FeMainMenu_Sub.fng", 0x93E8A57C, 1, 1);
	}
	else
	{
		Hooks::FECareerStateManager_HandleChildFlowDone(fe_career_state_manager, edx, unk);
	}
}

uintptr_t __fastcall Hooks::AITrafficManager_GetAvailablePresetVehicle_(uintptr_t ai_traffic_manager, uintptr_t edx, uint32_t skin_key, uint32_t encounter_key)
{
	if (g::encounter::overridden)
	{
		skin_key = 0;
		encounter_key = NFSC::Attrib_StringToKey(g::encounter::vehicle);
	}

	return Hooks::AITrafficManager_GetAvailablePresetVehicle(ai_traffic_manager, edx, skin_key, encounter_key);
}

void __fastcall Hooks::FEDebugCarStateManager_HandlePadButton3_(uintptr_t fe_debugcar_state_manager)
{
	uint32_t index = 0;
	NFSC::DALCareer_GetPodiumVehicle(&index);
	if (NFSC::DALFeVehicle_AddCarToMyCarsDB(index))
	{
		NFSC::FE_ShowChyron("Added to My Cars.", NFSC::Chyron::MAIL, false);
	}
	else
	{
		NFSC::FE_ShowChyron("Invalid vehicle or garage is full.", NFSC::Chyron::MAIL, false);
	}
}

void __fastcall Hooks::GRaceStatus_SetRoaming_(uintptr_t g_race_status)
{
	Hooks::GRaceStatus_SetRoaming(g_race_status);

	Write<uint32_t>(g_race_status + 0x6A08, 0);
}

void __fastcall Hooks::CarRenderConn_UpdateIcon_(uintptr_t car_render_conn, uintptr_t edx, uintptr_t pkt)
{
	Hooks::CarRenderConn_UpdateIcon(car_render_conn, edx, pkt);

	if (!g::health_icon::show)
	{
		return;
	}

	// Don't do anything if we have an icon already
	if (Read<uintptr_t>(car_render_conn + 0x1AC) != 0)
	{
		return;
	}

	uint32_t world_id = Read<uint32_t>(car_render_conn + 0x2C);

	uintptr_t this_vehicle = 0;
	for (size_t i = 0; i < NFSC::VehicleList[NFSC::VLType::AI_COPS]->size; i++)
	{
		uintptr_t vehicle = NFSC::VehicleList[NFSC::VLType::AI_COPS]->begin[i];
		uintptr_t simable = NFSC::PVehicle_GetSimable(vehicle);

		uint32_t simable_wid = NFSC::PhysicsObject_GetWorldID(simable);

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
	uintptr_t i_damageable = Read<uintptr_t>(this_vehicle + 0x44);

	// Set icon
	constexpr uint32_t INGAME_ICON_PLAYERCAR = 0x3E9CCFFA;
	Write<uintptr_t>(car_render_conn + 0x1AC, NFSC::GetTextureInfo(INGAME_ICON_PLAYERCAR, 1, 0));

	// Set scale
	Write<float>(car_render_conn + 0x1B4, 0.5f);

	float health = NFSC::DamageVehicle_GetHealth(i_damageable);

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
	Write<color_>(car_render_conn + 0x1B0, color);
}

void __fastcall Hooks::Camera_SetCameraMatrix_(uintptr_t camera, uintptr_t edx, void* matrix4, float dt)
{
	uintptr_t fov = camera + 0xE4;
	if (camera == g::fov::player)
	{
		if (g::fov::player_override)
		{
			Write<int>(fov, g::fov::player_fov);
			Hooks::Camera_SetCameraMatrix(camera, edx, matrix4, dt);
		}
		else
		{
			Hooks::Camera_SetCameraMatrix(camera, edx, matrix4, dt);
			g::fov::player_fov = Read<int>(fov);
		}
	}
	else if (camera == g::fov::rvm)
	{
		if (g::fov::rvm_override)
		{
			Write<int>(fov, g::fov::rvm_fov);
			Hooks::Camera_SetCameraMatrix(camera, edx, matrix4, dt);
		}
		else
		{
			Hooks::Camera_SetCameraMatrix(camera, edx, matrix4, dt);
			g::fov::rvm_fov = Read<int>(fov);
		}
	}
	else if (camera == g::fov::pip)
	{
		if (g::fov::pip_override)
		{
			Write<int>(fov, g::fov::pip_fov);
			Hooks::Camera_SetCameraMatrix(camera, edx, matrix4, dt);
		}
		else
		{
			Hooks::Camera_SetCameraMatrix(camera, edx, matrix4, dt);
			g::fov::pip_fov = Read<int>(fov);
		}
	}
	else
	{
		Hooks::Camera_SetCameraMatrix(camera, edx, matrix4, dt);
	}
}

uintptr_t __fastcall Hooks::GRaceStatus_GetMainBoss_(uintptr_t g_race_status)
{
	uintptr_t racer_info = Hooks::GRaceStatus_GetMainBoss(g_race_status);

	if (!racer_info)
	{
		uintptr_t my_simable = 0;
		NFSC::BulbToys_GetMyVehicle(nullptr, &my_simable);
		if (!my_simable)
		{
			return 0;
		}

		uintptr_t wingman_simable = NFSC::Game_GetWingman(my_simable);
		if (!wingman_simable)
		{
			return 0;
		}

		uintptr_t wingman_racer_info = NFSC::GRaceStatus_GetRacerInfo2(g_race_status, wingman_simable);
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
void __fastcall Hooks::WorldMap_AddPlayerCar_(uintptr_t world_map)
{
	// eFE_GAME_MODE_CAREER
	if (NFSC::FEStateManager_IsGameMode(Read<uintptr_t>(NFSC::FEManager), 0))
	{
		return;
	}

	// this->PackageFilename
	auto package_name = Read<char*>(world_map + 0xC);
	if (!package_name)
	{
		return;
	}

	constexpr uint32_t PLAYERCARINDICATOR = 0xDD9EF5FF;
	uintptr_t object = NFSC::FE_Object_FindObject(package_name, PLAYERCARINDICATOR);
	if (!object)
	{
		return;
	}

	// object->Type != FE_Image
	if (Read<int>(object + 0x18) != 1)
	{
		return;
	}

	uintptr_t my_vehicle = 0;
	NFSC::BulbToys_GetMyVehicle(&my_vehicle, nullptr);

	auto list = NFSC::VehicleList[NFSC::VLType::RACERS];
	for (int i = 0; i < (int)list->size; i++)
	{
		uintptr_t new_object = 0;

		auto racer = list->begin[i];
		if (racer == my_vehicle)
		{
			new_object = object;

			NFSC::FEColor cyan = { 255, 255, 115, 255 };
			NFSC::FE_Object_SetColor(new_object, &cyan);
		}
		else
		{
			if (NFSC::WorldMap_IsInPursuit(world_map))
			{
				continue;
			}
			if (!NFSC::PVehicle_IsActive(racer))
			{
				continue;
			}

			// HIDING_SPOT_0 -> HIDING_SPOT_99
			new_object = NFSC::FE_Object_FindObject(package_name, NFSC::FE_String_HashString("HIDING_SPOT_%d", i));

			NFSC::FE_Image_SetTextureHash(new_object, Read<uint32_t>(object + 0x24));

			NFSC::FEColor orange = { 115, 195, 255, 255 };
			NFSC::FE_Object_SetColor(new_object, &orange);
		}

		uintptr_t map_item = NFSC::malloc(0x30);
		if (map_item)
		{
			NFSC::Vector2 position, direction;
			NFSC::GetVehicleVectors(position, direction, NFSC::PVehicle_GetSimable(racer));

			// this->track_map_top_left, this->track_map_size
			NFSC::WorldMap_ConvertPos(position.x, position.y, *reinterpret_cast<NFSC::Vector2*>(world_map + 0x44),
				*reinterpret_cast<NFSC::Vector2*>(world_map + 0x4C));

			float rotation = NFSC::bATan(direction.y, direction.x) * 0.0054931641f /* 360 / 65536 */;

			map_item = NFSC::MapItem_MapItem(map_item, 1, new_object, position, rotation, 0, 0);

			// Add node to map items
			uintptr_t node = map_item + 4;
			uintptr_t prev = Read<uintptr_t>(world_map + 0x64 + 0x4);

			Write<uintptr_t>(prev, node);
			Write<uintptr_t>(world_map + 0x64 + 0x4, node);
			Write<uintptr_t>(node + 4, prev);
			Write<uintptr_t>(node, world_map + 0x64);
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

uintptr_t __fastcall Hooks::MLaunchPIP_MLaunchPIP_(uintptr_t m_launch_pip, uintptr_t edx, int id, uintptr_t simable_handle)
{
	LOG(3, "PIP: %d (%s)", id, pip_names[id - 1]);

	return Hooks::MLaunchPIP_MLaunchPIP(m_launch_pip, edx, id, simable_handle);
}

bool __fastcall Hooks::AITrafficManager_SpawnEncounter_(uintptr_t traffic_manager)
{
	static uintptr_t encounter_vehicle = 0;

	// Save the time since the last encounter spawn for later calculations
	uintptr_t time_since_last_encounter_spawn = traffic_manager + 0x130;
	float time = Read<float>(time_since_last_encounter_spawn);

	//LOG(1, "SpawnEncounter: TSLE = %.2f", time);

	// Tweak_TrafficRandomEncounter
	if (!Read<bool>(0xA4BF80))
	{
		return false;
	}

	// GCareer::mObj && (GCareer::mObj->mPendingAcitivity || *&GCareer::mObj->data_00_20[12]) - checks if we're in a race?
	uintptr_t g_career = Read<uintptr_t>(0xA982B8);
	if (g_career && (Read<uint32_t>(g_career + 0x118) || Read<int>(g_career + 0x110)))
	{
		return false;
	}

	uintptr_t vehicle, simable;
	if (!NFSC::BulbToys_GetMyVehicle(&vehicle, &simable))
	{
		return false;
	}

	uintptr_t vehicle_ai = NFSC::PVehicle_GetAIVehiclePtr(vehicle);
	if (!vehicle_ai)
	{
		return false;
	}

	// AIVehicle::GetPursuit
	uintptr_t pursuit = NFSC::AIVehicle_GetPursuit(vehicle_ai);
	if (pursuit)
	{
		return false;
	}

	// AITrafficManager::NeedsEncounter - returns false if a SPAWNED/ACTIVE racer already exists
	if (!Hooks::AITrafficManager_NeedsEncounter_(traffic_manager))
	{
		return false;
	}

	// AITrafficManager::NextEncounterSpawn - is it time for us to spawn? returns false if next_encounter_key is null (gets generated in this function)
	// next_encounter_key only gets set to null after a successful spawn - it will remain intact should the vehicle still be loading
	if (!reinterpret_cast<bool(__thiscall*)(uintptr_t)>(0x444AC0)(traffic_manager))
	{
		return false;
	}

	/*
	// don't think any of these are necessary lol (function gets called maybe 5 times per second), leaving them here just in case

	// Custom check - again, is it *really* time for us to spawn?
	// This time we don't care if next_encounter_key is set or not, we return false if it's not our time yet
	if (Read<float>(time_since_last_encounter_spawn) > ...)
	{
		return false;
	}

	// Time for us to spawn. If we fail for any reason (other than the vehicle loading), we start over at half our timer (pointless without custom check)
	Write<float>(time_since_last_encounter_spawn, time / 2);
	*/

	NFSC::WRoadNav nav;
	NFSC::WRoadNav_WRoadNav(nav);

	uintptr_t rigid_body = NFSC::PhysicsObject_GetRigidBody(simable);

	NFSC::Vector3 position = *NFSC::RigidBody_GetPosition(rigid_body);
	NFSC::Vector3 fwd_vec;
	NFSC::RigidBody_GetForwardVector(rigid_body, &fwd_vec);

	// Distance when spawning in front of us
	// TODO: is it worth keeping this feature? it kinda sucks lol
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

	NFSC::WRoadNav_InitAtPoint(nav, &position, &fwd_vec, false, 1.0);
	if (!nav.fValid)
	{
		NFSC::WRoadNav_Destructor(nav);
		return false;
	}

	NFSC::WRoadNav_IncNavPosition(nav, distance, &fwd_vec, 0.0, 0);

	NFSC::WCollisionMgr mgr;
	mgr.fPrimitiveMask = 3;
	mgr.fSurfaceExclusionMask = 0;

	float _;
	if (!NFSC::WCollisionMgr_GetWorldHeightAtPointRigorous(mgr, &nav.fPosition, &_, nullptr))
	{
		NFSC::WRoadNav_Destructor(nav);
		return 0;
	}

	fwd_vec.x = -nav.fForwardVector.x;
	fwd_vec.y = -nav.fForwardVector.y;
	fwd_vec.z = -nav.fForwardVector.z;

	// AITrafficManager::GetAvailablePresetVehicle
	encounter_vehicle = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uint32_t, uint32_t)>(0x42CB70)
		(traffic_manager, Read<uint32_t>(traffic_manager + 0x120), Read<uint32_t>(traffic_manager + 0x128));
	if (!encounter_vehicle)
	{
		return false;
	}

	if (NFSC::PVehicle_IsLoading(encounter_vehicle))
	{
		// Give it some time to load - try again after 1 second (pointless without custom check)
		//Write<float>(time_since_last_encounter_spawn, time - 1.0f);
		return false;
	}

	// Encounter has successfully spawned - reset timer and skin/vehicle keys to 0
	Write<float>(time_since_last_encounter_spawn, 0.0);
	Write<uint32_t>(traffic_manager + 0x120, 0);
	Write<uint32_t>(traffic_manager + 0x128, 0);

	NFSC::PVehicle_SetVehicleOnGround(encounter_vehicle, &nav.fPosition, &fwd_vec);

	uintptr_t encounter_ai = NFSC::PVehicle_GetAIVehiclePtr(encounter_vehicle);
	
	// AIVehicle::SetSpawned
	reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x418600)(encounter_ai);

	// AIVehicle::SetGoal
	constexpr uint32_t AIGoalEncounterPursuit = 0xD4272692;
	reinterpret_cast<void(__thiscall*)(uintptr_t, const uint32_t&)>(0x427A60)(encounter_ai, AIGoalEncounterPursuit);

	NFSC::PVehicle_Activate(encounter_vehicle);
	NFSC::PVehicle_SetSpeed(encounter_vehicle, 15.64605); // 35 mph

	NFSC::WRoadNav_Destructor(nav);

	return true;
}

bool __fastcall Hooks::DALCareer_GetSMSHandle_(uintptr_t ecx, uintptr_t edx, int* a1, int a2)
{
	if (a2 == g::custom_sms::handle)
	{
		// SMS has been deleted and thus no longer exists.
		if (!g::custom_sms::exists)
		{
			return false;
		}

		*a1 = g::custom_sms::handle;
		return true;
	}

	return Hooks::DALCareer_GetSMSHandle(ecx, edx, a1, a2);
}

bool __fastcall Hooks::DALCareer_GetSMSIsAvailable_(uintptr_t ecx, uintptr_t edx, int* a1, int a2)
{
	if (a2 == g::custom_sms::handle)
	{
		// Should the SMS show up in the mailbox? Can safely always return 1 here, since the SMS will not show if it's been deleted
		*a1 = 1;
		return true;
	}

	return Hooks::DALCareer_GetSMSIsAvailable(ecx, edx, a1, a2);
}

bool __fastcall Hooks::DALCareer_GetSMSWasRead_(uintptr_t ecx, uintptr_t edx, int* a1, int a2)
{
	if (a2 == g::custom_sms::handle)
	{
		// Dictates whether it should have a (!) in front or not. Already read messages don't pop-up if GManager::AddSMS gets called
		*a1 = g::custom_sms::read;
		return true;
	}

	return Hooks::DALCareer_GetSMSWasRead(ecx, edx, a1, a2);
}

bool __fastcall Hooks::DALCareer_GetSMSIsTip_(uintptr_t ecx, uintptr_t edx, int* a1, int a2)
{
	if (a2 == g::custom_sms::handle)
	{
		// Not a tip message
		*a1 = 0;
		return true;
	}

	return Hooks::DALCareer_GetSMSIsTip(ecx, edx, a1, a2);
}

bool __fastcall Hooks::DALCareer_GetSMSSortOrder_(uintptr_t ecx, uintptr_t edx, int* a1, int a2)
{
	if (a2 == g::custom_sms::handle)
	{
		// Sort order is an unsigned short. Higher numbers on top, lower numbers on bottom - make sure our SMS is always at the top
		*a1 = 0xFFFF;
		return true;
	}

	return Hooks::DALCareer_GetSMSSortOrder(ecx, edx, a1, a2);
}

bool __fastcall Hooks::DALCareer_GetSMSHashMessage_(uintptr_t ecx, uintptr_t edx, int* a1, int a2)
{
	if (a2 == g::custom_sms::handle)
	{
		// SMS message hash is unused due to our hooks, but still need to return true here to pass check in FESMSMessage::Setup
		return true;
	}

	return Hooks::DALCareer_GetSMSHashMessage(ecx, edx, a1, a2);
}

bool __fastcall Hooks::DALCareer_GetSMSIsVoice_(uintptr_t ecx, uintptr_t edx, int* a1, int a2)
{
	if (a2 == g::custom_sms::handle)
	{
		// Not a voice message
		*a1 = 0;
		return true;
	}

	return Hooks::DALCareer_GetSMSIsVoice(ecx, edx, a1, a2);
}

bool __fastcall Hooks::DALCareer_SetSMSWasRead_(uintptr_t ecx, uintptr_t edx, int a1, int a2)
{
	if (a2 == g::custom_sms::handle)
	{
		// Setting read flag for non-existent SMSs is invalid
		if (!g::custom_sms::exists)
		{
			return false;
		}

		g::custom_sms::read = (bool)a1;
		return true;
	}

	return Hooks::DALCareer_SetSMSWasRead(ecx, edx, a1, a2);
}

bool __fastcall Hooks::DALCareer_SetSMSIsAvailable_(uintptr_t ecx, uintptr_t edx, int a1, int a2)
{
	// No need to check "a1" as it is always 1 in game code (called in GManager::AddSMS)
	if (a2 == g::custom_sms::handle)
	{
		// Reset read flag if we're about to start existing
		if (!g::custom_sms::exists)
		{
			g::custom_sms::read = false;
		}

		g::custom_sms::exists = true;
		return true;
	}

	return Hooks::DALCareer_SetSMSIsAvailable(ecx, edx, a1, a2);
}

bool __fastcall Hooks::DALCareer_SetSMSHandle_(uintptr_t ecx, uintptr_t edx, int a1, int a2)
{
	// No need to check "a1" as it is always 255 in game code (called in FESMSStateManager::HandleButtonPressed)
	if (a2 == g::custom_sms::handle)
	{
		// Delete the SMS. Normally, this is permanent and irreversible for alias SMSs, but for our custom SMS we can will it back to life using GManager::AddSMS
		g::custom_sms::exists = false;
		return true;
	}

	return Hooks::DALCareer_SetSMSHandle(ecx, edx, a1, a2);
}

void __fastcall Hooks::FESMSMessage_RefreshHeader_(uintptr_t fe_sms_message)
{
	// Set SMS headers (From & Subject) for our custom SMS
	if (Read<uint32_t>(fe_sms_message + 0xD8) == g::custom_sms::handle)
	{	
		char* package_name = Read<char*>(fe_sms_message + 0xC);

		uintptr_t object = NFSC::FE_Object_FindObject(package_name, 0xFECED617);
		NFSC::FE_String_SetString(object, g::custom_sms::from);

		object = NFSC::FE_Object_FindObject(package_name, 0x2C167533);
		NFSC::FE_String_SetString(object, g::custom_sms::subject);

		return;
	}

	Hooks::FESMSMessage_RefreshHeader(fe_sms_message);
}

void __fastcall Hooks::CTextScroller_SetTextHash_(uintptr_t c_text_scroller, uintptr_t edx, uint32_t language_hash)
{
	// Set SMS message for our custom SMS
	if (Read<uint32_t>(c_text_scroller - 0x28 /* FESMSMessage */ + 0xD8) == g::custom_sms::handle)
	{
		// CTextScroller::SetText
		reinterpret_cast<void(__thiscall*)(uintptr_t, const wchar_t*)>(0x5BCDF0)(c_text_scroller, g::custom_sms::message);
		return;
	}

	Hooks::CTextScroller_SetTextHash(c_text_scroller, edx, language_hash);
}

void __fastcall Hooks::SMSSlot_Update_(uintptr_t sms_slot, uintptr_t edx, uintptr_t sms_datum, bool a3, uintptr_t fe_object)
{
	// Set SMS subject (in the mailbox list) for our custom SMS
	if (sms_datum && Read<uint32_t>(sms_datum + 0x18) == g::custom_sms::handle)
	{
		// ArraySlot::Update
		reinterpret_cast<void(__thiscall*)(uintptr_t, uintptr_t, bool, uintptr_t)>(0x59A360)(sms_slot, sms_datum, a3, fe_object);

		// this->text_object
		NFSC::FE_String_SetString(Read<uintptr_t>(sms_slot + 0x18), g::custom_sms::subject);

		// this->image_object, sms_datum->checked
		NFSC::FE_Object_SetVisibility(Read<uintptr_t>(sms_slot + 0x14), Read<bool>(sms_datum + 0x17));

		return;
	}

	Hooks::SMSSlot_Update(sms_slot, edx, sms_datum, a3, fe_object);
}

bool dal_spectate = false;

uintptr_t __stdcall Hooks::DALVehicle_GetIVehicle_(int settings_index)
{
	if (dal_spectate)
	{
		return NFSC::spectate::vehicle;
	}

	return Hooks::DALVehicle_GetIVehicle(settings_index);
}

void Hooks::HUDElement_Update_(uintptr_t hud_element, uintptr_t edx, uintptr_t player, HUDElementUpdateFunc* Update)
{
	if (*NFSC::spectate::enabled)
	{
		dal_spectate = true;
	}

	Update(hud_element, edx, player);

	dal_spectate = false;
}

void __fastcall Hooks::Tachometer_Update_(uintptr_t hud_element, uintptr_t edx, uintptr_t player)
{
	Hooks::HUDElement_Update_(hud_element, edx, player, Tachometer_Update);
}

void __fastcall Hooks::NitrousGauge_Update_(uintptr_t hud_element, uintptr_t edx, uintptr_t player)
{
	Hooks::HUDElement_Update_(hud_element, edx, player, NitrousGauge_Update);
}

void __fastcall Hooks::SpeedbreakerMeter_Update_(uintptr_t hud_element, uintptr_t edx, uintptr_t player)
{
	Hooks::HUDElement_Update_(hud_element, edx, player, SpeedbreakerMeter_Update);
}

/*
void* __cdecl Hooks::PickRoadblockSetupHook(float width, int num_vehicles, bool use_spikes)
{
	return g::roadblock_setups::force ? g::roadblock_setups::force : PickRoadblockSetup(width, num_vehicles, use_spikes);
}
*/

void __fastcall Hooks::FEWorldMapStateManager_HandlePadAccept_(uintptr_t fe_state_manager)
{
	// mCurrentState
	auto state = Read<int>(fe_state_manager + 4);

	if (g::world_map::shift_held && state == NFSC::FESM::WorldMap::NORMAL)
	{
		// sub_00582D90 (seems legit???)
		reinterpret_cast<void(__thiscall*)(uintptr_t, bool)>(0x582D90)(Read<uintptr_t>(NFSC::WorldMap), true);
		NFSC::FEStateManager_ShowDialog(fe_state_manager, NFSC::FESM::WorldMap::CLICK_TP);
		return;
	}

	Hooks::FEWorldMapStateManager_HandlePadAccept(fe_state_manager);
}

void __fastcall Hooks::FEWorldMapStateManager_HandleButtonPressed_(uintptr_t fe_state_manager, uintptr_t edx, uint32_t unk)
{
	// mCurrentState
	auto state = Read<int>(fe_state_manager + 4);

	uintptr_t dialog_screen = Read<uintptr_t>(0xA97B14);
	if (!dialog_screen)
	{
		Hooks::FEWorldMapStateManager_HandleButtonPressed(fe_state_manager, edx, unk);
		return;
	}

	uint32_t* button_hashes = Read<uint32_t*>(dialog_screen + 0x2C);

	auto gfs = NFSC::BulbToys_GetGameFlowState();

	if (state == NFSC::FESM::WorldMap::CLICK_TP)
	{
		// We are not in FE and we clicked on a valid road
		if (gfs == NFSC::GFS::RACING && !isnan(g::world_map::location.y))
		{
			// First button - Jump to Location
			if (unk == button_hashes[0])
			{
				NFSC::FEStateManager_ChangeState(fe_state_manager, NFSC::FESM::WorldMap::CLICK_TP_JUMP);
			}

			// Second button - Activate GPS
			else if (unk == button_hashes[1])
			{
				NFSC::FEStateManager_ChangeState(fe_state_manager, NFSC::FESM::WorldMap::CLICK_TP_GPS);
			}

			// Third button - Cancel
			else if (unk == button_hashes[2])
			{
				NFSC::FEStateManager_PopBack(fe_state_manager, 3);
			}
		}

		// First button - OK
		else if (unk == button_hashes[0])
		{
			NFSC::FEStateManager_PopBack(fe_state_manager, 3);
		}

		return;
	}

	// Only offer GPS if the option is enabled and we're not in FE
	if (g::world_map::gps_only && gfs == NFSC::GFS::RACING)
	{
		if (state == NFSC::FESM::WorldMap::RACE_EVENT || state == NFSC::FESM::WorldMap::CAR_LOT || state == NFSC::FESM::WorldMap::SAFEHOUSE)
		{
			// For these types of dialogs, use the button hashes of the GPS to safehouse prompt during pursuits
			//Write<int>(fe_state_manager + 4, 18);

			// ???

			// First button - Activate GPS
			if (unk == button_hashes[0])
			{
				NFSC::FEStateManager_ChangeState(fe_state_manager, 21);
			}

			// Second button - Cancel
			else if (unk == button_hashes[1])
			{
				NFSC::FEStateManager_PopBack(fe_state_manager, 3);
			}

			return;
		}
	}

	Hooks::FEWorldMapStateManager_HandleButtonPressed(fe_state_manager, edx, unk);
}

void __fastcall Hooks::FEWorldMapStateManager_HandleStateChange_(uintptr_t fe_state_manager)
{
	// mCurrentState
	auto state = Read<int>(fe_state_manager + 4);

	if (state == NFSC::FESM::WorldMap::CLICK_TP_JUMP)
	{
		uintptr_t vehicle = 0;
		uintptr_t simable = 0;
		if (!NFSC::BulbToys_GetMyVehicle(&vehicle, &simable))
		{
			return;
		}

		uintptr_t rigid_body = NFSC::PhysicsObject_GetRigidBody(simable);

		NFSC::Vector3 fwd_vec;
		NFSC::RigidBody_GetForwardVector(rigid_body, &fwd_vec);

		NFSC::Vector3 dimensions;
		NFSC::RigidBody_GetDimension(rigid_body, &dimensions);
		g::world_map::location.y += dimensions.y + 0.5f;

		NFSC::PVehicle_SetVehicleOnGround(vehicle, &g::world_map::location, &fwd_vec);

		// this->mNextManager = this->mParentManager;
		Write<uintptr_t>(fe_state_manager + 0xB4, Read<uintptr_t>(fe_state_manager + 0xAC));

		// this->mExitPoint = 2;
		Write<int>(fe_state_manager + 0xC, 2);

		// this->mSubState = 3;
		Write<int>(fe_state_manager + 0x18, 3);

		// FEStateManager::ProcessScreenTransition(this);
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x59B1B0)(fe_state_manager);

		return;
	}
	else if (state == NFSC::FESM::WorldMap::CLICK_TP_GPS)
	{
		if (NFSC::GPS_Engage(&g::world_map::location, 0.0, false))
		{
			NFSC::Vector3 position = { g::world_map::location.z, -g::world_map::location.x, g::world_map::location.y };
			auto icon = NFSC::GManager_AllocIcon(Read<uintptr_t>(NFSC::GManagerBase), 0x15, &position, 0, false);

			// Set flag to ShowOnSpawn
			//Write<uint8_t>(icon_addr + 1, 0x40);

			// Set flag to ShowInWorld + ShowOnMap
			Write<uint8_t>(icon + 0x1, 3);

			// Set color to white
			Write<uint32_t>(icon + 0x20, g::world_map::gps_color);

			// Set tex hash
			Write<uint32_t>(icon + 0x24, NFSC::bStringHash("MINIMAP_ICON_EVENT"));

			NFSC::GIcon_Spawn(icon);
			NFSC::WorldMap_SetGPSIng(icon);

			// Set flag to previous + Spawned + Enabled + GPSing
			Write<uint8_t>(icon + 1, 0x8F);
		}

		// this->mNextManager = this->mParentManager;
		Write<uintptr_t>(fe_state_manager + 0xB4, Read<uintptr_t>(fe_state_manager + 0xAC));

		// this->mExitPoint = 2;
		Write<int>(fe_state_manager + 0xC, 2);

		// this->mSubState = 3;
		Write<int>(fe_state_manager + 0x18, 3);

		// FEStateManager::ProcessScreenTransition(this);
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x59B1B0)(fe_state_manager);

		return;
	}

	Hooks::FEWorldMapStateManager_HandleStateChange(fe_state_manager);
}

void __fastcall Hooks::FEWorldMapStateManager_HandleScreenTick_(uintptr_t fe_state_manager)
{
	NFSC::BulbToys_UpdateWorldMapCursor(fe_state_manager);

	Hooks::FEWorldMapStateManager_HandleScreenTick(fe_state_manager);
}

// The "2" key
void __fastcall Hooks::FEWorldMapStateManager_HandlePadButton4_(uintptr_t fe_state_manager)
{
	// Correctly set FEManager's event key if we press the "Select wingman" button when interacting with an event's engagement ring
	if (g::wrong_warp_fix::enabled && Read<int>(fe_state_manager + 4) == NFSC::FESM::WorldMap::ENGAGE_EVENT)
	{
		// FEManager::mInstance->mEventKey = WorldMap::GetEventHash(WorldMap::mInstance);
		Write<uint32_t>(Read<uintptr_t>(NFSC::FEManager) + 0xEC,
			reinterpret_cast<uint32_t(__thiscall*)(uintptr_t)>(0x58FDC0)(Read<uintptr_t>(NFSC::WorldMap)));
	}

	Hooks::FEWorldMapStateManager_HandlePadButton4(fe_state_manager);
}

void __fastcall Hooks::FEWorldMapStateManager_HandleShowDialog_(uintptr_t fe_state_manager)
{	
	constexpr uintptr_t HideCursor = 0xA97B38;

	const char* COMMON_CANCEL = NFSC::GetLocalizedString(0x1A294DAD);
	const char* COMMON_OK = NFSC::GetLocalizedString(0x417B2601);	
	const char* DIALOG_MSG_ACTIVATE_GPS = NFSC::GetLocalizedString(0x6EB0EACE);

	// mCurrentState
	auto type = Read<int>(fe_state_manager + 4);

	if (type == NFSC::FESM::WorldMap::CLICK_TP)
	{
		Write<bool>(HideCursor, false);

		char title[64];
		if (isnan(g::world_map::location.y))
		{
			sprintf_s(title, 64, "Selected coordinates: (%.2f, N/A, %.2f)", g::world_map::location.x, g::world_map::location.z);

			NFSC::FEDialogScreen_ShowDialog(title, COMMON_OK, nullptr, nullptr);
		}
		else
		{
			sprintf_s(title, 64, "Selected coordinates: (%.2f, %.2f, %.2f)", g::world_map::location.x, g::world_map::location.y, g::world_map::location.z);

			NFSC::FEDialogScreen_ShowDialog(title, "Jump to Location", DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL);
		}

		return;
	}

	// Only offer GPS if the option is enabled and we're not in FE
	if (g::world_map::gps_only && NFSC::BulbToys_GetGameFlowState() == NFSC::GFS::RACING)
	{
		if (type == NFSC::FESM::WorldMap::RACE_EVENT)
		{
			// Unhide the cursor and show the dialog with its respective string
			Write<bool>(HideCursor, false);

			// DIALOG_MSG_WORLDMAP_EVENT
			NFSC::FEDialogScreen_ShowDialog(NFSC::GetLocalizedString(0xCF93709B), DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL, nullptr);
			return;
		}
		else if (type == NFSC::FESM::WorldMap::CAR_LOT)
		{
			Write<bool>(HideCursor, false);
			
			// DIALOG_MSG_WORLDMAP_CARLOT
			NFSC::FEDialogScreen_ShowDialog(NFSC::GetLocalizedString(0xBBE2483E), DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL, nullptr);

			return;
		}
		else if (type == NFSC::FESM::WorldMap::SAFEHOUSE)
		{
			Write<bool>(HideCursor, false);

			// DIALOG_MSG_WORLDMAP_SAFEHOUSE
			NFSC::FEDialogScreen_ShowDialog(NFSC::GetLocalizedString(0x8B776D3C), DIALOG_MSG_ACTIVATE_GPS, COMMON_CANCEL, nullptr);

			return;
		}
	}

	Hooks::FEWorldMapStateManager_HandleShowDialog(fe_state_manager);
}

/*__declspec(naked) void Hooks::CreateRoadBlockHook()
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

bool __fastcall Hooks::DALWorldMap_GetBool_(uintptr_t dal_world_map, uintptr_t edx, int id, bool* result)
{
	*result = true;
	return true;
}

/*
void __fastcall Hooks::UpdateRaceRouteHook(uintptr_t minimap)
{
	UpdateRaceRoute(minimap);


//////////////////////////////
	uintptr_t flm = Read<uintptr_t>(minimap + 0x158);

	if (wm_flm)
	{
		// dtor
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x7597C0)(wm_flm);
		NFSC::free(wm_flm);
	}
	
	// ctor
	wm_flm = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, bool)>(0x759690)(NFSC::malloc(0x160), false);

	// copy mBuildData
	memcpy(Read<void*>(wm_flm + 0), Read<void*>(flm + 0), 0x5C);

//////////////////
}
*/


void __fastcall Hooks::Minimap_dtor_(uintptr_t minimap)
{
	if (g::world_map::flm)
	{
		// dtor
		reinterpret_cast<void(__thiscall*)(uintptr_t)>(0x7597C0)(g::world_map::flm);
		NFSC::free(g::world_map::flm);
	}

	uintptr_t flm = Read<uintptr_t>(minimap + 0x158);
	g::world_map::flm = flm;
	Write<void*>(minimap + 0x158, nullptr);

	Hooks::Minimap_dtor(minimap);
}


void __fastcall Hooks::FatLineMesh_AddBezier_(uintptr_t flm, uintptr_t edx, void* vec2_a, void* vec2_b, void* vec2_c, void* vec2_d, float f)
{
	auto a = (NFSC::Vector2*)vec2_a;
	auto b = (NFSC::Vector2*)vec2_b;
	auto c = (NFSC::Vector2*)vec2_c;
	auto d = (NFSC::Vector2*)vec2_d;

	/*
	//Error("A: { %.2f, %.2f }\nB: { %.2f, %.2f }\nC: { %.2f, %.2f }\nC: { %.2f, %.2f }", a->x, a->y, b->x, b->y, c->x, c->y, d->x, d->y);

	float x = g::flm::x;
	float y = g::flm::y;
	float scale = g::flm::scale;

	if (!g::flm::custom)
	{
		if (NFSC::BulbToys_IsGameNFSCO())
		{
			x = 287.f;
		}
		else
		{
			x = -9.f;
		}

		y = 451.f;
		scale = 1.27f;
	}


	a->x -= x;
	a->y -= y;
	b->x -= x;
	b->y -= y;
	c->x -= x;
	c->y -= y;
	d->x -= x;
	d->y -= y;

	a->x *= scale;
	a->y *= scale;
	b->x *= scale;
	b->y *= scale;
	c->x *= scale;
	c->y *= scale;
	d->x *= scale;
	d->y *= scale;
	*/

	g::world_map::test.push_back(*a);
	g::world_map::test.push_back(*b);
	g::world_map::test.push_back(*c);
	g::world_map::test.push_back(*d);

	Hooks::FatLineMesh_AddBezier(flm, edx, vec2_a, vec2_b, vec2_c, vec2_d, f);
}

void __stdcall Hooks::cFEngRender_RenderTerritoryBorder_(uintptr_t object)
{
	// If we don't have a FLM, let the map render its territory borders normally
	if (!g::world_map::flm)
	{
		Hooks::cFEngRender_RenderTerritoryBorder(object);
		return;
	}

	uintptr_t territory = Read<uintptr_t>(0xA977F8);

	// mask
	reinterpret_cast<void(__thiscall*)(uintptr_t, uintptr_t, uintptr_t)>(0x7408C0)(g::world_map::flm, g::world_map::mask, territory + 0x50);

	// FatLineMesh::RenderFE(wm_flm, FEWorldMapTerritory::sInstance->views, &FEWorldMapTerritory::sInstance->matrix)
	reinterpret_cast<void(__thiscall*)(uintptr_t, uintptr_t, uintptr_t)>(0x74F0B0)(g::world_map::flm, Read<uintptr_t>(territory + 0xA0), territory + 0x50);
}

void __fastcall Hooks::FEPhotoModeStateManager_Start_(uintptr_t state_manager)
{
	auto fesm = reinterpret_cast<NFSC::FEStateManager*>(state_manager);
	
	// FEQuickRaceStateManager
	if (fesm->parent && fesm->parent->parent && fesm->parent->parent->vtable == 0x9F8428)
	{
		auto handle = Read<uint32_t>(Read<uintptr_t>(reinterpret_cast<uintptr_t>(fesm->parent) + 0xD0) + 0x8);

		// Only show the Customize button for My Cars vehicles
		uintptr_t user_profile = NFSC::FEManager_GetUserProfile(Read<uintptr_t>(NFSC::FEManager), 0);
		uintptr_t car_record = NFSC::FEPlayerCarDB_GetCarRecordByHandle(user_profile + 0x234, handle);
		if ((Read<uint32_t>(car_record + 0xC) & 0x4) == 0x4)
		{
			// FeCrewCar.fng
			NFSC::FEStateManager_Push(state_manager, reinterpret_cast<char*>(0x9D0078), 0);
			return;
		}
	}

	// FePhotoMode.fng
	NFSC::FEStateManager_Push(state_manager, reinterpret_cast<char*>(0x9CFCA8), 0);
}

void __fastcall Hooks::FEPhotoModeStateManager_HandlePadAccept_(uintptr_t state_manager)
{
	auto fesm = reinterpret_cast<NFSC::FEStateManager*>(state_manager);
	auto handle = Read<uint32_t>(Read<uintptr_t>(reinterpret_cast<uintptr_t>(fesm->parent) + 0xD0) + 0x8);

	uintptr_t user_profile = NFSC::FEManager_GetUserProfile(Read<uintptr_t>(NFSC::FEManager), 0);
	uintptr_t car_record = NFSC::FEPlayerCarDB_GetCarRecordByHandle(user_profile + 0x234, handle);

	// FECustomizeStateManager::FECustomizeStateManager
	uintptr_t fe_customize_sm = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t, uintptr_t, int, uintptr_t)>(0x8631D0)(NFSC::malloc(0xD4), state_manager, 0, car_record);
	NFSC::FEStateManager_SwitchChildManager(state_manager, fe_customize_sm, NFSC::FESM::PhotoMode::GIMME_MY_QR_FNG, 1);
}

void __fastcall Hooks::FEPhotoModeStateManager_HandleChildFlowDone_(uintptr_t state_manager, uintptr_t edx, int unk)
{
	if (Read<int>(state_manager + 0x4) == NFSC::FESM::PhotoMode::GIMME_MY_QR_FNG)
	{
		// FeCrewCar.fng
		NFSC::FEStateManager_Push(state_manager, reinterpret_cast<char*>(0x9D0078), 0);
	}
}

void __fastcall Hooks::FECrewManagementStateManager_HandleOptionSelected_(uintptr_t state_manager, uintptr_t edx, uint32_t value, int buttons)
{
	// !this->mCurState && FECrewManagement::sInstance && value == 1
	if (!Read<int>(state_manager + 0x4) && Read<uintptr_t>(0xBBAA70) && value == 1)
	{
		// FEPhotoModeStateManager::sIsInFE
		Write<bool>(0xA5EC7C, true);
	}

	Hooks::FECrewManagementStateManager_HandleOptionSelected(state_manager, edx, value, buttons);
}

void __fastcall Hooks::DriftScoring_AddRacer_(uintptr_t drift_scoring, uintptr_t edx, int unk1, const char* player_name, int unk2)
{
	if (strlen(player_name) == 0)
	{
		// COMMON_PLAYER: Player
		Hooks::DriftScoring_AddRacer(drift_scoring, edx, unk1, NFSC::GetLocalizedString(0x393CA814), unk2);
		return;
	}

	Hooks::DriftScoring_AddRacer(drift_scoring, edx, unk1, player_name, unk2);
}

uintptr_t IVehicle_temp;
__declspec(naked) void Hooks::UpdateCopElementsHook1()
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

/*
// This is wasteful, but MASM lacks a JNZ instruction, so there really is no better way
constexpr uintptr_t AIVehicle_GetVehicle = 0x406700;
constexpr uintptr_t PVehicle_IsDestroyed = 0x6D8030;
constexpr uintptr_t Minimap_GetCurrCopElementColor = 0x5D2200;
__declspec(naked) void Hooks::UpdateCopElementsHook2()
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
*/

constexpr uintptr_t AIVehicle_GetVehicle = 0x406700;
constexpr uintptr_t PVehicle_IsDestroyed = 0x6D8030;
constexpr uintptr_t Minimap_GetCurrCopElementColor = 0x5D2200;
__declspec(naked) void Hooks::UpdateCopElementsHook2()
{
	__asm
	{
		// Skip changing the color if the cop vehicle is destroyed
		mov     ecx, IVehicle_temp
		call    PVehicle_IsDestroyed
		test    eax, eax
		jnz     skip

		// Redo what we've overwritten
		mov     ecx, [esp + 0xC]
		call    Minimap_GetCurrCopElementColor
		mov     edi, eax

	skip:
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
__declspec(naked) void Hooks::UpdateRoadBlocksHook()
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

__declspec(naked) void Hooks::MoveVinylVerticalHook()
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

__declspec(naked) void Hooks::MoveVinylHorizontalHook()
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

__declspec(naked) void Hooks::VehicleChangeCacheHook()
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
__declspec(naked) void Hooks::UpdateAIPlayerListingHook()
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

__declspec(naked) void Hooks::PTagBustedHook()
{
	__asm
	{
		// If we've been busted as a racer, switch vehicles with our nearest cop
		// TODO: use pursuit contributions instead?

		// In the __fastcall calling convention, the first argument is stored in ecx (pointer to GRaceStatus instance)
		mov     ecx, NFSC::GRaceStatus
		mov     ecx, [ecx]

		// The second argument is stored in edx (is_busted (= true))
		mov     edx, 1
		call    NFSC::BulbToys_SwitchPTagTarget

		push    0x44A59B
		ret

		// No need to redo what we've overwritten cuz it's useless lol (we overwrote some call to an online-related function)
	}
}

__declspec(naked) void Hooks::DebugActionDropCarHook()
{
	__asm
	{
		call    NFSC::BulbToys_DebugActionDropCar

		// Return to the end of the case instead of where we left off
		push    0x49321D
		ret
	}
}

__declspec(naked) void Hooks::NoWingmanSoundHook()
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

constexpr uintptr_t WorldMap_AddPlayerIcon = 0x5ACB50;
__declspec(naked) void Hooks::NoIconsWorldMapHook()
{
	__asm
	{
		mov     ecx, esi
		call    WorldMap_AddPlayerIcon

		push    0x5CDBC0
		ret
	}
}
