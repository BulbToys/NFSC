#pragma once
#include "../ext/minhook/minhook.h"

namespace hooks
{
	inline MH_STATUS CreateHook(uintptr_t address, void* hook, void* call);
	bool Setup();
	bool SetupPart2(uintptr_t device);
	void Destroy();

	HRESULT __cdecl DxInitHook();
	static inline decltype(&DxInitHook) DxInit;

	long __stdcall EndSceneHook(IDirect3DDevice9* device);
	static inline decltype(&EndSceneHook) EndScene;

	HRESULT __stdcall ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params);
	static inline decltype(&ResetHook) Reset;

	bool __fastcall NeedsEncounterHook(uintptr_t traffic_manager);
	static inline decltype(&NeedsEncounterHook) NeedsEncounter;

	bool __fastcall NeedsTrafficHook(uintptr_t traffic_manager);
	static inline decltype(&NeedsTrafficHook) NeedsTraffic;

	bool __fastcall PursueRacersHook(uintptr_t ai_cop_manager);
	static inline decltype(&PursueRacersHook) PursueRacers;

	bool __fastcall GpsEngageHook(uintptr_t gps, uintptr_t edx, nfsc::Vector3* vec3target, float max_deviation, bool re_engage, bool always_re_establish);
	static inline decltype(&GpsEngageHook) GpsEngage;

	void __fastcall WorldMapPadAcceptHook(uintptr_t fe_state_manager);
	static inline decltype(&WorldMapPadAcceptHook) WorldMapPadAccept;

	bool __fastcall WorldMapSnapHook(uintptr_t world_map);
	static inline decltype(&WorldMapSnapHook) WorldMapSnap;

	void __fastcall WorldMapShowDialogHook(uintptr_t fe_state_manager);
	static inline decltype(&WorldMapShowDialogHook) WorldMapShowDialog;

	void __fastcall WorldMapButtonPressedHook(uintptr_t fe_state_manager, uintptr_t edx, uint32_t unk);
	static inline decltype(&WorldMapButtonPressedHook) WorldMapButtonPressed;

	void __fastcall WorldMapStateChangeHook(uintptr_t fe_state_manager);
	static inline decltype(&WorldMapStateChangeHook) WorldMapStateChange;

	void __fastcall WorldMapScreenTickHook(uintptr_t fe_state_manager);
	static inline decltype(&WorldMapScreenTickHook) WorldMapScreenTick;

	void __fastcall WorldMapButton4Hook(uintptr_t fe_state_manager);
	static inline decltype(&WorldMapButton4Hook) WorldMapButton4;

	void __fastcall ResetDriveToNavHook(uintptr_t ai_vehicle, uintptr_t edx, int lane_selection);
	static inline decltype (&ResetDriveToNavHook) ResetDriveToNav;

	uintptr_t __fastcall RacerInfoCreateVehicleHook(uintptr_t racer_info, uintptr_t edx, uint32_t key, int racer_index, uint32_t seed);
	static inline decltype (&RacerInfoCreateVehicleHook) RacerInfoCreateVehicle;

	const char* __cdecl GetPursuitVehicleNameHook(bool is_player);
	static inline decltype (&GetPursuitVehicleNameHook) GetPursuitVehicleName;

	void __fastcall RaceStatusUpdateHook(uintptr_t race_status, uintptr_t edx, float dt);
	static inline decltype (&RaceStatusUpdateHook) RaceStatusUpdate;

	//uintptr_t __cdecl PursuitSwitchHook(int racer_index, bool is_busted, int* result);
	//static inline decltype (&PursuitSwitchHook) PursuitSwitch;

	float __fastcall GetTimeLimitHook(uintptr_t race_parameters);
	static inline decltype (&GetTimeLimitHook) GetTimeLimit;

	void __cdecl ShowLosingScreenHook();
	static inline decltype (&ShowLosingScreenHook) ShowLosingScreen;

	void __cdecl ShowWinningScreenHook();
	static inline decltype (&ShowWinningScreenHook) ShowWinningScreen;

	void __fastcall CareerManagerChildFlowDoneHook(uintptr_t fe_career_state_manager, uintptr_t edx, int unk);
	static inline decltype (&CareerManagerChildFlowDoneHook) CareerManagerChildFlowDone;

	uintptr_t __fastcall GetAvailablePresetVehicleHook(uintptr_t ai_traffic_manager, uintptr_t edx, uint32_t skin_key, uint32_t encounter_key);
	static inline decltype (&GetAvailablePresetVehicleHook) GetAvailablePresetVehicle;

	void __fastcall DebugCarPadButton3Hook(uintptr_t fe_debugcar_state_manager);
	static inline decltype (&DebugCarPadButton3Hook) DebugCarPadButton3;

	void __fastcall SetRoamingHook(uintptr_t g_race_status);
	static inline decltype (&SetRoamingHook) SetRoaming;

	void __fastcall UpdateIconHook(uintptr_t car_render_conn, uintptr_t edx, uintptr_t pkt);
	static inline decltype (&UpdateIconHook) UpdateIcon;

	//void* __cdecl PickRoadblockSetupHook(float width, int num_vehicles, bool use_spikes);
	//static inline decltype (&PickRoadblockSetupHook) PickRoadblockSetup;

	//void CreateRoadBlockHook();
	void UpdateCopElementsHook1();
	void UpdateCopElementsHook2();
	//void UpdateRoadBlocksHook();
	void MoveVinylVerticalHook();
	void MoveVinylHorizontalHook();
	void VehicleChangeCacheHook();
	void UpdateAIPlayerListingHook();
	void PTagBustedHook();
}