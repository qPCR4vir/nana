/*
 *	The Deploy Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/depoly.cpp
 *
 *	What follows is dependent on what defined in nana/config.hpp
 */

#include <nana/deploy.hpp>
#include <cstdlib>
#include <cstring> //std::strlen
#include <stdexcept>

#if defined(NANA_WINDOWS)
	#include <windows.h>
#elif defined(NANA_POSIX)
	#include <string.h>
	#include "detail/platform_spec_selector.hpp"
#endif

#include <iostream>

namespace nana
{
	bool is_utf8(std::string_view str)
	{
		auto ustr = reinterpret_cast<const unsigned char*>(str.data());
		auto end = ustr + str.size();

		while (ustr < end)
		{
			const auto uv = *ustr;
			if (uv < 0x80)
			{
				++ustr;
				continue;
			}

			if (uv < 0xC0)
				return false;

			if ((uv < 0xE0) && (end - ustr > 1))
				ustr += 2;
			else if ((uv < 0xF0) && (end - ustr > 2))
				ustr += 3;
			else if ((uv < 0x1F) && (end - ustr > 3))
				ustr += 4;
			else
				return false;
		}
		return true;
	}

	void throw_not_utf8(std::string_view str)
	{
		if (!is_utf8(str))
			return utf8_Error(std::string("\nThe text is not encoded in UTF8: ") + std::string(str.data(), str.size())).emit();
	}

	//class utf8_Error
	void utf8_Error::emit()
	{
		if (use_throw)
			throw utf8_Error(*this);
		std::cerr << what();
	}

	//bool utf8_Error::use_throw{true};
	bool utf8_Error::use_throw{ false };
	//end class utf8_Error

	std::string recode_to_utf8(std::string no_utf8)
	{
		return nana::charset(std::move(no_utf8)).to_bytes(nana::unicode::utf8);
	}

	/// this text needed change, it needed review ??
	bool review_utf8(const std::string& text)
	{
		if (!is_utf8(text))
		{
			utf8_Error(std::string("\nThe const text is not encoded in UTF8: ") + text).emit();
			return true;   /// it needed change, it needed review !!
		}
		else
			return false;
	}

	/// this text needed change, it needed review ??
	bool review_utf8(std::string& text)
	{
		if(!is_utf8(text))
		{
			utf8_Error(std::string("\nThe text is not encoded in UTF8: ") + text).emit();
			text=recode_to_utf8(text);
			return true;   /// it needed change, it needed review !!
		}
		else
			return false;
	}

	const std::string& to_utf8(const std::string& str)
	{
		return str;
	}

	std::string to_utf8(std::wstring_view text)
	{
		return ::nana::charset(std::wstring{text}).to_bytes(::nana::unicode::utf8);
	}

	std::wstring to_wstring(std::string_view utf8_str)
	{
		if (utf8_str.empty())
			return{};

		return ::nana::charset(std::string{ utf8_str.data(), utf8_str.size() }, unicode::utf8);
	}


	const std::wstring& to_wstring(const std::wstring& wstr)
	{
		return wstr;
	}

	std::wstring&& to_wstring(std::wstring&& wstr)
	{
		return static_cast<std::wstring&&>(wstr);
	}

#if defined(NANA_WINDOWS)
	std::string to_osmbstr(const std::string& text_utf8)
	{
		return ::nana::charset(text_utf8, ::nana::unicode::utf8);
	}
#else
	std::string to_osmbstr(std::string text_utf8)
	{
		return text_utf8;
	}
#endif

#if defined(NANA_WINDOWS)
	const detail::native_string_type to_nstring(const std::string& text)
	{
		return ::nana::charset(text, ::nana::unicode::utf8);
	}

	const detail::native_string_type& to_nstring(const std::wstring& text)
	{
		return text;
	}

	detail::native_string_type to_nstring(std::string&& text)
	{
		return ::nana::charset(text, ::nana::unicode::utf8);
	}

	detail::native_string_type&& to_nstring(std::wstring&& text)
	{
		return std::move(text);
	}

	detail::native_string_type to_nstring(int n)
	{
		return std::to_wstring(n);
	}

	detail::native_string_type to_nstring(double d)
	{
		return std::to_wstring(d);
	}

	detail::native_string_type to_nstring(std::size_t d)
	{
		return std::to_wstring(d);
	}
#else	//POSIX
	const detail::native_string_type& to_nstring(const std::string& text)
	{
		return text;
	}

	const detail::native_string_type to_nstring(const std::wstring& text)
	{
		return ::nana::charset(text).to_bytes(::nana::unicode::utf8);
	}

	detail::native_string_type&& to_nstring(std::string&& text)
	{
		return std::move(text);
	}

	detail::native_string_type to_nstring(std::wstring&& text)
	{
		return ::nana::charset(text).to_bytes(::nana::unicode::utf8);
	}

	detail::native_string_type to_nstring(int n)
	{
		return std::to_string(n);
	}

	detail::native_string_type to_nstring(double d)
	{
		return std::to_string(d);
	}

	detail::native_string_type to_nstring(std::size_t d)
	{
		return std::to_string(d);
	}
#endif


}

#if defined(VERBOSE_PREPROCESSOR)
#	include <nana/verbose_preprocessor.hpp>
#endif
