#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <vector>


std::uint8_t* PatternScan(const char* module_name, const char* signature) noexcept {
	const auto module_handle = GetModuleHandleA(module_name);

	if (!module_handle)
		return nullptr;

	static auto pattern_to_byte = [](const char* pattern) {
		auto bytes = std::vector<int>{};
		auto start = const_cast<char*>(pattern);
		auto end = const_cast<char*>(pattern) + std::strlen(pattern);

		for (auto current = start; current < end; ++current) {
			if (*current == '?') {
				++current;

				if (*current == '?')
					++current;

				bytes.push_back(-1);
			}
			else {
				bytes.push_back(std::strtoul(current, &current, 16));
			}
		}
		return bytes;
		};

	auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
	auto nt_headers =
		reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

	auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
	auto pattern_bytes = pattern_to_byte(signature);
	auto scan_bytes = reinterpret_cast<std::uint8_t*>(module_handle);

	auto s = pattern_bytes.size();
	auto d = pattern_bytes.data();

	for (auto i = 0ul; i < size_of_image - s; ++i) {
		bool found = true;

		for (auto j = 0ul; j < s; ++j) {
			if (scan_bytes[i + j] != d[j] && d[j] != -1) {
				found = false;
				break;
			}
		}
		if (found)
			return &scan_bytes[i];
	}

	throw std::runtime_error(std::string("Wrong signature: ") + signature);
}


#define INVALID_EHANDLE_INDEX 0xFFFFFFFF
#define ENT_ENTRY_MASK 0x7FFF
#define NUM_SERIAL_NUM_SHIFT_BITS 15
#define ENT_MAX_NETWORKED_ENTRY 16384

class CBaseHandle
{
public:
	CBaseHandle() :
		nIndex(INVALID_EHANDLE_INDEX) { }

	CBaseHandle(const int nEntry, const int nSerial)
	{
		if (nEntry < 0 || (nEntry & ENT_ENTRY_MASK) != nEntry)
		{
			// we can handle the error here
			return;
		}
		if (nSerial < 0 || nSerial >= (1 << NUM_SERIAL_NUM_SHIFT_BITS))
		{
			// we can handle the error here
			return;
		}

		nIndex = nEntry | (nSerial << NUM_SERIAL_NUM_SHIFT_BITS);
	}
	bool IsValid()
	{
		return nIndex != INVALID_EHANDLE_INDEX;
	}

	int GetEntryIndex()
	{
		return static_cast<int>(nIndex & ENT_ENTRY_MASK);
	}
private:
	std::uint32_t nIndex;
};

class CBaseEntity {
public:
	/// Insert values from Entity
};

class Pawn
{
public:
	/// Insert values for Pawn
};

class Controller {
public:

	CBaseHandle GetHandle()
	{
		return *reinterpret_cast<CBaseHandle*>(this + 0x5F4);;
	}
};


class IGameEntityList {
public:
	template <typename T = CBaseEntity>
	T* GetEntity(int Index)
	{
		return reinterpret_cast<T*>(this->pGetBaseEntity(Index));
	}

	template <typename T = CBaseEntity>
	T* GetEntity(CBaseHandle hHandle)
	{
		if (!hHandle.IsValid())
			return nullptr;

		return reinterpret_cast<T*>(this->pGetBaseEntity(hHandle.GetEntryIndex()));
	}

private:
	void* pGetBaseEntity(int Index) {
		using fnGetBaseEntity = void* (__thiscall*)(void*, int);
		static auto GetBaseEntity = reinterpret_cast<fnGetBaseEntity>(PatternScan("client.dll", "81 FA ? ? ? ? 77 ? 8B C2 C1 F8 ? 83 F8 ? 77 ? 48 98 48 8B 4C C1 ? 48 85 C9 74 ? 8B C2 25 ? ? ? ? 48 6B C0 ? 48 03 C8 74 ? 8B 41 ? 25 ? ? ? ? 3B C2 75 ? 48 8B 01"));
		return GetBaseEntity(this, Index);
	}
};

std::uint8_t* ResolveRip(std::uint8_t* address, std::uint32_t rva_offset, std::uint32_t rip_offset)
{
	if (!address || !rva_offset || !rip_offset)
	{
		return nullptr;
	}

	std::uint32_t rva = *reinterpret_cast<std::uint32_t*>(address + rva_offset);
	std::uint64_t rip = reinterpret_cast<std::uint64_t>(address) + rip_offset;

	return reinterpret_cast<std::uint8_t*>(rva + rip);
}

DWORD WINAPI EntryPoint(void* param) {
	
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);


	IGameEntityList* pEntityList = nullptr;

	pEntityList = *reinterpret_cast<IGameEntityList**>(ResolveRip(PatternScan("client.dll", "48 8B 0D ? ? ? ? 48 8D 94 24 ? ? ? ? 33 DB"), 3, 7));

	std::cout << "Press insert to dump entities\n";


	while (!GetAsyncKeyState(VK_DELETE))
	{
		if (GetAsyncKeyState(VK_INSERT) & 1) {
			for (int i = 0; i <= 64; i++)
			{
				CBaseEntity* entity = pEntityList->GetEntity(i);
				if (!entity)
					continue;
				Controller* controller = (Controller*)entity;
				if (!controller)
					continue;
				Pawn* pawn =  (Pawn*)pEntityList->GetEntity(controller->GetHandle());
				if (!pawn)
					continue;

				std::cout<<"["<<i<<"]"<<" Pawn -> "<<pawn<<" Controller -> "<<controller<<" Entity -> "<<entity<<"\n";

			}
		}
	}
		Sleep(150);


		if(fDummy)
		fclose(fDummy);
		FreeConsole();


	FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(param), 0);
}


BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		auto handle = CreateThread(nullptr, 0, EntryPoint, hModule, 0, nullptr);
		if (!handle)
			return FALSE;
		CloseHandle(handle);
	}
	return TRUE;
}

