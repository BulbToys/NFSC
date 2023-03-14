#pragma once
#include <cstdint>

#define PROJECT_NAME "NFSC Bulb Toys"

#define MENU_KEY VK_F9
#define PANIC_KEY VK_F13

inline uintptr_t basePtr = 0;

inline void Error(const char* message) noexcept;

template <typename T>
inline T ReadMemory(int offset) {
	return *reinterpret_cast<T*>(basePtr + offset);
}

template <typename T>
inline void WriteMemory(int offset, T value) {
	T* memory = reinterpret_cast<T*>(basePtr + offset);
	*memory = value;
}