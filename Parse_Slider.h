#pragma once

#include "Slider_Body_Neg.h"
#include "Slider_Body_Pop2.h"
//#include "Slider_Body_Pop4.h"
#include "Slider_Body_Pos.h"

__forceinline void parse_slider_pair_GENERAL(const __m128i m0, const __m128i shuffle, _slider_point *const out) noexcept {

	const auto d = _mm_shuffle_epi8(m0, shuffle);

	const auto pairs = _mm_maddubs_epi16(d, _mm_setr_epi8(0, 10, 10, 1, 0, 10, 10, 1, 0, 10, 10, 1, 0, 10, 10, 1));

	const auto result = _mm_madd_epi16(pairs, _mm_setr_epi16(10, 1, 10, 1, 10, 1, 10, 1));

	_mm_storeu_si128((__m128i *)out, result);

}

__forceinline u32 parse_two_slider_points(const char* __restrict p, _slider_point* const __restrict out) {

    // for objects outside the digit range of 1-3
    //      example: 0:1234
    // returning 0 puts this slider into a deferred list to be recomputed using a general parser

	const auto m0 = _mm_loadu_si128((const __m128i*)p);

	const auto X = (u32)_mm_movemask_epi8(
		_mm_shuffle_epi8(_mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			(char)-1, // :
			0,
			(char)-1, // |,
			(char)-1, // -
			0, 0),
			m0));

	const auto comma_xmm = _mm_cmpeq_epi8(m0, _mm_set1_epi8(','));
    const auto digits = _mm_sub_epi8(m0, _mm_set1_epi8('0'));

	const auto first4 = (u32)_pdep_u32(0b1111u, X);

    const auto commas = (u32)_mm_movemask_epi8(comma_xmm);

	if (const u32 negative = first4 & ((first4 << 1u) | 1u); negative) [[unlikely]] {

        const u32 sep = X & ~negative;

        const auto first2 = (u32)_pdep_u32(0b11, sep);

        const auto end = (u32)_blsi_u32(_blsr_u32(first2));

        const u32 point_negative = negative & (end - 1u);

        if (point_negative)
            return slider_body_neg::parse_slider_point_negative(p, out,
                first2 | (point_negative << 16u), commas);
        
        const u32 key = (first2 * 3u) & 0x6Cu;

        {
            const auto* tbl_vali = (const u32*)(slider_body_pop2::POINT_SINGLE_SHUF_DELIM2.validation.data());

            if (*(tbl_vali + key) != first2) [[unlikely]] {
                return 0;
            }
        }

        const auto* tbl = (const u32*)(slider_body_pop2::POINT_SINGLE_SHUF_DELIM2.table.data());

        parse_slider_pair_GENERAL(digits, _mm_load_si128((const __m128i*)(tbl + key)), out);

        unsigned long consumed;
        _BitScanReverse(&consumed, first2);

        return (1u | (1 << 24) | ((end & commas) << 8u)) + (consumed << 24u);
	}

    const u32 effective_bits = first4 & _blsmsk_u32(commas); // is 26% 0x0088 and 38% 0x8888    

    const u32 key = ((effective_bits * 480925u) >> 10u) & 0x1fe0u;

    const auto* entry = (const u8*)slider_body_positive::TABLE.data() + key;

    const u64 entry_data = load_u64(entry);

    if (u32(entry_data) != effective_bits) [[unlikely]]
        return 0;

	parse_slider_pair_GENERAL(digits, _mm_load_si128((const __m128i*)(entry + 16)), out);

	return u32(entry_data >> 32) | ((effective_bits & commas) << 8);

}

#include "Parse_Double.h"

__forceinline const char *parse_slider_path(const char *__restrict p, _slider_point*__restrict slider_ptr, _slider_data *const __restrict r) {

	{ // hitsound

		const auto v = parse_integer_m2::likely_1(load_u32(p));

		p += (v >> 32);
		// p += 4;
	}

	const auto curve_type = (u8)*p;
	r->curve_type = curve_type;

	p += 2;

	r->point_end = (_slider_point*)size_t(p); // should be safe, we write over this again in all cases except the error

	r->point_start = slider_ptr;


	for (;;) {

		const auto result = parse_two_slider_points(p, slider_ptr);

		if (result == 0u) [[unlikely]] { // ditch all our work and come back later		
			push_error_slider_body_list(r);
			return nullptr;
		}

		slider_ptr += u8(result);
		p += (result >> 24);
		
		if (result & 0x00FFFF00) [[likely]] //~81%
			break;

	}

	r->point_end = slider_ptr;

	if (p[1] == ',') [[likely]] { // 99.92%

		r->slides = (p[0] & 0x0f);

		p += 2;

	} else [[unlikely]] {

		u32 slides = (p[0] & 0x0f) * 10 + (p[1] & 0x0f);

		u32 v = load_u32(p += 2);

		while (v && u8(v) != u8(',')) {
			slides = slides * 10 + (v & 0x0f);
			v >>= 8;
			++p;
		}

		r->slides = slides;

		if (v == 0) [[unlikely]] {
			push_error_slider_body_list(r);
			return nullptr;
		}

		++p;

	}

	//r->length = parse_double::from_ascii::parse_decimal_16(p);

	return p;
}