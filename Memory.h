#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma comment(lib, "OneCore.lib")
#include <memoryapi.h>
#undef min
#undef max

constexpr size_t POINTER_RESET_MASK = ~((1 << 29) - 1);

namespace byte_allocator {

	static constexpr size_t MEM_MAX = 512ull * 1024ull * 1024ull; // 512mb
	static constexpr size_t CHUNK_SIZE = 512ull * 1024ull; // 512kb - 128 pages of 4k

	__forceinline bool need_resize(size_t size, u32& bytes_alloc) {

		if (size <= bytes_alloc) [[likely]]
			return true;

		return false;
	}

	inline void* resize(size_t size, void* data, u32& bytes_alloc) {

		if (size <= bytes_alloc) [[likely]]
			return data;

		const size_t needed_size = (size + CHUNK_SIZE - 1u) & ~(CHUNK_SIZE - 1u);

		if (needed_size > MEM_MAX)
			return nullptr;

		const size_t new_size = needed_size - bytes_alloc;

		const void* result = VirtualAlloc((u8*)data + bytes_alloc, new_size, MEM_COMMIT, PAGE_READWRITE);

		if (!result)
			return nullptr;

		bytes_alloc = needed_size;

		return data;
	}

	__declspec(noinline) void* NOINLINE_resize(size_t size, void* data, u32& bytes_alloc) {

		return resize(size, data, bytes_alloc);
	}

	inline void* reserve(u32& bytes_alloc) {

		bytes_alloc = 0;

		return resize(CHUNK_SIZE, VirtualAlloc(nullptr, MEM_MAX, MEM_RESERVE, PAGE_NOACCESS), bytes_alloc);
	}

	template<size_t i>
	inline void* reserve_aligned(u32& bytes_alloc) {

		bytes_alloc = 0;

		MEM_ADDRESS_REQUIREMENTS mer{
			.Alignment = i
		};

		MEM_EXTENDED_PARAMETER param{
			.Type = MemExtendedParameterAddressRequirements,
			.Pointer = &mer
		};

		auto* r = VirtualAlloc2(GetCurrentProcess(), nullptr, MEM_MAX, MEM_RESERVE, PAGE_NOACCESS, &param, 1);

		return resize(CHUNK_SIZE, r, bytes_alloc);
	}

};