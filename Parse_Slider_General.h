#pragma once

u32 general_parse_slider_points(const char* __restrict p,
	slider_point* __restrict slider_ptr, _slider_data* const __restrict r) {

	r->point_start = slider_ptr;

	const auto consume_digits_read = [](const char* p) {

		bool sign = *p == '-' ? (++p, true) : false;
		int ret{};

		auto digit = u32(*p) - '0';

		while (digit <= 9) {
			ret = (ret * 10) + digit;
			digit = u32(*++p) - '0';
		}

		return std::tuple<int, const char*>{sign ? -ret : ret, p};
	};

	for (;;) {

		const auto [x_value, x_end] {consume_digits_read(p)};

		if (*x_end != ':')
			return 0;

		const auto [y_value, y_end] {consume_digits_read(x_end+1)};

		slider_ptr->x = x_value;
		slider_ptr->y = y_value;

		++slider_ptr;

		if (*y_end != '|') {
			p = y_end;
			break;
		}

		p = y_end+1;

	}

	r->point_end = slider_ptr;

	if (*p != ',')
		return 0;
	{

		const auto [slides, slides_end] {consume_digits_read(p+1)};

		r->slides = slides;
		p = slides_end;

	}

	if (*p != ',')
		return 0;


	r->length = parse_double::from_ascii::parse_decimal_16(p+1);

	return 1;
}