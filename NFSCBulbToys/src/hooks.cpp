#include "hooks.h"
#include "shared.h"
#include "gui.h"
#include "nfsc.h"

#include "../ext/minhook/minhook.h"

#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_impl_win32.h"
#include "../ext/imgui/imgui_impl_dx9.h"

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
		if (MH_CreateHook(reinterpret_cast<LPVOID>(0x710220), &DxInitHook, reinterpret_cast<void**>(&DxInit)) != MH_OK)
		{
			Error("Unable to hook DirectX_Init().");
			return false;
		}

		if (MH_EnableHook(reinterpret_cast<LPVOID>(0x710220)) != MH_OK)
		{
			Error("Unable to enable DirectX_Init() hook.");
			return false;
		}
	}

	return true;
}

bool hooks::SetupPart2(IDirect3DDevice9* device)
{
	if (MH_CreateHook(VirtualFunction(device, 42), &EndSceneHook, reinterpret_cast<void**>(&EndScene)) != MH_OK)
	{
		Error("Unable to hook EndScene().");
		return false;
	}

	if (MH_CreateHook(VirtualFunction(device, 16), &ResetHook, reinterpret_cast<void**>(&Reset)) != MH_OK)
	{
		Error("Unable to hook Reset().");
		return false;
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
		Error("Unable to enable EndScene() and Reset() hooks.");
		return false;
	}

	gui::SetupMenu(device);

	/* Non-critical hooks go here */

	//
	if (MH_CreateHook(reinterpret_cast<LPVOID>(0x5BD3D0), &HandleStateChangeHook, reinterpret_cast<void**>(&HandleStateChange)) == MH_OK &&
		MH_EnableHook(reinterpret_cast<LPVOID>(0x5BD3D0)) == MH_OK)
	{
		// Prevent pushing splash screen: "DEMO_SPLASH.fng" -> ""
		WriteMemory<uint8_t>(0x9CB4E4, 0x00);
	}

	// Override encounter spawn requirement
	if (MH_CreateHook(reinterpret_cast<LPVOID>(0x422BF0), &NeedsEncounterHook, reinterpret_cast<void**>(&NeedsEncounter)) == MH_OK &&
		MH_EnableHook(reinterpret_cast<LPVOID>(0x422BF0)) == MH_OK)
	{
		needs_encounter::hooked = true;
	}

	// Override traffic spawn requirement
	if (MH_CreateHook(reinterpret_cast<LPVOID>(0x422990), &NeedsTrafficHook, reinterpret_cast<void**>(&NeedsTraffic)) == MH_OK &&
		MH_EnableHook(reinterpret_cast<LPVOID>(0x422990)) == MH_OK)
	{
		needs_traffic::hooked = true;
	}

	//
	if (MH_CreateHook(reinterpret_cast<LPVOID>(0x433930), &GpsEngageHook, reinterpret_cast<void**>(&GpsEngage)) == MH_OK &&
		MH_EnableHook(reinterpret_cast<LPVOID>(0x433930)) == MH_OK)
	{
		gps_engage::hooked = true;
	}

	// Increment cop counter by 1 per roadblock vehicle
	// TODO: if re-enabling this, make sure roadblock cops that get attached don't increment again
	//WriteJmp(0x445A9D, CreateRoadBlockHook, 6);

	// Remove dead roadblock cops from the pursuit
	// Horribly unstable
	//WriteJmp(0x445D3B, UpdatePursuitsHook, 19);

	// Fix dead roadblock vehicles not turning off their lights
	WriteJmp(0x5D8C10, UpdateCopElementsHook1, 6);
	WriteJmp(0x5D8CFB, UpdateCopElementsHook2, 11);

	// Attach roadblock cops to the pursuit once it's been dodged
	WriteJmp(0x4410D4, UpdateRoadBlocksHook, 6);
	
	return true;
}

void hooks::Destroy()
{
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

	if (gui::menuOpen)
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

enum class bootflow_state : unsigned int
{
	terminal_state      = 0xFFFFFFFF,
	attract             = 0x00,
	autoload            = 0x01,
	backdrop            = 0x02,
	bootcheck           = 0x03,
	controller_check    = 0x04,
	ea_logo             = 0x05,
	hi_def              = 0x06,
	enable_home_menu    = 0x07,
	language_select     = 0x08,
	psa                 = 0x09,
	splash              = 0x0A,
	num_main_states     = 0x0B,
	do_autoload         = 0x0C,
	do_backdrop         = 0x0D,
	do_bootcheck        = 0x0E,
	do_controller_check = 0x0F,
	do_language_select  = 0x10,
	do_splash           = 0x11,
	pop_backdrop        = 0x12,
	showing_screen      = 0x13,
	playing_attract     = 0x14,
	playing_ea_logo     = 0x15,
	playing_hi_def      = 0x16,
	playing_movie       = 0x17,
	num_total_states    = 0x18
};

void __fastcall hooks::HandleStateChangeHook(void* state_manager)
{
	/*
		Original boot flow is as follows:

		 1. 0x02 - STATE_BACKDROP
		 2. 0x0D - STATE_DO_BACKDROP
		 3. 0x05 - STATE_EA_LOGO
		 4. 0x13 - STATE_SHOWING_SCREEN
		 5. 0x09 - STATE_PSA
		 6. 0x15 - STATE_PLAYING_EA_LOGO
		 7. 0x00 - STATE_ATTRACT
		 8. 0x12 - STATE_POP_BACKDROP
		 9. 0x0A - STATE_SPLASH
		10. 0x11 - STATE_DO_SPLASH
		11. 0x03 - STATE_BOOTCHECK
		12. 0x0E - STATE_DO_BOOTCHECK
		13. 0x01 - STATE_AUTOLOAD
		14. 0x0C - STATE_DO_AUTOLOAD
		15.  -1  - STATE_TERMINAL_STATE
	*/
	auto mCurState = reinterpret_cast<uintptr_t>(state_manager) + 4;
	auto current_state = ReadMemory<bootflow_state>(mCurState);

	// First state is backdrop, force splash instead (creates FeMainMenu)
	if (current_state == bootflow_state::backdrop)
	{
		WriteMemory<bootflow_state>(mCurState, bootflow_state::splash);
	}

	// Now it will try to play the splash, force bootcheck instead (instantiates memcard)
	else if (current_state == bootflow_state::do_splash)
	{
		WriteMemory<bootflow_state>(mCurState, bootflow_state::bootcheck);
	}

	// Next is do_bootcheck, which is what we want, no change necessary

	// Afterwards, it will try to play the EA logo, force autoload instead (handles immediate load if only save(?))
	else if (current_state == bootflow_state::ea_logo)
	{
		WriteMemory<bootflow_state>(mCurState, bootflow_state::autoload);
	}

	// Next is do_autoload, which is what we want, no change necessary

	// Finally, it will try to play the Nikki PSA movie, force terminal state instead (completes boot flow)
	else if (current_state == bootflow_state::psa)
	{
		WriteMemory<bootflow_state>(mCurState, bootflow_state::terminal_state);

		// Crashes the game
		//MH_DisableHook(reinterpret_cast<LPVOID>(0x5BD3D0));
		//MH_RemoveHook(reinterpret_cast<LPVOID>(0x5BD3D0));
	}
	
	HandleStateChange(state_manager);
}

bool __fastcall hooks::NeedsEncounterHook(void* traffic_manager)
{
	return needs_encounter::overridden ? needs_encounter::value : NeedsEncounter(traffic_manager);
}

bool __fastcall hooks::NeedsTrafficHook(void* traffic_manager)
{
	return needs_traffic::overridden ? needs_traffic::value : NeedsTraffic(traffic_manager);
}

bool __fastcall hooks::GpsEngageHook(void* gps, void* edx, nfsc::vector3* vec3target, float max_deviation, bool re_engage, bool always_re_establish)
{
	bool result = GpsEngage(gps, edx, vec3target, max_deviation, re_engage, always_re_establish);

	// If we're in AutoDrive and the GPS has been successfully established, find the path to the destination
	// Since Gps::Engage also gets called after reconnections, turning on AutoDrive late will still work
	if (gps_engage::myAIVehicle && result)
	{
		auto myRoadNav = ReadMemory<void*>(reinterpret_cast<uintptr_t>(gps_engage::myAIVehicle) + 0x38);
		if (myRoadNav)
		{
			// TODO: Needs more testing, AutoDrive might forget the destination at any point, ie. AIActionGetUnstuck, etc...
			nfsc::WRoadNav_FindPath(myRoadNav, vec3target, 0, 1);
		}
	}

	return result;
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

/*
constexpr uintptr_t AIPursuit_IncNumCopsDestroyed = 0x42A4E0;
constexpr uintptr_t AIVehiclePursuit_SetInPursuit = 0x43CE10;
uintptr_t AIVehiclePursuit_temp;
__declspec(naked) void hooks::UpdatePursuitsHook()
{
	__asm
	{
		// Preserve eax (AIVehicle pointer)
		mov     AIVehiclePursuit_temp, eax

		// Redo what we've overwritten
		call    dword ptr[edx + 0xC0]
		mov     ecx, [eax + 0x60]
		test    ecx, ecx
		jz      skip
		push    esi
		call    AIPursuit_IncNumCopsDestroyed

		// If a roadblock cop has been killed, unset it from the pursuit
		// Reuse AIVehicle pointer that we've stored
		mov     ecx, AIVehiclePursuit_temp
		push    0
		call    AIVehiclePursuit_SetInPursuit

	skip:
		// Continue as normal (return to just after the jump)
		push    0x445D4E
		ret
	}
}
*/

uintptr_t IVehicle_temp;
__declspec(naked) void hooks::UpdateCopElementsHook1()
{
	__asm
	{
		// Redo what we've overwritten
		mov     esi, [ecx]
		mov     edx, [esi]
		mov     ecx, esi

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
		// Skip changing the color if the vehicle is destroyed
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

constexpr uintptr_t AIPursuit_Attach = 0x412850;
uintptr_t AIPursuit;
uintptr_t AIRoadBlock;
__declspec(naked) void hooks::UpdateRoadBlocksHook()
{
	__asm
	{
		// Redo what we've overwritten
		inc     dword ptr[edi + 0x1B4]

		// Preserve pursuit and roadblock
		mov		AIPursuit, edi
		mov		AIRoadBlock, esi

		push    ebx
		push	ebp
		push	esi
		push	edi

		// Check if the vehicle list is empty, bail if so, iterate otherwise
		mov		ebx, AIRoadBlock
		mov     eax, [ebx + 0x44]
		mov     edi, [ebx + 0x40]
		cmp		edi, eax
		jz		done_iterating

	iterate:
		mov     esi, [edi]
		push	esi
		mov     ecx, AIPursuit
		call	AIPursuit_Attach

		// Next vehicle, check if it's the last vehicle, stop iterating if so, continue iterating otherwise
		mov		eax, [ebx + 0x44]
		add		edi, 4
		cmp		edi, eax
		jz		done_iterating
		jmp		iterate

	done_iterating:
		pop		edi
		pop		esi
		pop		ebp
		pop		ebx
		push	0x4410DA
		ret
	}
}