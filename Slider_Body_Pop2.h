#pragma once

namespace slider_body_pop2 {

	// ((first4 * 3u) & 0x6cu) * 4
	// best cpu cache line adjustment i could find - uses 4 cache lines two effective LEA
	// works if first4 has only 2 bits or more too.

	//CACHE 0
		// 3,3
		// 1,1
	 //CACHE 2
		// 2,1
		// 1,2
	//CACHE 4
		// 3,2
		// 2,3
	//CACHE 6
		// 3,1
		// 1,3
		// 2,2

	alignas(64) inline constexpr auto POINT_SINGLE_SHUF_DELIM2 = [] {


		struct alignas(16) _validation {
			u32 mask;
			u8 padding0[12];
		};

		struct _ret {

			alignas(64) std::array<std::array<u8, 16>, 28> table{};
			alignas(64) std::array<_validation, 28> validation;

		} output{};

		for (auto& entry : output.table)
			for (auto& b : entry)
				b = 0x80;

		for (auto& vali : output.validation)
			vali.mask = u32(-1);

		auto write = [&output](u32 mask, u32 x, u32 y) {

			const u32 key = (mask * 3u) & 0x6cu;

			const u32 byte_offset = key * 4u;
			const u32 slot = byte_offset / 16u;

			output.validation[slot].mask = mask;

			auto& s = output.table[slot];

			const u32 y_start = x + 1;

			for (u32 i{}; i < x; ++i)
				s[4 - x + i] = u8(i);

			for (u32 i{}; i < y; ++i)
				s[8 - y + i] = u8(y_start + i);

		};

		write(0x0A, 1, 1);
		write(0x12, 1, 2);
		write(0x22, 1, 3);

		write(0x14, 2, 1);
		write(0x24, 2, 2);
		write(0x44, 2, 3);

		write(0x28, 3, 1);
		write(0x48, 3, 2);
		write(0x88, 3, 3);

		return output;
	}();

}
