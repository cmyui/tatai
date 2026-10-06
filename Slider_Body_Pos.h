#pragma once

namespace slider_body_positive {

	constexpr u32 ENTRY_COUNT = 256;
	constexpr u32 ENTRY_SIZE = 32;

	struct alignas(32) slider_lookup_entry {

		u64 validation{};
		u64 padding0{};
		std::array<u8, 16> shuffle{};

	}; static_assert(sizeof(slider_lookup_entry) == 32);


	struct generated_tables {
		alignas(64) std::array<slider_lookup_entry, ENTRY_COUNT> entry{};
	};

	constexpr u32 make_slot(const u32 effective_bits) noexcept {
		return (((effective_bits * 480925u) >> 11u) & 0xff0u) >> 4u;
	}

	constexpr void write_integer_shuffle( slider_lookup_entry& e,
		const u32 group, const u32 src_start, const u32 digit_count) noexcept {

		const u32 dst = group * 4u;

		e.shuffle[dst + 0] = 0x80u;
		e.shuffle[dst + 1] = 0x80u;
		e.shuffle[dst + 2] = 0x80u;
		e.shuffle[dst + 3] = 0x80u;

		const u32 first_dst = dst + (4u - digit_count);

		for (u32 i{}; i < digit_count; ++i)
			e.shuffle[first_dst + i] = u8(src_start + i);

	}

	constexpr void write_validation(slider_lookup_entry& e,
		const u32 effective_bits, const u32 point_count, const u32 consumed) noexcept {

		const u32 return_meta = point_count | ((consumed + 1u) << 24u);

		e.validation = u64(effective_bits) | (u64(return_meta) << 32u);

	}

	constexpr generated_tables generate_tables() {

		generated_tables out{};

		std::array<bool, ENTRY_COUNT> used{};

		for (u32 i{}; i < ENTRY_COUNT; ++i) {
			for (auto& v : out.entry[i].shuffle)
				v = 0x80u;
		}

		// POP2
		for (u32 xd = 1; xd <= 3; ++xd) {

			for (u32 yd = 1; yd <= 3; ++yd) {

				const u32 colon = xd;

				const u32 comma = colon + 1u + yd;

				const u32 effective_bits = (1u << colon) | (1u << comma);

				const u32 slot = make_slot(effective_bits);

				if (used[slot])
					return { 0ull / 0 };

				used[slot] = true;

				auto& e = out.entry[slot];

				write_validation(e, effective_bits, 1u, comma);
				write_integer_shuffle(e, 0u, 0u, xd);
				write_integer_shuffle(e, 1u, colon + 1u, yd);

			}

		}

		// POP4
		for (u32 x0d = 1; x0d <= 3; ++x0d) {

			for (u32 y0d = 1; y0d <= 3; ++y0d) {

				for (u32 x1d = 1; x1d <= 3; ++x1d) {

					for (u32 y1d = 1; y1d <= 3; ++y1d) {

						const u32 colon0 = x0d;

						const u32 pipe = colon0 + 1u + y0d;

						const u32 colon1 = pipe + 1u + x1d;

						const u32 end = colon1 + 1u + y1d;

						const u32 effective_bits =
							(1u << colon0) |
							(1u << pipe) |
							(1u << colon1) |
							(1u << end);

						const u32 slot = make_slot(effective_bits);

						if (used[slot])
							return {1ull/0};

						used[slot] = true;

						auto& e = out.entry[slot];

						write_validation(e, effective_bits, 2u, end);
						write_integer_shuffle(e, 0u, 0u, x0d);
						write_integer_shuffle(e, 1u, colon0 + 1u, y0d);
						write_integer_shuffle(e, 2u, pipe + 1u, x1d);
						write_integer_shuffle(e, 3u, colon1 + 1u, y1d);

					}

				}

			}

		}

		return out;
	}


	inline constexpr auto TABLES = generate_tables();

	inline constexpr const auto& TABLE = TABLES.entry;

}