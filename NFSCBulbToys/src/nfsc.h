#pragma once

namespace nfsc
{
	inline int gameState;

	inline void (*Game_ForceAIControl)(int) = reinterpret_cast<void(*)(int)>(0x65C2C0);
	inline void (*Game_ClearAIControl)(int) = reinterpret_cast<void(*)(int)>(0x65C330);
}