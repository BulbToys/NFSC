#pragma once

namespace nfsc {
	void Setup() noexcept;

	inline int gameState;

	inline void (*Game_ForceAIControl)(int);
	inline void (*Game_ClearAIControl)(int);
}