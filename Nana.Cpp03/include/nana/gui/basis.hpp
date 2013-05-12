/*
 *	Basis Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/basis.hpp
 *
 *	This file provides basis class and data structrue that required by gui
 */

#ifndef NANA_GUI_BASIS_HPP
#define NANA_GUI_BASIS_HPP

#include "../basic_types.hpp"
#include "../traits.hpp"	//metacomp::fixed_type_set

namespace nana
{
namespace gui
{
	namespace detail
	{
		struct native_window_handle_impl{};
		struct window_handle_impl{};
		struct event_handle_impl{};
	}

	typedef detail::native_window_handle_impl * native_window_type;
	typedef detail::window_handle_impl*	window;
	typedef detail::event_handle_impl*	event_handle;

	struct keyboard
	{
		enum t{
			//Control Code for ASCII
			select_all	= 0x1,
			copy		= 0x3,		//Ctrl+C
			backspace	= 0x8,	tab		= 0x9,
			enter_n		= 0xA,	enter	= 0xD,	enter_r = 0xD,
			alt			= 0x12,
			paste		= 0x16,		//Ctrl+V
			cut			= 0x18,		//Ctrl+X
			escape		= 0x1B,

			//System Code for OS
			os_pageup		= 0x21,	os_pagedown,
			os_arrow_left	= 0x25, os_arrow_up, os_arrow_right, os_arrow_down,
			os_insert		= 0x2D, os_del
		};
	};

	struct mouse
	{
		enum t
		{
			any_button, left_button, middle_button, right_button
		};
	};

	struct cursor
	{
		enum t
		{
			hand	= 60,
			arrow	= 68,
			wait	= 150,
			iterm	= 152, //A text caret
			size_we	= 108,
			size_ns	= 116,
			size_top_left = 134,
			size_top_right = 136,
			size_bottom_left = 12,
			size_bottom_right = 14
		};
	};

	namespace color
	{
		enum
		{
			white = 0xFFFFFF,
			button_face_shadow_start = 0xF5F4F2,
			button_face_shadow_end = 0xD5D2CA,
			button_face = 0xD4D0C8,
			dark_border	= 0x404040,
			gray_border	= 0x808080,
			highlight = 0x1CC4F7,
		};
	};

	struct z_order_action
	{
		enum t
		{
			none, bottom, top, topmost, foreground
		};
	};

	//Window appearance structure
	struct appearance
	{
		bool taskbar;
		bool floating;
		bool no_activate;

		bool minimize;
		bool maximize;
		bool sizable;

		bool decoration;

		appearance();
		appearance(bool has_decaration, bool taskbar, bool floating, bool no_activate, bool min, bool max, bool sizable);
	};

	struct appear
	{
		struct minimize{};
		struct maximize{};
		struct sizable{};
		struct taskbar{};
		struct floating{};
		struct no_activate{};

		template<typename Minimize = null_type,
					typename Maximize = null_type,
					typename Sizable = null_type,
					typename Floating = null_type,
					typename NoActive = null_type>
		struct decorate
		{
			typedef metacomp::fixed_type_set<Minimize, Maximize, Sizable, Floating, NoActive> set_type;

			operator appearance() const
			{
				return appearance(	true, true,
									set_type::template count<floating>::value,
									set_type::template count<no_activate>::value,
									set_type::template count<minimize>::value,
									set_type::template count<maximize>::value,
									set_type::template count<sizable>::value
									);
			}
		};

		template<typename Taskbar = null_type, typename Floating = null_type, typename NoActive = null_type, typename Minimize = null_type, typename Maximize = null_type, typename Sizable = null_type>
		struct bald
		{
			typedef metacomp::fixed_type_set<Taskbar, Floating, NoActive, Minimize, Maximize, Sizable> set_type;

			operator appearance() const
			{
				return appearance(	false,
									set_type::template count<taskbar>::value,
									set_type::template count<floating>::value,
									set_type::template count<no_activate>::value,
									set_type::template count<minimize>::value,
									set_type::template count<maximize>::value,
									set_type::template count<sizable>::value
									);
			}
		};

		template<bool HasDecoration = true, typename Sizable = null_type, typename Taskbar = null_type, typename Floating = null_type, typename NoActive = null_type>
		struct optional
		{
			typedef metacomp::fixed_type_set<Taskbar, Floating, NoActive> set_type;

			operator appearance() const
			{
				return appearance(HasDecoration,
									set_type::template count<taskbar>::value,
									set_type::template count<floating>::value,
									set_type::template count<no_activate>::value,
									true, true,
									set_type::template count<sizable>::value);
			}
		};
	};//end namespace apper
}//end namespace gui
}//end namespace nana

#endif


