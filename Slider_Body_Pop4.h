#pragma once

namespace slider_body_pop4 {

	// cant seem to crack a good formula for this
	// 128 with simple compute is a no go, LEA is useless
	// simple 256 (only 81 possible inputs so, gross) with minor shift to better align the most common into joined lanes...
	// 4k... an entire page...

	alignas(64) inline constexpr auto POINT_PAIR_SHUF_DELIM4 = [] {

		struct alignas(64) pop4_table_storage {
			std::array<u8, 16> padding0;
			std::array<std::array<u8, 16>, 256> table;
		} table{};

		for (auto& entry : table.table)
			for (auto& b : entry)
				b = 0x80;

		constexpr auto make_mask = [](u32 x0, u32 y0, u32 x1, u32 y1) {

			const auto d0 = x0;
			const auto d1 = x0 + y0 + 1;
			const auto d2 = x0 + y0 + x1 + 2;
			const auto d3 = x0 + y0 + x1 + y1 + 3;

			return (1u << d0) | (1u << d1) | (1u << d2) | (1u << d3);
		};

		for (u32 x0{ 1 }; x0 < 4; ++x0) {
			for (u32 y0{ 1 }; y0 < 4; ++y0) {
				for (u32 x1{ 1 }; x1 < 4; ++x1) {
					for (u32 y1{ 1 }; y1 < 4; ++y1) {

						const u32 digit_count = x0 + y0 + x1 + y1;

						//if (digit_count < 9)
						//    continue;

						const u32 delim_mask = make_mask(x0, y0, x1, y1);

						const u32 key = ((delim_mask * 27151u) >> 5u) & 0xff0u;

						auto& s = table.table[key >> 4];

						const u32 colon0 = x0;
						const u32 pipe0 = x0 + y0 + 1;
						const u32 colon1 = x0 + y0 + x1 + 2;

						const u32 x1_start = pipe0 + 1;
						const u32 y0_start = colon0 + 1;
						const u32 y1_start = colon1 + 1;

						for (u32 i{}; i < x0; ++i)
							s[4 - x0 + i] = u8(i);

						for (u32 i{}; i < y0; ++i)
							s[8 - y0 + i] = u8(y0_start + i);

						for (u32 i{}; i < x1; ++i)
							s[12 - x1 + i] = u8(x1_start + i);

						for (u32 i{}; i < y1; ++i)
							s[16 - y1 + i] = u8(y1_start + i);
					}
				}
			}
		}

		return table;
	}();

}
