#pragma once
#include "../ext/minhook/minhook.h"

#define HOOK(address, return_t, callconv, name, ...) return_t callconv name##_##(__VA_ARGS__); \
	static inline decltype(&name##_##) name = reinterpret_cast<decltype(&name##_##)>(address) \

#define VIRTUAL 0x0

// todo fuck me
namespace NFSC
{
	struct Vector3;
}

namespace Hooks
{
	inline MH_STATUS CreateHook(void* address, void* hook, void* call);
	inline MH_STATUS CreateHook(uintptr_t address, void* hook, void* call);
	inline void CreateVTablePatch(uintptr_t vtbl_func_addr, void* hook, void* call);

	bool Setup();
	bool SetupPart2(uintptr_t device);
	void Destroy();

	HOOK(0x710220, HRESULT, __cdecl, DirectX_Init);

	HOOK(VIRTUAL, HRESULT, __stdcall, ID3DDevice9_EndScene, IDirect3DDevice9* device);
	HOOK(VIRTUAL, HRESULT, __stdcall, ID3DDevice9_Reset, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params);

	HOOK(0x422BF0, bool, __fastcall, AITrafficManager_NeedsEncounter, uintptr_t traffic_manager);
	HOOK(0x422990, bool, __fastcall, AITrafficManager_NeedsTraffic, uintptr_t traffic_manager);

	HOOK(VIRTUAL, bool, __fastcall, AICopManager_CanPursueRacers, uintptr_t cop_manager);

	HOOK(0x433930, bool, __fastcall, Gps_Engage, uintptr_t gps, uintptr_t edx, NFSC::Vector3* vec3target, float max_deviation, bool re_engage, bool always_re_establish);

	HOOK(VIRTUAL, void, __fastcall, FEWorldMapStateManager_HandlePadAccept, uintptr_t state_manager);
	HOOK(VIRTUAL, void, __fastcall, FEWorldMapStateManager_HandleShowDialog, uintptr_t state_manager);
	HOOK(VIRTUAL, void, __fastcall, FEWorldMapStateManager_HandleButtonPressed, uintptr_t state_manager, uintptr_t edx, uint32_t unk);
	HOOK(VIRTUAL, void, __fastcall, FEWorldMapStateManager_HandleStateChange, uintptr_t state_manager);
	HOOK(VIRTUAL, void, __fastcall, FEWorldMapStateManager_HandleScreenTick, uintptr_t state_manager);
	HOOK(VIRTUAL, void, __fastcall, FEWorldMapStateManager_HandlePadButton4, uintptr_t state_manager);

	HOOK(VIRTUAL, void, __fastcall, AIVehicleHuman_ResetDriveToNav, uintptr_t ai_vehicle, uintptr_t edx, int lane_selection);

	HOOK(0x6298C0, uintptr_t, __fastcall, GRacerInfo_CreateVehicle, uintptr_t racer_info, uintptr_t edx, uint32_t key, int racer_index, uint32_t seed);

	HOOK(0x616EF0, const char*, __cdecl, GKnockoutRacer_GetPursuitVehicleName, bool is_player);

	HOOK(0x646B00, void, __fastcall, GRaceStatus_Update, uintptr_t race_status, uintptr_t edx, float dt);

	//uintptr_t __cdecl PursuitSwitchHook(int racer_index, bool is_busted, int* result);
	//static inline decltype (&PursuitSwitchHook) PursuitSwitch;

	HOOK(0x63E6C0, float, __fastcall, GRaceParameters_GetTimeLimit, uintptr_t race_parameters);

	HOOK(0x65E240, void, __cdecl, FE_ShowLosingPostRaceScreen);
	HOOK(0x65E270, void, __cdecl, FE_ShowWinningPostRaceScreen);

	HOOK(VIRTUAL, void, __fastcall, FECareerStateManager_HandleChildFlowDone, uintptr_t fe_career_state_manager, uintptr_t edx, int unk);

	HOOK(0x42CB70, uintptr_t, __fastcall, AITrafficManager_GetAvailablePresetVehicle, uintptr_t ai_traffic_manager, uintptr_t edx, uint32_t skin_key, uint32_t encounter_key);

	HOOK(VIRTUAL, void, __fastcall, FEDebugCarStateManager_HandlePadButton3, uintptr_t fe_debugcar_state_manager);

	HOOK(0x641310, void, __fastcall, GRaceStatus_SetRoaming, uintptr_t g_race_status);

	HOOK(0x7AEFD0, void, __fastcall, CarRenderConn_UpdateIcon, uintptr_t car_render_conn, uintptr_t edx, uintptr_t pkt);

	HOOK(0x4822F0, void, __fastcall, Camera_SetCameraMatrix, uintptr_t camera, uintptr_t edx, void* matrix4, float dt);

	//void* __cdecl PickRoadblockSetupHook(float width, int num_vehicles, bool use_spikes);
	//static inline decltype (&PickRoadblockSetupHook) PickRoadblockSetup;

	HOOK(0x643A50, uintptr_t, __fastcall, GRaceStatus_GetMainBoss, uintptr_t g_race_status);

	HOOK(0x5ACB50, void, __fastcall, WorldMap_AddPlayerCar, uintptr_t world_map);

	HOOK(0x44BCE0, uintptr_t, __fastcall, MLaunchPIP_MLaunchPIP, uintptr_t m_launch_pip, uintptr_t edx, int id, uintptr_t simable_handle);

	HOOK(0x444D90, bool, __fastcall, AITrafficManager_SpawnEncounter, uintptr_t traffic_manager);

	HOOK(0x4A1250, bool, __fastcall, DALCareer_GetSMSHandle, uintptr_t ecx, uintptr_t edx, int* a1, int a2);
	HOOK(0x4A1290, bool, __fastcall, DALCareer_GetSMSIsAvailable, uintptr_t ecx, uintptr_t edx, int* a1, int a2);
	HOOK(0x4A12E0, bool, __fastcall, DALCareer_GetSMSWasRead, uintptr_t ecx, uintptr_t edx, int* a1, int a2);
	HOOK(0x4A1330, bool, __fastcall, DALCareer_GetSMSIsTip, uintptr_t ecx, uintptr_t edx, int* a1, int a2);
	HOOK(0x4A1390, bool, __fastcall, DALCareer_GetSMSSortOrder, uintptr_t ecx, uintptr_t edx, int* a1, int a2);
	HOOK(0x4A14A0, bool, __fastcall, DALCareer_GetSMSHashMessage, uintptr_t ecx, uintptr_t edx, int* a1, int a2);
	HOOK(0x4B2B80, bool, __fastcall, DALCareer_GetSMSIsVoice, uintptr_t ecx, uintptr_t edx, int* a1, int a2);

	HOOK(0x4A1530, bool, __fastcall, DALCareer_SetSMSWasRead, uintptr_t ecx, uintptr_t edx, int a1, int a2);
	HOOK(0x4A15B0, bool, __fastcall, DALCareer_SetSMSIsAvailable, uintptr_t ecx, uintptr_t edx, int a1, int a2);
	HOOK(0x4A1600, bool, __fastcall, DALCareer_SetSMSHandle, uintptr_t ecx, uintptr_t edx, int a1, int a2);

	HOOK(0x5D7550, void, __fastcall, FESMSMessage_RefreshHeader, uintptr_t fe_sms_message);

	HOOK(0x5C6370, void, __fastcall, CTextScroller_SetTextHash, uintptr_t c_text_scroller, uintptr_t edx, uint32_t language_hash);

	HOOK(0x5D7250, void, __fastcall, SMSSlot_Update, uintptr_t sms_slot, uintptr_t edx, uintptr_t sms_datum, bool a3, uintptr_t fe_object);

	HOOK(0x4B62B0, uintptr_t, __stdcall, DALVehicle_GetIVehicle, int settings_index);

	using HUDElementUpdateFunc = void __fastcall (uintptr_t hud_element, uintptr_t edx, uintptr_t player);
	void HUDElement_Update_(uintptr_t hud_element, uintptr_t edx, uintptr_t player, HUDElementUpdateFunc* Update);

	HUDElementUpdateFunc Tachometer_Update_;
	static inline HUDElementUpdateFunc* Tachometer_Update;

	HUDElementUpdateFunc NitrousGauge_Update_;
	static inline HUDElementUpdateFunc* NitrousGauge_Update;

	HUDElementUpdateFunc SpeedbreakerMeter_Update_;
	static inline HUDElementUpdateFunc* SpeedbreakerMeter_Update;

	HOOK(0x4B57E0, bool, __fastcall, DALWorldMap_GetBool, uintptr_t dal_world_map, uintptr_t edx, int id, bool* result);

	HOOK(0x573D30, void, __stdcall, cFEngRender_RenderTerritoryBorder, uintptr_t feobject);

	//void __fastcall UpdateRaceRouteHook(uintptr_t minimap);
	//static inline decltype (&UpdateRaceRouteHook) UpdateRaceRoute;

	//void __fastcall MinimapDestructorHook(uintptr_t minimap);
	//static inline decltype (&MinimapDestructorHook) MinimapDestructor;

	//void __fastcall FLMMoveMaybeHook(uintptr_t flm, uintptr_t edx, void* vec2_a, void* vec2_b, void* vec2_c, void* vec2_d, float f);
	//static inline decltype (&FLMMoveMaybeHook) FLMMoveMaybe;

	HOOK(VIRTUAL, void, __fastcall, FEPhotoModeStateManager_Start, uintptr_t state_manager);
	HOOK(VIRTUAL, void, __fastcall, FEPhotoModeStateManager_HandlePadAccept, uintptr_t state_manager);
	HOOK(VIRTUAL, void, __fastcall, FEPhotoModeStateManager_HandleChildFlowDone, uintptr_t state_manager, uintptr_t edx, int unk);

	HOOK(VIRTUAL, void, __fastcall, FECrewManagementStateManager_HandleOptionSelected, uintptr_t state_manager, uintptr_t edx, uint32_t value, int buttons);

	//void CreateRoadBlockHook();
	void UpdateCopElementsHook1();
	void UpdateCopElementsHook2();
	//void UpdateRoadBlocksHook();
	void MoveVinylVerticalHook();
	void MoveVinylHorizontalHook();
	void VehicleChangeCacheHook();
	void UpdateAIPlayerListingHook();
	void PTagBustedHook();
	void DebugActionDropCarHook();
	void NoWingmanSoundHook();
	void NoIconsWorldMapHook();
}