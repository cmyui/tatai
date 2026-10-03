#include <vector>
#include <tuple>
#include <array>
#include <fstream>
#include <type_traits>

#include <immintrin.h>
#include <intrin.h>

#include <chrono>

#if defined(__clang__)
	constexpr bool is_clang{true};
#else
	constexpr bool is_clang{ false };
#endif


struct _Timer {

	const char* name;
	const std::chrono::steady_clock::time_point sTime;

	_Timer() : sTime(std::chrono::steady_clock::now()), name{} {};
	_Timer(const char* name) : sTime(std::chrono::steady_clock::now()), name{ name } {};

	float get_time() {

		return double(double((unsigned long long)(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - sTime).count())) / 1000.0);
	}

	~_Timer() {

		const auto v = double(double((unsigned long long)(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - sTime).count())) / (1000.0));

		if (name)
			printf("%s> %f\xE6s\n", name, v);
		else printf("\n%.2f\xE6s\n", v);

	}
};


template <typename F> struct on_scope_exit {
private: F func;
public:
	on_scope_exit(const on_scope_exit&) = delete;
	on_scope_exit& operator=(const on_scope_exit&) = delete;
	on_scope_exit(F&& f) : func(std::forward<F>(f)) {}
	~on_scope_exit() { func(); }
};

#define PCAT0(x, y) PCAT1(x, y)
#define PCAT1(x, y) PCAT2(!, x ## y)
#define PCAT2(x, r) r
#define ON_SCOPE_EXIT(...) on_scope_exit PCAT0(__scope, __LINE__) {[&]{__VA_ARGS__}}

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int64_t i64;

constexpr u32 pext_constexpr(u32 v, u32 m) noexcept {

	u32 r{}, o{ 1 };

	while (m) {
		const u32 bit = m & (0 - m);
		if (v & bit) r |= o;
		m &= m - 1;
		o <<= 1;
	}

	return r;
}

void read_file2(const char* file_name, std::vector<u8>& o) {

	o.clear();

	std::ifstream file(file_name, std::ios::binary | std::ios::ate | std::ios::in);

	if (file.is_open() == 0) [[unlikely]]
		return;

	const size_t file_size{ (size_t)file.tellg() };

	o.resize(file_size);

	if (file_size) [[likely]] {
		file.seekg(0, std::ios::beg);
		file.read((char*)o.data(), file_size);
	}
	file.close();

	return;
}

std::vector<u8> read_file(const char* file_name) {

	std::ifstream file(file_name, std::ios::binary | std::ios::ate | std::ios::in);

	if (file.is_open() == 0) [[unlikely]]
		return {};

	const size_t file_size{ (size_t)file.tellg() };

	std::vector<u8> ret(file_size);

	if (file_size) [[likely]] {
		file.seekg(0, std::ios::beg);
		file.read((char*)ret.data(), file_size);
	}
	file.close();

	return ret;
}

struct _slider_point {
	int x;
	int y;
};

struct alignas(16) _object_header {
	u32 x, y;
	u32 time;
	u32 type; // hit sound data will be blitted here afterwards
};

struct _error_entry {
	const char* line;
	_object_header* object;
};

struct alignas(16) _object_header_error {
	_error_entry* error_out;
	u32 error_count;
	u32 ALLOC_error_out;
};

static_assert(sizeof(_object_header_error) == sizeof(_object_header));


struct _spinner_data {
	u32 end_time;
};

struct _slider_data {

	/*const*/ _slider_point* point_start, * point_end;
	double length;
	u32 slides;
	u32 curve_type;

};

struct _object_body {

	union {

		_slider_data slider;
		_spinner_data spinner;

	};

}; static_assert(sizeof(_object_body) == 32);


struct _slider_deferral {
	const char* p;
	_slider_data* out;
};


int total_count_opt{};

[[nodiscard]] __forceinline u64 load_u64(const void* p) noexcept {
	u64 v;
	std::memcpy(&v, p, sizeof(v));
	return v;
}

[[nodiscard]] __forceinline u32 load_u32(const void* p) noexcept {
	u32 v;
	std::memcpy(&v, p, sizeof(v));
	return v;
}

[[nodiscard]] __forceinline u16 load_u16(const void* p) noexcept {
	u16 v;
	std::memcpy(&v, p, sizeof(v));
	return v;
}
[[nodiscard]] __forceinline u8 load_u8(const void* p) noexcept {
	u8 v;
	std::memcpy(&v, p, sizeof(v));
	return v;
}

namespace parse_integer_m3 {

	__forceinline u32 fixed_likely_1(u32 x, u32 digits) {

		const u32 d0 = x & 0xf;

		if (digits == 1)
			return d0;

		const u32 d1 = (x >> 8) & 0xf;

		if (digits == 2)
			return d0 * 10 + d1;

		const u32 d2 = (x >> 16) & 0xf;
		return d0 * 100 + d1 * 10 + d2;
	}

	__forceinline u32 fixed_likely_3(u32 x, u32 digits) {

		x &= 0x0f0f0f0f;

		const u32 d0 = u8(x);
		const u32 d1 = u8(x >> 8);

		if (digits == 3) {
			const u32 d2 = u8(x >> 16);
			return d0 * 100 + d1 * 10 + d2;
		}

		if (digits == 2)
			return d0 * 10 + d1;

		return d0;
	}

	template<size_t SHIFT = 0>
	__forceinline std::tuple<u32, u32> likely_1(const u32 x) {

		const u32 d0 = u8(x) & 0x0fu;

		if (u8(x >> 8) == ',') [[likely]]
			return { 2 << SHIFT, d0 };

		const u32 masked = x & 0x000f0f0fu;

		//const u32 d1 = u8(masked >> 8);
		//const u32 d2 = u8(masked >> 16);

		if ((masked >> 16) > 9) [[likely]]
			return { 3 << SHIFT, d0 * 10u + u8(masked >> 8) };

		return {
			4 << SHIFT,
			d0 * 100u + u8(masked >> 8) * 10u + u8(masked >> 16)
		};
	}


	//__forceinline std::tuple<u32, u32> likely_3(u32 x) {
	//    constexpr u32 BOTH = 0x00101000;
	//
	//    const u32 digit_bits = x & BOTH;
	//
	//    x &= 0x0f0f0f0fu;
	//
	//    const u32 d0 = u8(x);
	//    const u32 d1 = u8(x >> 8);
	//
	//    if (digit_bits == BOTH) {
	//        const u32 d2 = u8(x >> 16);
	//
	//        return {
	//            4,
	//            d0 * 100 + d1 * 10 + d2
	//        };
	//    }
	//
	//    if (digit_bits & 0x00001000) {
	//        return {
	//            3,
	//            d0 * 10 + d1
	//        };
	//    }
	//
	//    return { 2, d0 };
	//}


}

namespace parse_integer_m3 {


	__forceinline u32 expect_2(u32 x) { // TODO min-max this

		const u32 d0 = u8(x) - u8('0');
		const u32 d1 = u8(x >> 8u) - u8('0');
		const u32 d2 = u8(x >> 16u) - u8('0');

		if (d1 > 9)
			return d0;

		if (d2 > 9)
			return d0 * 10 + d1;


		return d0 * 100 + d1 * 10 + d2;
	}


}


namespace parse_integer_m2 {


	__forceinline u64 likely_1(u64 x) {

		const u64 d0 = u8(x) & 0x0fu;

		if (u8(x >> 8u) == ',')
			return (2ull << 32u) | d0;

		const u64 d1 = u8(x >> 8u) & 0x0f;

		return (3ull << 32u) | (d0 * 10 + d1);
	}


}

size_t is_valid_slider_type(u32 v) {
	constexpr u32 table =
		(1u << ('B' & 31)) |
		(1u << ('C' & 31)) |
		(1u << ('L' & 31)) |
		(1u << ('P' & 31));
	return (table >> (u32(v) & 31)) & 1;
}

#include "Memory.h"

constexpr u32 MAX_NOTES{ (1 << 15)-1 };

struct _object_body_buffer {
	_object_body objects[MAX_NOTES];
};

struct _timing_point {
	double beat_length, tick_beat_length;
	int time;
};

constexpr u64 MEMORY_REGION_SIZE{ 512ull * 1024ull * 1024ull };
constexpr u64 MEMORY_CHUNK_SIZE = 512ull * 1024ull; // 512kb - 128 pages of 4k

enum memory_region_enum : u64{
	MEM_header = 0,

	MEM_lines,

	MEM_object_header,
	MEM_object_body,

	MEM_timing_point,

	MEM_slider_defer,
	MEM_slider_path,

	MEM_object_fallback,
	MEM_slider_fallback,


	MEM_REGION_COUNT
};

struct _memory_region_header {

	__forceinline const char** get_lines() const noexcept {
		return (const char**)((u8*)this + MEMORY_REGION_SIZE * MEM_lines);
	}
	__forceinline _object_header* get_object_header() const noexcept {
		return (_object_header*)((u8*)this + MEMORY_REGION_SIZE * MEM_object_header);
	}
	__forceinline _slider_data* get_object_body() const noexcept {
		return (_slider_data*)((u8*)this + MEMORY_REGION_SIZE * MEM_object_body);
	}
	__forceinline _timing_point* get_timing_point() const noexcept {
		return (_timing_point*)((u8*)this + MEMORY_REGION_SIZE * MEM_timing_point);
	}
	__forceinline _slider_deferral* get_slider_defer() const noexcept {
		return (_slider_deferral*)((u8*)this + MEMORY_REGION_SIZE * MEM_slider_defer);
	}
	__forceinline _slider_point* get_slider_path() const noexcept {
		return (_slider_point*)((u8*)this + MEMORY_REGION_SIZE * MEM_slider_path);
	}
	__forceinline _error_entry* get_slider_fallback() const noexcept {
		return (_error_entry*)((u8*)this + MEMORY_REGION_SIZE * MEM_slider_fallback);
	}


	u32 ALLOC_COUNTS[MEM_REGION_COUNT];

	u32 ELEM_COUNT[MEM_REGION_COUNT];// this isnt always kept up to date, at least for now

	u32 version_number;

	std::string_view osu_header_table[32];

	//struct _osu_header {
	//
	//
	//	float HPDrainRate;
	//	float CircleSize;
	//	float OverallDifficulty;
	//	float ApproachRate;
	//
	//	double StackLeniency;
	//	double SliderMultiplier;
	//	double SliderTickRate;
	//
	//	std::string_view Mode;
	//	std::string_view AudioFilename;
	//	std::string_view AudioLeadIn;
	//	std::string_view PreviewTime;
	//	std::string_view Countdown;
	//	std::string_view SampleSet;
	//	std::string_view LetterboxInBreaks;
	//	std::string_view UseSkinSprites;
	//	std::string_view OverlayPosition;
	//	std::string_view SkinPreference;
	//	std::string_view EpilepsyWarning;
	//	//std::string_view CountdownOffset;
	//	std::string_view SpecialStyle;
	//	std::string_view WidescreenStoryboard;
	//	std::string_view SamplesMatchPlaybackRate;
	//
	//	std::string_view Title;
	//	std::string_view TitleUnicode;
	//	std::string_view Artist;
	//	std::string_view ArtistUnicode;
	//	std::string_view Creator;
	//	std::string_view Version;
	//	std::string_view Source;
	//	std::string_view Tags;
	//	std::string_view BeatmapID;
	//	std::string_view BeatmapSetID;
	//
	//} osu_headers;

	void print_map_data() {

		_object_header* o{ get_object_header() };
		_slider_data* s{ get_object_body() };

		for (size_t i{}, size{ELEM_COUNT[MEM_object_header]}; i < size; ++i) {

			printf("%i> %i,%i | %i  ", o[i].time, o[i].x, o[i].y, o[i].type);

			if (o[i].type & 2) {

				printf("\n  %i %f  ", s[i].slides, s[i].length);
				const auto* p{ s[i].point_start };
				for (; p != s[i].point_end; ++p) {
					printf("%i:%i|", p->x, p->y);
				}

			}

			printf("\n");

		}

		printf("NOTE_COUNT: %i\n", ELEM_COUNT[MEM_object_header]);

	}

};

struct _memory_region_new {

	_memory_region_header header;

	u8 padding0[4096 - sizeof(_memory_region_header)];

}; static_assert(sizeof(_memory_region_new) == 4096);

_memory_region_new* create_memory_region(bool dont_pre_alloc = 0) {

	// reserve 8GB of memory - each region can grow to a max of 512mb
	auto* base_addr = byte_allocator::large_reserve();

	auto* MR = (_memory_region_new*)byte_allocator::commit_memory(base_addr, sizeof(_memory_region_new));

	ZeroMemory(MR, sizeof(_memory_region_new));

	MR->header.ALLOC_COUNTS[0] = sizeof(_memory_region_new);

	if (dont_pre_alloc)
		return MR;

	for (u64 i{ (u64)MEM_lines }; i <= (u64)MEM_slider_path; ++i) {

		byte_allocator::resize(MEMORY_CHUNK_SIZE, (u8*)base_addr + MEMORY_REGION_SIZE * i, MR->header.ALLOC_COUNTS[i]);

	}

	return MR;
}

__declspec(noinline) void push_error_object_header_list(const char* __restrict p, _object_header* object) {

	object->type = 0;

	auto* MEM = (_memory_region_new*)(size_t(object) & POINTER_RESET_MASK);

	const auto new_count = ++MEM->header.ELEM_COUNT[MEM_object_fallback];

	auto* err_out = (_error_entry*)((u8*)MEM + (MEMORY_REGION_SIZE * MEM_object_fallback));

	err_out = (_error_entry*)byte_allocator::resize( new_count * sizeof(_error_entry),
		err_out, MEM->header.ALLOC_COUNTS[MEM_object_fallback]);

	err_out[new_count - 1] = _error_entry{
		.line = p,
		.object = object
	};

}

__declspec(noinline) void push_error_slider_body_list(_slider_data* const object) {

	auto* const p = (const char* const)object->point_end;

	object->point_end = object->point_start;

	auto* MEM = (_memory_region_new*)(size_t(object) & POINTER_RESET_MASK);

	const auto new_count = ++MEM->header.ELEM_COUNT[MEM_slider_fallback];

	auto* err_out = (_error_entry*)((u8*)MEM + (MEMORY_REGION_SIZE * MEM_slider_fallback));
	
	err_out = (_error_entry*)byte_allocator::resize(new_count * sizeof(_error_entry),
		err_out, MEM->header.ALLOC_COUNTS[MEM_slider_fallback]);

	err_out[new_count - 1] = _error_entry{
		.line = p,
		.object = (_object_header*)object
	};

}


#include "Parse_7_time.h"
#include "Parse_6_time.h"
#include "Parse_5_time.h"
#include "Parse_4_time.h"

#include "Parse_Slider.h"
#include "Parse_Spinner.h"

#include "Parse_Slider_General.h"

#include <thread>
#include <chrono>

#include <cstdlib>

template <auto parse_func>
__declspec(noinline) u64 parse_object_loop(
	const char* const* __restrict pos,
	_object_header* __restrict object,
	_slider_data* __restrict object_data,
	_slider_deferral* __restrict defer
) {

	const auto* start = pos;
	const auto* start_defer = defer;

	for (;;) {

		const char* p = *pos;

		if (p == nullptr)
			break;

		const auto con = parse_func(p, object);

		if (con == 0)
			break;

		*defer = { p + con, object_data };
		defer += (object->type >> 1) & 1;

		++pos;
		++object;
		++object_data;
	}

	return (pos - start) | (u64(defer - start_defer) << 32);
}

template <auto parse_func>
__declspec(noinline) u64 PAIR_parse_object_loop(
	const char* const* __restrict pos,
	_object_header* __restrict object,
	_slider_data* __restrict object_data,
	_slider_deferral* __restrict defer
) {

	const auto* start = pos;
	const auto* start_defer = defer;


	for (;;) {

		const char* p = *pos;

		if (p == nullptr)
			break;

		const char* p1 = *(pos + 1);

		if (p1 == nullptr) {
			break;
		}

		const auto t = parse_func(p, p1, object);
		//const auto t = parse_7_time::parse_object_7digit_SIMD_pair(p, p1, object);

		auto con0 = u8(t);

		if (con0 == 0) [[unlikely]]
			break;

		*defer = { p + con0, object_data };
		defer += (object->type >> 1) & 1;

		auto con1 = t >> 8;

		if (con1 == 0) [[unlikely]] {
			pos += 1;
			object += 1;
			object_data += 1;
			break;
		}

		*defer = { p1 + con1, object_data + 1 };
		defer += ((object + 1)->type >> 1) & 1;

		pos += 2;
		object += 2;
		object_data += 2;

	}

	return (pos - start) | (u64(defer - start_defer) << 32);
}

const char* find_hitobjects_line(const char* const start, const char* const end) {

	const auto open = _mm256_set1_epi8('[');
	const auto h = _mm256_set1_epi8('H');

	for (const char* p = start; p < end; p += 32) {

		auto mask = (u32)_mm256_movemask_epi8(_mm256_and_si256(
			_mm256_cmpeq_epi8(_mm256_loadu_si256((__m256i const*)p), open),
			_mm256_cmpeq_epi8(_mm256_loadu_si256((__m256i const*)(p + 1)), h)));

		for (; mask; mask = _blsr_u32(mask)) {

			const char* c = p + _tzcnt_u32(mask);

			if (load_u64(c) == 0x656A624F7469485Bull && (c == start || c[-1] == '\n') && c < end)
				return c;
		}
	}

	return end;
}

#include "Parse_File_Header.h"

void parse_beatmap_from_memory(_memory_region_header* __restrict MEM, char const* __restrict p, char const* __restrict end) {

	if (MEM == 0)
		return;

	ZeroMemory(MEM->ELEM_COUNT, sizeof(MEM->ELEM_COUNT));

	{

		const auto file_size{ (end - p) };

		//64mb is the max size accepted - fail on any file above that size
		if (byte_allocator::resize(sizeof(char*) * (file_size + 4),
			MEM->get_lines(), MEM->ALLOC_COUNTS[MEM_lines]) == nullptr)
			return;

		const u64 max_slider_points = file_size >> 2;

		byte_allocator::resize( sizeof(_slider_point) * max_slider_points,
			MEM->get_slider_path(), MEM->ALLOC_COUNTS[MEM_slider_path]);

		byte_allocator::resize( sizeof(_timing_point) * max_slider_points, 
			MEM->get_timing_point(), MEM->ALLOC_COUNTS[MEM_timing_point]);

		//X,X,X,XN
		const u32 max_notes = file_size >> 3;

		byte_allocator::resize(sizeof(_object_header) * max_notes,
			MEM->get_object_header(), MEM->ALLOC_COUNTS[MEM_object_header]);

		byte_allocator::resize(sizeof(_object_body) * max_notes,
			MEM->get_object_body(), MEM->ALLOC_COUNTS[MEM_object_body]);

		//TODO figure out minimum slider length i should accept

		byte_allocator::resize(sizeof(_slider_deferral) * max_notes,
			MEM->get_slider_defer(), MEM->ALLOC_COUNTS[MEM_slider_defer]);

	}

	const auto nl = _mm256_set1_epi8('\n');

	_object_header* object_ptr{ MEM->get_object_header() };
	_slider_data* object_data_ptr{ MEM->get_object_body() };

	_slider_point* slider_ptr{ MEM->get_slider_path() };

	const char** line_ptr{ MEM->get_lines() };
	const char** line_ptr_end{ line_ptr };

	auto* slider_defer_table{ MEM->get_slider_defer() };

	{ // parse all the new lines for now, should skip [Events] unless we specifically want them in the future though.

		*line_ptr++ = p;

		#define DO { const auto bit = _tzcnt_u64(mask); *(line_ptr++) = p + bit; mask = _blsr_u64(mask); }


		for (; p + 64 <= end; p += 63) {

			const auto v0 = _mm256_loadu_si256((__m256i const*)(p + 0x00));
			const auto v1 = _mm256_loadu_si256((__m256i const*)(p + 0x20));

			++p;

			const auto cmp0 = _mm256_cmpeq_epi8(v0, nl);
			const auto cmp1 = _mm256_cmpeq_epi8(v1, nl);

			const auto m0 = (u32)_mm256_movemask_epi8(cmp0);
			const auto m1 = (u32)_mm256_movemask_epi8(cmp1);

			auto mask = u64(m0) | (u64(m1) << 32);


			const auto count = (u32)_mm_popcnt_u64(mask);

			line_ptr[0] = p + _tzcnt_u64(mask); mask = _blsr_u64(mask);
			line_ptr[1] = p + _tzcnt_u64(mask); mask = _blsr_u64(mask);
			line_ptr[2] = p + _tzcnt_u64(mask); mask = _blsr_u64(mask);
			line_ptr[3] = p + _tzcnt_u64(mask); mask = _blsr_u64(mask);

			if (count > 4) [[unlikely]] {
				line_ptr += 4;
				while (mask) DO
			} else {
				line_ptr += count;
			}

		}

		if (p < end) {

			const auto v0 = _mm256_loadu_si256((__m256i const*)(p + 0x00));
			const auto v1 = _mm256_loadu_si256((__m256i const*)(p + 0x20));

			const auto m0 = (u32)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v0, nl));
			const auto m1 = (u32)_mm256_movemask_epi8(_mm256_cmpeq_epi8(v1, nl));

			auto mask = _bzhi_u64(u64(m0) | (u64(m1) << 32), u32(end - p));

			++p;

			while (mask) DO

		}

		#undef DO		

		line_ptr_end = line_ptr;

		*line_ptr++ = nullptr;
		*line_ptr = nullptr;

		line_ptr = MEM->get_lines();

		_mm256_zeroupper();

	}

	line_ptr = parse_beatmap_header(MEM, line_ptr, line_ptr_end);

	{

		const auto P_SAVE = p;

		//_Timer A{};
		//for (size_t CRANK{}; CRANK < 100000; ++CRANK) 
		{

			for (; line_ptr != line_ptr_end; ++line_ptr) {

				if (*(const u64*)(*line_ptr) == 0x656A624F7469485Bull) {
					++line_ptr;
					break;
				}

			}

			//for (size_t CRANK{}; CRANK < 100000; ++CRANK)
			{

				object_ptr = MEM->get_object_header();
				object_data_ptr = MEM->get_object_body();
				slider_ptr = MEM->get_slider_path();
				slider_defer_table = MEM->get_slider_defer();


				for (;;) {

					if (line_ptr == line_ptr_end)
						break;

					const u8* p{ (u8 const*)*line_ptr };

					//if (p == nullptr) break;

					const auto m0 = _mm_loadu_si128((__m128i*)p);
					auto v = (u32)_mm_movemask_epi8(_mm_cmpeq_epi8(m0, _mm_set1_epi8(',')));

					const auto c0 = (u32)_tzcnt_u32(v);
					v = _blsr_u32(v);

					const auto c1 = (u32)_tzcnt_u32(v);
					v = _blsr_u32(v);

					const auto c2 = (u32)_tzcnt_u32(v);

					switch (c2 - c1/*time_digits*/) {

					case 5: goto parse4;
					case 6: goto parse5;
					case 7: goto parse6;
					case 8: goto parse7;

					case 1:
					case 2:
					case 3:
					case 4:
						break;
					default: goto parse_finished;
					}

					ON_SCOPE_EXIT(
						++object_ptr;
						++object_data_ptr;
						++line_ptr;
					);

					v = _blsr_u32(v);

					const auto c3 = (u32)_tzcnt_u32(v);

					object_ptr->x = parse_integer_m3::fixed_likely_3(load_u32(p), (c0));
					object_ptr->y = parse_integer_m3::fixed_likely_3(load_u32(p + c0 + 1), (c1 - c0) - 1);
					object_ptr->time = parse_integer_m3::fixed_likely_3(load_u32(p + c1 + 1), (c2 - c1) - 1);
					object_ptr->type = parse_integer_m3::fixed_likely_1(load_u32(p + c2 + 1), (c3 - c2) - 1);

					*slider_defer_table = { (const char*)p + c3 + 1, object_data_ptr };
					slider_defer_table += (object_ptr->type >> 1) & 1;

				}

				{
				parse4: //if (*line_ptr == nullptr) goto parse_finished;

					{

						const auto res = parse_object_loop<parse_4_time::parse_object_4digit_single>(line_ptr, object_ptr, object_data_ptr, slider_defer_table);

						slider_defer_table += u32(res >> 32);

						const auto count = u32(res);

						line_ptr += count;
						object_ptr += count;
						object_data_ptr += count;

					}

				parse5: //if (*line_ptr == nullptr) goto parse_finished;

					{

						//const auto res = PAIR_parse_object_loop<parse_5_time::parse_object_5digit_pair>(
						const auto res = parse_object_loop<parse_5_time::parse_object_5digit_single>(
							line_ptr, object_ptr, object_data_ptr, slider_defer_table);

						slider_defer_table += u32(res >> 32);

						const auto count = u32(res);

						line_ptr += count;
						object_ptr += count;
						object_data_ptr += count;

					}

				parse6: if (line_ptr == line_ptr_end) goto parse_finished;
				{
					// only pair that wins for now
					//const auto res = PAIR_parse_object_loop<parse_6_time::parse_object_6digit_pair>(
					const auto res = parse_object_loop<parse_6_time::parse_object_6digit_single>(
						line_ptr, object_ptr, object_data_ptr, slider_defer_table);

					slider_defer_table += u32(res >> 32);

					const auto count = u32(res);

					line_ptr += count;
					object_ptr += count;
					object_data_ptr += count;

				}

				parse7: if (line_ptr == line_ptr_end) goto parse_finished;
				{
					//const auto res = PAIR_parse_object_loop<parse_7_time::parse_object_7digit_SIMD_pair>(
					const auto res = parse_object_loop<parse_7_time::parse_object_7digit_single>(
						line_ptr, object_ptr, object_data_ptr, slider_defer_table);

					slider_defer_table += u32(res >> 32);

					const auto count = u32(res);

					line_ptr += count;
					object_ptr += count;
					object_data_ptr += count;

				}

			}

			parse_finished:

				// error table

				{ // calc slider paths
					for (auto* d = MEM->get_slider_defer(); d != slider_defer_table; ++d) {

						d->p = parse_slider_path(d->p, slider_ptr, d->out);

						slider_ptr = d->out->point_end;

					}
				}

				{ // calc slider length

					//double last{ 0. }; u64 check{};

					for (const auto* d = MEM->get_slider_defer(); d != slider_defer_table; ++d) {

						if (d->p == nullptr) [[unlikely]]
							continue;

						d->out->length = parse_double::from_ascii::parse_decimal_16(d->p);
						 
						//const auto v = load_u64(d->p);
						//
						//if (v == check) {
						//	d->out->length = last;
						//	continue;
						//}
						//check = v;
						//
						//last = parse_double::from_ascii::parse_decimal_16(d->p);
						//d->out->length = last;
						//printf("%f\n", d->out->length);
					}
				}

				{ // fall back general cases

					const auto* error_header = MEM->get_slider_fallback();
					const auto*const error_header_end = error_header + MEM->ELEM_COUNT[MEM_slider_fallback];

					//if (error_header->error_count) printf("CORRECTIONS:%i\n", error_header->error_count);

					for (; error_header != error_header_end; ++error_header) {

						const auto ret = general_parse_slider_points(error_header->line, slider_ptr, (_slider_data*)error_header->object);

						if (ret == 0)[[unlikely]] // fully corrupted slider data, abort map?
							return;

						slider_ptr = ((_slider_data*)error_header->object)->point_end;


					}

				}

			}

		}

	}

	MEM->ELEM_COUNT[MEM_object_header] = object_ptr - MEM->get_object_header();

	return;
}

#include <filesystem>
#include <iostream>

void run_test_folder() {

	//return;
	
	_memory_region_new* MR{ create_memory_region() };

	u32 XOR_TOTAL{};
	u32 COUNT{};
	{
		//_Timer A{};
		//std::vector<u8> FILE_BUFFER{}; FILE_BUFFER.reserve(u16(-1));
		std::chrono::steady_clock::duration total_elapsed_time = std::chrono::steady_clock::duration::zero();

		std::vector<u8> FILE_BUFFER{};

		//_Timer A{};
		for (const auto& file_entry : std::filesystem::directory_iterator("../fast_beatmap_load/map/maps")) {

			const auto _p{ file_entry.path().native() };

			const auto file_name{ std::string(_p.begin(), _p.end()) };

			if (file_name.find(".osu") == std::string::npos)
				continue;

			//printf("%s\n", file_name.c_str());

			read_file2(file_name.c_str(), FILE_BUFFER);

			FILE_BUFFER.push_back('\n');
			FILE_BUFFER.resize(FILE_BUFFER.size() + 128);


			++COUNT;

			if ((COUNT & ((1<<10)-1)) == 0) printf("%i\n", COUNT);

			//if (COUNT > 2000) break;

			u32 XOR = 0;

			std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

			//for (size_t CRANK{}; CRANK < 1000; ++CRANK)
			parse_beatmap_from_memory(&MR->header, (char*)FILE_BUFFER.data(), (char*)FILE_BUFFER.data() + FILE_BUFFER.size() - 128);

			//MR.print_map_data();

			total_elapsed_time += std::chrono::steady_clock::now() - start_time;

			//const auto duration = (u64)(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - //start_time)).count();
			////
			//double nano_seconds{ (double(duration) / 1000.) };
			//double micro_seconds{ nano_seconds / 1000. };

			//printf("%s> %.2f\xE6s (%.2fns)\n", file_name.substr(file_name.find_last_of('/') + 1).c_str(), micro_seconds, nano_seconds / double(MR.note_count ? MR.note_count : 1));

			XOR_TOTAL ^= (size_t)MR->header.get_object_body()[593].point_start;
			XOR_TOTAL ^= (size_t)MR->header.get_object_header()[4882].time;
			//XOR_TOTAL += XOR ^ MR.object_body[52].spinner.end_time;
			XOR_TOTAL += MR->header.ELEM_COUNT[MEM_object_header];
			//XOR_TOTAL += MR.headers.StackLeniency;
		}

		const auto duration = (u64)(std::chrono::duration_cast<std::chrono::nanoseconds>(total_elapsed_time).count());
		
		double nano_seconds{ double(duration) };
		double micro_seconds{ nano_seconds / 1000. };
		printf("TOTAL_TIME: %f| average_per_map:%f\n", micro_seconds, micro_seconds / double(COUNT));

	}
	printf("%i\n", XOR_TOTAL);

}

//#include "state_test.h"

int main() {

	//run_test_folder();
	//return 0;

	auto data = read_file("within_objects.txt");

	data.push_back('\n');
	data.resize(data.size() + 128);

	_memory_region_new* MR{ create_memory_region(0) };

	for (size_t warm_up{}; warm_up < 1000; ++warm_up)
		parse_beatmap_from_memory(&MR->header, (char*)data.data(), (char*)data.data() + data.size() - 128);

	u64 XOR{};
	{
		_Timer A{};
		parse_beatmap_from_memory(&MR->header, (char*)data.data(), (char*)data.data() + data.size() - 128);
	}

	std::cin.get();
	MR->header.print_map_data();

	return 0;

}