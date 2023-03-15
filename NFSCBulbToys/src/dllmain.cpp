#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <thread>
#include <cstdint>

#include "hooks.h"
#include "gui.h"
#include "shared.h"

inline void Error(const char* message) noexcept {
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
		if (!strcmp(reinterpret_cast<char*>(0x9F94B0), "This is test, This is Test. This is test, This is Test.")) {
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