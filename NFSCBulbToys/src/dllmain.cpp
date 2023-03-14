#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <thread>
#include <cstdint>

#include "hooks.h"
#include "gui.h"
#include "shared.h"

void Error(const char* message) noexcept {
	MessageBoxA(NULL, message, PROJECT_NAME, MB_ICONERROR);
}

void Setup(const HMODULE instance) noexcept {
	if (gui::Setup() && hooks::Setup())
		while (!GetAsyncKeyState(PANIC_KEY))
			std::this_thread::sleep_for(std::chrono::milliseconds(200));

	hooks::Destroy();
	gui::Destroy();

	FreeLibraryAndExitThread(instance, 0);
}

BOOL APIENTRY DllMain(HMODULE instance, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
		IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)(base);
		IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

		// Check if .exe file is compatible - Thanks to thelink2012 and MWisBest
		if ((base + nt->OptionalHeader.AddressOfEntryPoint + (0x400000 - base)) == 0x87E926) { 
			DisableThreadLibraryCalls(instance);

			const auto thread = CreateThread(
				nullptr,
				0,
				reinterpret_cast<LPTHREAD_START_ROUTINE>(Setup),
				instance,
				0,
				nullptr
			);

			if (thread)
				CloseHandle(thread);
		}

		else {
			Error("Only the v1.4 English nfsc.exe (6,88 MB (7.217.152 bytes)) is supported.");
			return FALSE;
		}
	}

	return TRUE;
}