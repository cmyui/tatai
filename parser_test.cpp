#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#define main tatai_main
#include "Source.cpp"
#undef main

static void check(bool ok) {
	if (!ok) { std::fputs("parser test failed\n", stderr); std::abort(); }
}

int main() {
	// Every populated lookup entry must produce exactly the single decoder's result.
	auto headers = []<u32 Width>() {
		for (u32 mask = 0; mask < 65536; ++mask) {
			const auto& e = object_pair::TABLE<Width>[mask];
			if (!e.consumed) continue;
			for (u32 offset = 0; offset < 32; ++offset) {
				alignas(32) char buffer[96]; std::memset(buffer, '1', sizeof(buffer));
				char* p = buffer + offset;
				for (u32 bit = 0; bit < 16; ++bit) if (mask & (1u << bit)) p[bit] = ',';
				p[e.consumed - 1] = ',';
				alignas(32) _object_header pair[2]{}, single{};
				const char* next0 = p; const char* next1 = p;
				const auto result = object_pair::parse<Width>(next0, next1, pair);
				u32 consumed;
				if constexpr (Width == 5) consumed = parse_5_time::parse_object_5digit_single(p, &single);
				else consumed = parse_6_time::parse_object_6digit_single(p, &single);
				check(result && next0 - p == consumed && next1 - p == consumed);
				check(!std::memcmp(pair, &single, sizeof(single)) && !std::memcmp(pair + 1, &single, sizeof(single)));
				p[0] = '-';
				next0 = next1 = p;
				check(!object_pair::parse<Width>(next0, next1, pair));
				check(next0 == p && next1 == p);
			}
		}
	};
	headers.template operator()<5>(); headers.template operator()<6>();

	for (u32 n = 1; n <= 16; ++n) for (u32 dot = 0; dot <= n; ++dot) {
		std::string a, b;
		for (u32 i = 0; i < n; ++i) { a += char('0' + (i * 7 + 3) % 10); b += char('0' + (i * 3 + 9) % 10); }
		if (dot < n) { a[dot] = '.'; b[n - 1 - dot] = '.'; }
		a += ','; b += ','; a.resize(64); b.resize(64);
		double x, y;
		parse_double::from_ascii::parse_decimal_16_pair(a.data(), b.data(), x, y);
		const double sx = parse_double::from_ascii::parse_decimal_16(a.data());
		const double sy = parse_double::from_ascii::parse_decimal_16(b.data());
		check(!std::memcmp(&x, &sx, 8) && !std::memcmp(&y, &sy, 8));
	}

	auto* memory = create_memory_region();
	// Put the padded input immediately before a guard page, at every alignment.
	auto* pages = (char*)VirtualAlloc(nullptr, 12288, MEM_RESERVE, PAGE_NOACCESS);
	check(pages && VirtualAlloc(pages + 4096, 4096, MEM_COMMIT, PAGE_READWRITE));
	for (u32 version : {7u, 14u}) for (u32 offset = 0; offset < 32; ++offset) {
		std::string map = "osu file format v" + std::to_string(version) + "\n\n[General]\nAudioFilename: a\nMode: 3\n[Difficulty]\nCircleSize: 4\n[TimingPoints]\n0,500,4,2,1,100,1,0\n100,-50,4,2,1,100,0,0\n[HitObjects]\n";
		for (u32 i = 0; i < 67; ++i) {
			const u32 time = i < 33 ? 10000 + i : 100000 + i;
			map += (i % 3 ? "512,23," : "-12,999,") + std::to_string(time);
			map += i % 2 ? ",1,0\n" : ",2,0,B|10:20|30:40,1,0.35\n";
		}
		check(map.size() + 160 < 4096);
		for (bool at_end : {false, true}) {
			char* p = at_end ? pages + 8192 - 160 - offset - map.size() : pages + 4096 + offset;
			std::memcpy(p, map.data(), map.size()); std::memset(p + map.size(), 0, 129); p[map.size()] = '\n';
			parse_beatmap_from_memory(&memory->header, p, p + map.size());
			const auto& h = memory->header;
			check(h.osu_headers.Mode == 3 && h.ELEM_COUNT[MEM_object_header] == 67);
			check(h.ELEM_COUNT[MEM_timing_point] == 2);
			check(h.get_timing_point()[1].beat_length == 250 && h.get_timing_point()[1].tick_beat_length == (version < 8 ? 250 : 500));
			for (u32 i = 0; i < 67; ++i) {
				const auto& o = h.get_object_header()[i];
				check(o.x == (i % 3 ? 512 : 0) && o.y == (i % 3 ? 23 : 512));
				check(o.time == (i < 33 ? 10000 + i : 100000 + i) && o.type == (i % 2 ? 1 : 2));
				if (!(i % 2)) { const auto& s = h.get_object_body()[i];
					check(s.point_end - s.point_start == 2 && s.slides == 1 && s.curve_type == 'B');
					check(s.point_start[0].x == 10 && s.point_start[0].y == 20 && s.point_start[1].x == 30 && s.point_start[1].y == 40);
				}
			}
		}
	}
	// Rejected general fallback must not poison the next slider's output cursor.
	std::string recovery = "osu file format v14\n\n[General]\nAudioFilename: a\nMode: 0\n[Difficulty]\n[TimingPoints]\n0,500,4,2,1,100,1,0\n[HitObjects]\n"
		"1,2,10000,2,0,B|1234:x,1,100,0:0:0:0:\n"
		"1,2,10001,2,0,B|1234:56,1,100,0:0:0:0:\n";
	const auto recovery_size = recovery.size(); recovery.resize(recovery_size + 129);
	parse_beatmap_from_memory(&memory->header, recovery.data(), recovery.data() + recovery_size);
	check(memory->header.ELEM_COUNT[MEM_object_header] == 1);
	check(memory->header.get_object_header()[0].time == 10001);
	check(memory->header.get_object_body()[0].point_start[0].x == 1234);

	VirtualFree(pages, 0, MEM_RELEASE);
	std::puts("parser tests passed");
}
