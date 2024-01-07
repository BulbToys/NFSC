#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "shared.h"

// TODO: Everything here is thread unsafe as fuck. Rewrite rewrite rewrite!!!
void Setup(const HMODULE instance)
{
	Patches::Do();

	if (!NFSC::BulbToys_IsGameNFSCO())
	{
		GUI::debug_shortcut = true;
	}

	if (Hooks::Setup())
	{
		while (!exitMainLoop)
		{
			Sleep(200);
		}
	}

	Hooks::Destroy();
	GUI::Destroy();
	Patches::Undo();

	/*
	size_t size = patch_map.size();
	if (size > 0)
	{
		Error("Patch map has %u leftover patch(es).", size);
		ASSERT(0);
	}
	*/

	FreeLibraryAndExitThread(instance, 0);
}

BOOL APIENTRY DllMain(HMODULE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		// Version check :3
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