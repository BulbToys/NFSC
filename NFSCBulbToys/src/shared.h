#pragma once
#include <cstdint>

#define PROJECT_NAME "NFSC Bulb Toys"

#define MENU_KEY VK_F9

inline bool exitMainLoop = false;

namespace needs_encounter
{
	inline bool hooked = false;
	inline bool value = false;
	inline bool overridden = false;
}

namespace needs_traffic
{
	inline bool hooked = false;
	inline bool value = false;
	inline bool overridden = false;
}

namespace gps_engage
{
	inline bool hooked = false;
	inline void* myAIVehicle = nullptr;
}

void Error(const char* message, ...);
inline void Sleep(int ms);

template <typename T>
inline T ReadMemory(size_t address)
{
	return *reinterpret_cast<T*>(address);
}

template <typename T>
inline void WriteMemory(size_t address, T value)
{
	T* memory = reinterpret_cast<T*>(address);
	*memory = value;
}

inline void WriteNop(size_t address, int count = 1)
{
	for (int i = 0; i < count; i++)
	{
		WriteMemory<unsigned char>(address + i, 0x90);
	}
}