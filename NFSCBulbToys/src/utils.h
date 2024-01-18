#pragma once

/* === Main project stuff === */

#define PROJECT_NAME "NFSC Bulb Toys"

#define MENU_KEY VK_F9

#define DIE() *((int*)0xDEAD) = 0
#define ASSERT(cond) do { if (!(cond)) { Error("Assertion failed: " #cond); DIE(); } } while (false)

inline bool exitMainLoop = false;

// todo move me
inline void Error(const char* message, ...)
{
	char buffer[1024];
	va_list va;
	va_start(va, message);
	vsprintf_s(buffer, 1024, message, va);

	MessageBoxA(NULL, buffer, PROJECT_NAME, MB_ICONERROR);
}

/* ===== Structs ===== */

// Interface for structs/classes that can be saved to files
// For game structs/classes, make an adapter class first (ie. RoadblockSetupFile contains RoadblockSetup)
struct IFileBase
{
	virtual size_t Size() const = 0;

	// Return false if struct/class data is bogus, true otherwise. Returning false prompts a warning when saving/loading
	virtual bool Validate() = 0;

	virtual void Save(const char* filename) final
	{
		size_t size = Size();

		if (!Validate())
		{
			char msg[512];
			sprintf_s(msg, 512, "Error saving file %s.\n\n"
				"The object you are trying to save has failed to validate, indicating it contains invalid (corrupt or otherwise unsafe) values.\n\nProceed?", filename);

			if (MessageBoxA(NULL, msg, PROJECT_NAME, MB_ICONWARNING | MB_YESNO) != IDYES)
			{
				return;
			}
		}

		// Avoid saving the vtable pointer
		size -= 4;

		FILE* file = nullptr;
		fopen_s(&file, filename, "wb");
		if (!file)
		{
			char error[64];
			strerror_s(error, errno);
			Error("Error saving file %s.\n\nError code %d: %s", filename, errno, error);
		}
		else
		{
			// Avoid saving the vtable pointer
			fwrite((char*)this + 4, 1, size, file);
			fclose(file);
		}
	}

	// Object remains unchanged if this returns false!
	virtual bool Load(const char* filename, bool allow_undersize = false) final
	{
		size_t size = Size();

		// Offset from vtable pointer
		size -= 4;

		FILE* file = nullptr;
		fopen_s(&file, filename, "rb");
		if (!file)
		{
			char error[64];
			strerror_s(error, errno);
			Error("Error opening file %s.\n\nError code %d: %s", filename, errno, error);
			return false;
		}
		else
		{
			char* buffer = new char[size];

			fseek(file, 0, SEEK_END);
			int len = ftell(file);
			fseek(file, 0, SEEK_SET);

			if (len > size || (!allow_undersize && len < size))
			{
				fclose(file);
				Error("Error opening file %s.\n\nInvalid file length - expected %d, got %d.", filename, size, len);
				return false;
			}

			fread_s(buffer, len, 1, len, file);
			fclose(file);

			if (!Validate())
			{
				char msg[512];
				sprintf_s(msg, 512, "Error opening file %s.\n\n"
					"The object you are trying to load has failed to validate, indicating it contains invalid (corrupt or otherwise unsafe) values.\n\nProceed?", filename);

				if (MessageBoxA(NULL, msg, PROJECT_NAME, MB_ICONWARNING | MB_YESNO) != IDYES)
				{
					return false;
				}
			}

			// Offset from vtable pointer
			memcpy((char*)this + 4, buffer, len);
		}

		return true;
	}
};

template <typename T>
struct IFile : IFileBase
{
	virtual size_t Size() const override final { return sizeof(T); }
};

struct PatchInfo
{
	char* bytes = nullptr;
	size_t len = 0;

	PatchInfo(uintptr_t address, size_t len) : len(len)
	{
		bytes = new char[len];
		memcpy(bytes, reinterpret_cast<void*>(address), len);
	}

	~PatchInfo()
	{
		delete[] bytes;
	}
};
inline std::unordered_map<uintptr_t, PatchInfo*> patch_map;

/* ===== Helper functions ===== */

template <typename T>
inline T Read(uintptr_t address)
{
	return *reinterpret_cast<T*>(address);
}

template <typename T>
inline void Write(uintptr_t address, T value)
{
	*reinterpret_cast<T*>(address) = value;
}

// Identical to Write, except it gets added to the patch map (for later unpatching)
template <typename T>
inline void Patch(uintptr_t address, T value)
{
	PatchInfo* p = new PatchInfo(address, sizeof(T));
	patch_map.insert({ address, p });

	T* memory = reinterpret_cast<T*>(address);
	*memory = value;
}

inline void PatchNOP(uintptr_t address, int count = 1)
{
	PatchInfo* p = new PatchInfo(address, count);
	patch_map.insert({ address, p });

	memset(reinterpret_cast<void*>(address), 0x90, count);
}

// !!! ASM FUNCTIONS ONLY !!!
// NOTE: The jump instruction is 5 bytes
inline void PatchJMP(uintptr_t address, void* asm_func, size_t patch_len = 5)
{
	ASSERT(patch_len >= 5);

	PatchInfo* p = new PatchInfo(address, patch_len);
	patch_map.insert({ address, p });

	ptrdiff_t relative = reinterpret_cast<uintptr_t>(asm_func) - address - 5;

	// Write the jump instruction
	Write<uint8_t>(address, 0xE9);

	// Write the relative address to jump to
	Write<ptrdiff_t>(address + 1, relative);

	// Write nops until we've reached length
	memset(reinterpret_cast<void*>(address + 5), 0x90, patch_len - 5);
}

// Iterates function in memory until it finds "int 3" - opcode 0xCC
// Usually, this catches the first 0xCC "align 0x10" byte, but it WILL break for functions that are (len % 0x10) long
// For best results, ONLY pass functions that are guaranteed to end with "int 3" (such as ASM)
inline size_t ScuffedASMLength(void* asm_func)
{
	size_t len = 0;
	while (*(char*)asm_func != 0xCC)
	{
		len++;
	}
	return len;
}

// !!! ASM FUNCTIONS ONLY !!!
// Do NOT use with ASM functions that contain relative offsets (such as non-external (static/game) calls)
inline void PatchASM(uintptr_t address, void* asm_func, size_t patch_len)
{
	size_t func_len = ScuffedASMLength(asm_func);
	ASSERT(func_len <= patch_len);

	PatchInfo* p = new PatchInfo(address, patch_len);
	patch_map.insert({ address, p });

	memcpy(reinterpret_cast<void*>(address), asm_func, func_len);

	// Write nops until we've reached length
	memset(reinterpret_cast<void*>(address + func_len), 0x90, patch_len - func_len);
}

inline void Unpatch(uintptr_t address, bool low_priority = false)
{
	if (patch_map.find(address) == patch_map.end())
	{
		if (!low_priority)
		{
			Error("Tried to Unpatch() non-existent patch %08X.", address);
			DIE();
		}

		// Low priority unpatching - ignore non-existent patch error
		// Only use for dynamic patches such as No Busted
		// For static patches, it's better not to ignore these errors
		return;
	}

	PatchInfo* p = patch_map.at(address);
	patch_map.erase(address);

	memcpy(reinterpret_cast<void*>(address), p->bytes, p->len);
	delete p;
}

// Returns THE ADDRESS to the virtual function at <index> in the virtual table of this_
template <int index>
inline uintptr_t Virtual(uintptr_t this_)
{
	return Read<uintptr_t>(Read<uintptr_t>(this_) + index * 4);
}

// Returns A POINTER TO THE ADDRESS of the virtual function at <index> in the virtual table of this_
template <int index>
inline uintptr_t PtrVirtual(uintptr_t this_)
{
	return Read<uintptr_t>(this_) + index * 4;
}

// TODO: Print addy where the purecall occurred. Not easy without some stack fuckery. C++23 <stacktrace> maybe?
inline __declspec(noreturn) void PurecallHandler()
{
	/*
	uintptr_t stack[1];
	Error("Pure virtual function call at %08X.", stack[-1]);
	DIE();
	*/

	Error("Pure virtual function call.");
	DIE();
}

/* ===== Miscellaneous utility functions ===== */

// !!! Length MUST match for both strings !!!
inline void ScuffedStringToWide(const char* string, wchar_t* wide, size_t len = 0xFFFFFFFF)
{
	char* wide_multi = reinterpret_cast<char*>(wide);

	for (int i = 0; i < len; i++)
	{
		wide_multi[i * 2] = string[i];
		wide_multi[i * 2 + 1] = '\0';

		// Null character - assume we're done
		if (string[i] == '\0')
		{
			return;
		}
	}
}