#pragma once

namespace object_pair {

	struct alignas(32) entry {
		u8 consumed{};
		alignas(16) std::array<u8, 16> shuffle{};
	};

	template<u32 Width>
	inline constexpr auto TABLE = [] {
		std::array<entry, 65536> out{};
		for (u32 x = 1; x <= 3; ++x) {
			for (u32 y = 1; y <= 3; ++y) {
				const u32 time_end = x + y + Width + 2;
				const u32 type_end = time_end + 2;
				const u32 fixed = (1u << x) | (1u << (x + y + 1)) | (1u << time_end);
				const u32 end_bit = type_end < 16 ? 1u << type_end : 0;
				const u32 suffix_start = std::min(type_end + 1, 16u);
				for (u32 suffix = 0; suffix < (1u << (16 - suffix_start)); ++suffix) {
					auto& e = out[fixed | end_bit | (suffix << suffix_start)];
					e.consumed = u8(type_end + 1);
					if constexpr (Width == 5)
						e.shuffle = parse_5_time::SHUF_TBL5[((x - 1) * 3 + y - 1) * 3];
					else
						e.shuffle = parse_6_time::SHUF_TBL6[((x - 1) * 3 + y - 1) * 3];
				}
			}
		}
		return out;
	}();

	// Only the common one-digit type shape takes this path. All other shapes
	// use the existing decoders, before any output or deferral is changed.
	template<u32 Width>
	__forceinline bool parse(const char*& p0, const char*& p1, _object_header* __restrict out) {
		const auto raw = _mm256_inserti128_si256(
			_mm256_castsi128_si256(_mm_loadu_si128((const __m128i*)p0)),
			_mm_loadu_si128((const __m128i*)p1), 1);
		const u32 commas = _mm256_movemask_epi8(_mm256_cmpeq_epi8(raw, _mm256_set1_epi8(',')));
		const auto& a = TABLE<Width>[u16(commas)];
		const auto& b = TABLE<Width>[commas >> 16];
		const u32 c0 = a.consumed, c1 = b.consumed;
		if (!c0 || !c1) return false;
		if constexpr (Width == 6)
			if (p0[c0 - 1] != ',' || p1[c1 - 1] != ',') return false;
		const auto digits = _mm256_sub_epi8(raw, _mm256_set1_epi8('0'));
		if (u32(_mm256_movemask_epi8(digits)) & ~commas) return false;
		const auto shuffle = _mm256_inserti128_si256(
			_mm256_castsi128_si256(_mm_load_si128((const __m128i*)a.shuffle.data())),
			_mm_load_si128((const __m128i*)b.shuffle.data()), 1);
		const auto pack = _mm256_shuffle_epi8(digits, shuffle);
		const auto m0 = Width == 5
			? _mm256_setr_epi8(10,1,0,1,10,1,0,1,10,1,10,1,10,1,0,1, 10,1,0,1,10,1,0,1,10,1,10,1,10,1,0,1)
			: _mm256_setr_epi8(0,1,10,1,10,1,0,1,10,1,10,1,10,1,10,1, 0,1,10,1,10,1,0,1,10,1,10,1,10,1,10,1);
		const auto m1 = Width == 5
			? _mm256_setr_epi16(10,1,10,1,100,1,1,256, 10,1,10,1,100,1,1,256)
			: _mm256_setr_epi16(100,1,10,1,100,1,1,256, 100,1,10,1,100,1,1,256);
		const auto i0 = _mm256_maddubs_epi16(pack, m0);
		const auto i1 = _mm256_madd_epi16(i0, m1);
		const auto i2 = _mm256_shuffle_epi8(i1, _mm256_setr_epi8(
			0,1,-1,-1,4,5,-1,-1,8,9,13,-1,12,-1,-1,-1, 0,1,-1,-1,4,5,-1,-1,8,9,13,-1,12,-1,-1,-1));
		constexpr u32 time_scale = Width == 5 ? 10 : 100;
		const auto result = _mm256_min_epu32(
			_mm256_madd_epi16(i2, _mm256_setr_epi16(1,0,1,0,time_scale,1,1,0, 1,0,1,0,time_scale,1,1,0)),
			_mm256_setr_epi32(512,512,-1,-1,512,512,-1,-1));
		_mm256_storeu_si256((__m256i*)out, result);
		p0 += c0;
		p1 += c1;
		return true;
	}
}
