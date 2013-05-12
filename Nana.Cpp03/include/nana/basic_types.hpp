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

#include "deploy.hpp"

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
		struct mouse_action
		{
			enum t
			{
				begin, normal = begin, over, pressed, end
			};
		};
	}

	typedef unsigned scalar_t;
	typedef unsigned char	uint8_t;
	typedef unsigned long	uint32_t;
	typedef unsigned		uint_t;
	typedef unsigned		color_t;
#if defined(NANA_WINDOWS)
	typedef __int64	long_long_t;
#elif defined(NANA_LINUX)
	typedef long long long_long_t;
#endif

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

	struct point
	{
		point();
		point(int x, int y);
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
		rectangle(const point&, const size&);

		bool operator==(const rectangle&) const;
		bool operator!=(const rectangle&) const;
		rectangle & operator=(const point&);
		rectangle & operator=(const size&);

		rectangle& pare_off(int pixels);

		int x;
		int y;
		unsigned width;
		unsigned height;
	};

	struct arrange
	{
		enum t{unkown, horizontal, vertical, horizontal_vertical};
		t value;

		arrange();
		arrange(t);
		operator t() const;
		arrange& operator=(t);
		bool operator==(t) const;
		bool operator!=(t) const;
	};

	//The definition of text alignment
	struct align
	{
		enum t{
			left, center, right
		};
	};
}//end namespace nana

#endif


