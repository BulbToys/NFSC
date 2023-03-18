#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <thread>
#include <cstdint>
#include <string>

#include "shared.h"
#include "hooks.h"
#include "gui.h"

void Error(const char* message, ...)
{
	char buffer[1024];
	va_list va;
	va_start(va, message);
	vsprintf_s(buffer, 1024, message, va);
	MessageBoxA(NULL, message, PROJECT_NAME, MB_ICONERROR);
}

inline void Sleep(int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void Setup(const HMODULE instance)
{
	if (hooks::Setup())
	{		
		while (!exitMainLoop)
		{
			Sleep(200);
		}
	}

	hooks::Destroy();
	gui::Destroy();

	FreeLibraryAndExitThread(instance, 0);
}

BOOL APIENTRY DllMain(HMODULE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		if (!strncmp(reinterpret_cast<char*>(0x9F03D5), ":33", 3))
		{
			DisableThreadLibraryCalls(instance);

			const auto thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Setup), instance, 0, nullptr);
			if (thread)
			{
				CloseHandle(thread);
			}
		}
		else
		{
			Error("Only the v1.4 English nfsc.exe (6,88 MB (7.217.152 bytes)) is supported.");
			return FALSE;
		}
	}

	return TRUE;
}