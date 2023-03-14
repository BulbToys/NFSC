#include "nfsc.h"
#include "shared.h"

void nfsc::Setup() noexcept {
	Game_ForceAIControl = reinterpret_cast<void(*)(int)>(basePtr + 0x25C2C0);
	Game_ClearAIControl = reinterpret_cast<void(*)(int)>(basePtr + 0x25C330);
}