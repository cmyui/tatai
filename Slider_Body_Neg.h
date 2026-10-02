#pragma once
namespace slider_body_neg {

	struct alignas(32) _NEGATIVE_INFO {

		std::array<u8, 16> shuf;

		u32 consumed;
		u8 padding0[4];
		int sign_x, sign_y;

	};

	static_assert(sizeof(_NEGATIVE_INFO) == 32);
	

	constexpr auto NEGATIVE_INFO = [] {

		struct _negative_info_tables {
			std::array<_NEGATIVE_INFO, 128> table{};
			std::array<u32, 128> validation{};
		} output{};

			for (auto& v : output.table)
				v.consumed = (u32(16) << 24) | 1;
			for (auto& v : output.validation)
				v = u32(-1);


			for (u32 x_len{ 1 }; x_len < 4; ++x_len) {
				for (u32 y_len{ 1 }; y_len < 4; ++y_len) {
					for (u32 x_neg{}; x_neg < 2; ++x_neg) {
						for (u32 y_neg{}; y_neg < 2; ++y_neg) {

							if (!x_neg && !y_neg)
								continue;

							const u32 x_start = x_neg;

							const u32 colon = x_neg + x_len;

							const u32 y_start = colon + 1u + y_neg;

							const u32 end = y_start + y_len;

							const u32 first2 = (1u << colon) | (1u << end);

							u32 neg_bits = 0;

							if (x_neg)
								neg_bits |= 1u;

							if (y_neg)
								neg_bits |= 1u << (colon + 1u);

							const u32 key_in{ first2 | neg_bits };

							const u32 key = pext_constexpr(key_in, 0b110111101);

							output.validation[key] = key_in;

							auto& e = output.table[key];

							for (auto& v : e.shuf)
								v = 0x80;

							const u32 x_dst = 4u - x_len;

							for (u32 i{}; i < x_len; ++i)
								e.shuf[x_dst + i] = u8(x_start + i);

							const u32 y_dst = 8u - y_len;

							for (u32 i{}; i < y_len; ++i)
								e.shuf[y_dst + i] = u8(y_start + i);

							e.consumed = (u32(end + 1u) << 24) | 1; // 1 is here to implicitly return +1

							e.sign_x = x_neg ? -1 : 1;
							e.sign_y = y_neg ? -1 : 1;

							//e.sign_bit = x_neg | (y_neg << 1);

						}
					}
				}
			}

			return output;
		}();

	__declspec(noinline) u32 parse_slider_point_negative(const char*__restrict p, slider_point* __restrict const out, const u32 in, const u32 com) noexcept {

		const auto input = _mm_loadu_si128((const __m128i*)p);
		const auto digits = _mm_sub_epi8(input, _mm_set1_epi8('0'));

		const u32 first2 = u16(in);

		const u32 neg_bits = in >> 16u;
		const u32 key_in{ first2 | neg_bits };

		const auto key = (u32)_pext_u32(key_in, 0b000110111101u);

		const auto& info = NEGATIVE_INFO.table[key];

		if (NEGATIVE_INFO.validation[key] != key_in) [[unlikely]] {

			return 0;
		}

		const auto shuff =_mm_shuffle_epi8(digits, _mm_load_si128((const __m128i*)info.shuf.data()));

		const auto signs = _mm_loadl_epi64((__m128i const*)(&info.sign_x));

		const auto inter0 =_mm_maddubs_epi16(shuff, _mm_setr_epi8(0, 10, 10, 1, 0, 10, 10, 1, 0, 0, 0, 0, 0, 0, 0, 0));

		const u32 ret = info.consumed | (((first2 & com) << 8));

		const auto values = _mm_madd_epi16(inter0, _mm_setr_epi16(10, 1, 10, 1, 0, 0, 0, 0));

		_mm_storeu_si64((__m128i*)out, _mm_sign_epi32(values, signs));

		return ret;
	}

}