#pragma once

#include <string>
#include <iostream>

template<typename T, int N>
inline void print_hex_and_ascii(std::string const &name, T (&data) [N])
{
	auto print_hex_byte = [](uint32_t const &b)
	{
		std::cout.width(2);
		std::cout.fill('0');
		std::cout << std::hex << b;
	};
	auto print_iff_ascii = [](char const &b)
	{
		if (b >= ' ' && b <= '~')
			std::cout << b;
	};
	std::cout << name << ": ";
	std::for_each(std::begin(data), std::end(data), print_iff_ascii);
	std::cout << " (0x";
	std::for_each(std::begin(data), std::end(data), print_hex_byte);
	std::cout << ")" << std::endl;
};

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
