/*
 *	Basic Types definition
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/basic_types.hpp
 */

#ifndef NANA_BASIC_TYPES_HPP
#define NANA_BASIC_TYPES_HPP

#include <nana/deploy.hpp>

namespace nana
{
	//A constant value for the invalid position.
	const std::size_t npos = static_cast<std::size_t>(-1);

	namespace detail
	{
		struct drawable_impl_type;	//declearation, defined in platform_spec.hpp
	}

	namespace paint
	{
		typedef nana::detail::drawable_impl_type*	drawable_type;
	}

	namespace gui
	{
		enum class mouse_action
		{
			begin, normal = begin, over, pressed, end
		};
	}

	typedef unsigned scalar_t;
	typedef unsigned char	uint8_t;
	typedef unsigned long	uint32_t;
	typedef unsigned		uint_t;
	typedef unsigned		color_t;
	typedef long long long_long_t;

	const color_t null_color = 0xFFFFFFFF;

	struct pixel_rgb_t
	{
		union
		{
			struct element_tag
			{
				unsigned int blue:8;
				unsigned int green:8;
				unsigned int red:8;
				unsigned int alpha_channel:8;
			}element;

			color_t color;
		}u;
	};

	struct rectangle;

	struct point
	{
		point();
		point(int x, int y);

		point& operator=(const rectangle&);
		bool operator==(const point&) const;
		bool operator!=(const point&) const;
		bool operator<(const point&) const;
		bool operator<=(const point&) const;
		bool operator>(const point&) const;
		bool operator>=(const point&) const;

		int x;
		int y;
	};

	struct upoint
	{
		typedef unsigned value_type;

		upoint();
		upoint(value_type x, value_type y);
		bool operator==(const upoint&) const;
		bool operator!=(const upoint&) const;
		bool operator<(const upoint&) const;
		bool operator<=(const upoint&) const;
		bool operator>(const upoint&) const;
		bool operator>=(const upoint&) const;

		value_type x;
		value_type y;
	};

	struct size
	{
		size();
		size(unsigned width, unsigned height);

		size& operator=(const rectangle&);
		bool is_zero() const;
		bool operator==(const size& rhs) const;
		bool operator!=(const size& rhs) const;

		unsigned width;
		unsigned height;
	};

	struct rectangle
	{
		rectangle();
		rectangle(int x, int y, unsigned width, unsigned height);
		rectangle(const size &);
		rectangle(const point&, const size& = size());

		bool operator==(const rectangle& rhs) const;
		bool operator!=(const rectangle& rhs) const;

		rectangle & operator=(const point&);
		rectangle & operator=(const size&);

		rectangle& pare_off(int pixels);

		int x;
		int y;
		unsigned width;
		unsigned height;
	};

	enum class arrange
	{
		unknown, horizontal, vertical, horizontal_vertical
	};

	//The definition of text alignment
	enum class align
	{
		left, center, right
	};
}//end namespace nana

#endif


