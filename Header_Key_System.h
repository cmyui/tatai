#pragma once

enum header_id : u8 {
	LetterboxInBreaks = 0,
	Mode = 1,
	WidescreenStoryboard = 2,
	Title = 3,
	AudioFilename = 4,
	StackLeniency = 5,
	ArtistUnicode = 6,
	PreviewTime = 7,
	OverallDifficulty = 8,
	UseSkinSprites = 9,
	ApproachRate = 10,
	Version = 11,
	SampleSet = 12,
	OverlayPosition = 13,
	Artist = 14,
	Countdown = 15,
	Creator = 16,
	HPDrainRate = 17,
	BeatmapSetID = 18,
	CircleSize = 19,
	Source = 20,
	BeatmapID = 21,
	Tags = 22,
	SkinPreference = 23,
	SliderTickRate = 24,
	AudioLeadIn = 25,
	EpilepsyWarning = 26,
	TitleUnicode = 27,
	SamplesMatchPlaybackRate = 28,
	SpecialStyle = 29,
	SliderMultiplier = 31,
};

namespace header_key {

	struct _key {
		std::string_view name;
		header_id id;
	};

	#define KEY(x) _key{ #x, header_id::x }

	inline constexpr _key KEYS[] = {
		KEY(LetterboxInBreaks), KEY(Mode), KEY(WidescreenStoryboard), KEY(Title), KEY(AudioFilename),
		KEY(StackLeniency), KEY(ArtistUnicode), KEY(PreviewTime), KEY(OverallDifficulty), KEY(UseSkinSprites),
		KEY(ApproachRate), KEY(Version), KEY(SampleSet), KEY(OverlayPosition), KEY(Artist),
		KEY(Countdown), KEY(Creator), KEY(HPDrainRate), KEY(BeatmapSetID), KEY(CircleSize),
		KEY(Source), KEY(BeatmapID), KEY(Tags), KEY(SkinPreference), KEY(SliderTickRate),
		KEY(AudioLeadIn), KEY(EpilepsyWarning), KEY(TitleUnicode), KEY(SamplesMatchPlaybackRate), KEY(SpecialStyle),
		KEY(SliderMultiplier),
	};

	#undef KEY

	// a line is hashed on its bytes before the ':', cut to the first 8, so a short key never hashes its value
	constexpr u64 masked_key(std::string_view s) {

		u64 ret{};

		for (size_t i{}; i < s.size() && i < 8; ++i)
			ret |= u64(u8(s[i])) << (i * 8);

		return ret;
	}

	constexpr u32 key_slot(u64 key) {
		return u32((key * 0x3d1c550f1692402full) >> 58);
	}

	// no masked line key can start with ':', so an empty slot never matches
	constexpr u64 EMPTY_SLOT = u64(':');

	struct _key_table {
		u64 key[64];
		u8 length[64];
		u8 id[64];
	};

	alignas(64) inline constexpr auto KEY_TABLE = [] {

		_key_table t{};

		for (auto& k : t.key)
			k = EMPTY_SLOT;

		for (const auto& k : KEYS) {

			const auto slot = key_slot(masked_key(k.name));

			if (t.key[slot] != EMPTY_SLOT)
				throw "two header keys share a slot, pick a new multiplier";

			t.key[slot] = masked_key(k.name);
			t.length[slot] = u8(k.name.size());
			t.id[slot] = k.id;
		}

		return t;
	}();


	const char** parse_headers_key_index(_memory_region_header* __restrict MEM,
		const char** __restrict start, const char** const __restrict end) {

		MEM->version_number = 0;

		const char* first_line{ *start++ };

		// skip anything before the format line, e.g. a UTF-8 BOM
		for (const char* const limit{ first_line + 16 }; *first_line != 'o' && first_line != limit; ++first_line) {}

		if (load_u64(first_line) == str_to_u64("osu file format v")) [[likely]] {

			MEM->version_number = parse_integer_m3::expect_2(load_u32(first_line + sizeof("osu file format v") - 1));

		}
		//else return end;

		ZeroMemory(&MEM->osu_header_table, sizeof(MEM->osu_header_table));

		for (; start != end; ++start) {

			const char* line_start = *start;

			const u64 key{ load_u64(line_start) };
			
			if (key == str_to_u64("[Events]")) {
				++start;
				goto skip_events;
			}

			if (key == str_to_u64("[TimingPoints]")) {
				++start;
				goto do_timing;
			}

			// the bytes before a ':' in the first 8
			const u64 not_colon = key ^ 0x3a3a3a3a3a3a3a3aull;
			const u64 colon_bits = _tzcnt_u64((not_colon - 0x0101010101010101ull) & ~not_colon & 0x8080808080808080ull);

			const u64 masked = _bzhi_u64(key, u32(colon_bits) & ~7u);

			const u32 slot = key_slot(masked);

			const u32 key_length = KEY_TABLE.length[slot];

			if (KEY_TABLE.key[slot] != masked || line_start[key_length] != ':')
				continue;

			const char* line_end = (start + 1 == end) ? *start : *(start + 1);

			const char* value = line_start + key_length + 1;

			value += (*value == ' ');

			// keeps the trailing '\r'. underflows on the last line, but this piece of code should not be here at EOF anyway.
			MEM->osu_header_table[KEY_TABLE.id[slot]] = {
				value,
				size_t((line_end - 1) - value)
			};

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
		
			if (MEM->version_number < 8)
				start = parse_timing_points<1>(MEM, start, end);
			else
				start = parse_timing_points<0>(MEM, start, end);

		}

		return start;
	}
}