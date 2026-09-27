#pragma once

namespace state_test {    


	const char* state_jump_to_hitobjects(char const* __restrict p, char const* __restrict const end) {

		for (; p + 64 <= end; p += 64) {

			const auto v0 = _mm256_loadu_si256((__m256i const*)(p + 0x00));
			const auto v1 = _mm256_loadu_si256((__m256i const*)(p + 0x20));

			const auto m0 = (u32)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v0, _mm256_set1_epi8('[')));
			const auto m1 = (u32)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v1, _mm256_set1_epi8('[')));

			auto mask = u64(m0) | (u64(m1) << 32);

			while (mask) {

				const auto bit = _tzcnt_u64(mask);

				ON_SCOPE_EXIT(
					mask = _blsr_u64(mask);
				);

				if (*(const u64*)((p + bit)) == 0x656A624F7469485Bull) {

					p += 14 + bit;

					if (p > end) [[unlikely]]
						return end;

					if (*(p - 1) != '\n') [[unlikely]] // no return carriage fixm just incase
						--p;

					return p;
				}


			}

		}

		return end;
	}
	
	template<auto parse_func>
	__declspec(noinline)    
	const char* line_reader_parse(_memory_region* __restrict const MEM, char const* __restrict p, char const* __restrict const end) {

		return end;
	}

	const char* state_test(_memory_region* __restrict const MEM, char const* __restrict p, char const* __restrict const end) {

		MEM->cur_object = (_object_header*)MEM->object_header_data;
		MEM->cur_slider = (_slider_data*)MEM->object_body_data;
		MEM->cur_slider_path = (slider_point*)MEM->SLIDER_PATHS;

		p = state_jump_to_hitobjects(p, end);

		--p;// temp for now        

		p = line_reader_parse<parse_4_time::parse_object_4digit_single>(MEM, p, end);
		p = line_reader_parse<parse_5_time::parse_object_5digit_single>(MEM, p, end);
		p = line_reader_parse<parse_6_time::parse_object_6digit_single>(MEM, p, end);
		p = line_reader_parse<parse_7_time::parse_object_7digit_single>(MEM, p, end);


		MEM->note_count = MEM->cur_object - (_object_header*)MEM->object_header_data;

		return nullptr;
	}

}