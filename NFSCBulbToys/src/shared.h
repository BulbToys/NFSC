#pragma once
#include <cstdint>

#define PROJECT_NAME "NFSC Bulb Toys"

#define MENU_KEY VK_F9

inline bool exitMainLoop = false;

void Error(const char* message, ...);
inline void Sleep(int ms);

template <typename T>
inline T ReadMemory(int address)
{
	return *reinterpret_cast<T*>(address);
}

template <typename T>
inline void WriteMemory(int address, T value)
{
	T* memory = reinterpret_cast<T*>(address);
	*memory = value;
}