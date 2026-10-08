#pragma once

enum header_id : u8 {
	CircleSize = 0,
	StackLeniency = 1,
	ApproachRate = 2,
	SliderTickRate = 4,
	SliderMultiplier = 6,
	OverallDifficulty = 7,
};

namespace header_key {

	__forceinline const char** parse_headers_key_index(_memory_region_header* __restrict MEM,
		const char** __restrict start, const char** const __restrict end) {

		{

			const auto* s = *start;
			auto o_check = load_u64(s);

			while (o_check && u8(o_check) != 'o') {
				o_check >>= 8;
				++s;
			}

			if (load_u64(s) == str_to_u64("osu file format v")) [[likely]]
				MEM->version_number = parse_integer_m3::expect_2(load_u32(s + sizeof("osu file format v") - 1));
			else
				MEM->version_number = 14; // what osu does but.. this is very bad behaviour

			start += 4;

			if (size_t(start) > size_t(end))
				start = end;

		}

		MEM->osu_headers.Mode = 0;

		MEM->osu_headers.table[header_id::CircleSize] = 5.;
		MEM->osu_headers.table[header_id::OverallDifficulty] = 5.;
		MEM->osu_headers.table[header_id::ApproachRate] = -1.;

		MEM->osu_headers.table[header_id::StackLeniency] = 0.7;

		MEM->osu_headers.table[header_id::SliderTickRate] = 1.;
		MEM->osu_headers.table[header_id::SliderMultiplier] = 1.4;

		for (; start != end; ++start) {

			const u8* line_start = (u8*)*start;

			const u64 key{ load_u64(line_start) };

			if (u8(key) == u8('['))
				break;			

			if (key == str_to_u64("StackLeniency:")){
			
				line_start += 14;

				line_start += u8(*line_start - u8('0')) > 9;

				MEM->osu_headers.table[header_id::StackLeniency] = parse_double::from_ascii::parse_decimal_16((const char*)line_start);
				continue;
			}

			if (u32(key >> 8) == str_to_u32("ode:")) {

				line_start += 5;

				u8 digit = *line_start - u8('0');

				if (digit > 9)
					digit = *++line_start - u8('0');

				MEM->osu_headers.Mode = digit;

				continue;
			}			

		}

		for (; start != end; ++start) {

			if (load_u64(*start) != str_to_u64("[Difficulty]"))
				continue;

			++start;
			break;
		}


		{

			for (; start != end; ++start) {

				const u8* line_start = (u8*)*start;

				const u64 key{ load_u64(line_start) };

				if (key == str_to_u64("[Events]")) {
					++start;
					goto skip_events;
				}

				if (key == str_to_u64("[TimingPoints]")) {
					++start;
					goto do_timing;
				}			

				alignas(64) constexpr static u64 verification_table[8] = {
					str_to_u64("CircleSize:"),
					~0ull,//StackLeniency
					str_to_u64("ApproachRate:"),
					~0ull,
					str_to_u64("SliderTickRate:"),
					~0ull,
					str_to_u64("SliderMultiplier:"),
					str_to_u64("OverallDifficulty:")
				};

				const u32 index = u32((key * 157206ull) >> 61);

				__assume(index < 8);

				if (key != verification_table[index])
					continue;

				line_start += index + 11;
				line_start += u8(*line_start - u8('0')) > 9;

				//for older beatmap versions, would be faster to parse ints for the stuff we can.
				MEM->osu_headers.table[index] = parse_double::from_ascii::parse_decimal_16((const char*)line_start);

			}

		}

		skip_events: {
			for (; start != end; ++start) { // TODO: unroll?

				if (load_u64(*start) != str_to_u64("[TimingPoints]"))
					continue;

				++start;
				break;
			}
		}

		do_timing: {
			
			if (MEM->osu_headers.table[header_id::ApproachRate] == -1.)
				MEM->osu_headers.table[header_id::ApproachRate] =
					MEM->osu_headers.table[header_id::OverallDifficulty];

			if (MEM->version_number < 8)
				start = parse_timing_points<1>(MEM, start, end);
			else
				start = parse_timing_points<0>(MEM, start, end);

		}

		return start;
	}
}