#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

template<typename T, int N>
inline void print_hex_and_ascii(std::string const &name, T (&data) [N])
{
	auto print_hex_byte = [](uint32_t const &b)
	{
		std::cout << std::setw(2) << std::setfill('0') << std::hex << b;
	};
	auto print_iff_ascii = [](char const &b)
	{
		if (isalnum(b))
			std::cout << b;
	};
	std::cout << name << ": ";
	std::for_each(std::begin(data), std::end(data), print_iff_ascii);
	std::cout << " (0x";
	std::for_each(std::begin(data), std::end(data), print_hex_byte);
	std::cout << ')' << std::endl;
}

template<typename T>
inline void hexdump_n(T const &x, size_t const n)
{
	size_t const bytes_per_row = 16;
	size_t total = n, addr_width = 1;
	while (total >>= 8)
		addr_width++;
	addr_width *= 2;

	for (size_t offset = 0; offset < n;)
	{
		std::stringstream ascii;

		std::cout << std::setfill('0') << std::hex <<
			std::setw(addr_width) << offset << ' ';

		for (int i = 0; i < bytes_per_row && offset < n; ++i, ++offset)
		{
			std::cout << std::setw(2) << (uint32_t const)x[offset] << ' ';
			ascii << (char)(isalnum(x[offset]) ? x[offset] : '.');
		}
		std::cout << ascii.str() << std::endl;
	}
}

template<typename T>
inline void memzero(T &x)
{
	std::fill(std::begin(x), std::end(x), 0);
}

template<typename T>
inline void memzero_n(T &x, size_t const n)
{
	std::fill(x, x + n, 0);
}

#if defined(_MSC_VER)

#include <Windows.h>

inline std::wstring string_to_wstring(std::string const &str)
{
	std::wstring result;
	wchar_t *str_wide = nullptr;
	auto size_requested = str.length() + 1;
	auto cstr = str.c_str();

	auto result_ok = [&size_requested](size_t const size)
	{
		return size != -1 && size != size_requested;
	};

	auto size_required = mbstowcs(nullptr, cstr, size_requested);

	if (result_ok(size_required))
	{
		str_wide = new wchar_t[size_required + 1];
		if (result_ok(mbstowcs(str_wide, cstr, size_requested)))
		{
			str_wide[size_required] = L'\0';
			result = str_wide;
		}
		delete [] str_wide;
	}

	return result;
}

#endif
