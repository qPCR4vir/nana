/*
 *	Nana GUI Programming Interface Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/programming_interface.hpp
 */

#ifndef NANA_GUI_PROGRAMMING_INTERFACE_HPP
#define NANA_GUI_PROGRAMMING_INTERFACE_HPP
#include <nana/config.hpp>
#include GUI_BEDROCK_HPP
#include "effects.hpp"
#include <nana/paint/image.hpp>

namespace nana{	namespace gui{

namespace API
{
	void effects_edge_nimbus(window, effects::edge_nimbus);
	effects::edge_nimbus effects_edge_nimbus(window);

	//namespace dev
	//@brief: The interfaces defined in namespace dev are used for developing the nana.gui
	namespace dev
	{
		template<typename Object, typename Concept>
		bool attach_signal(window wd, Object& object, void (Concept::*f)(int, const gui::detail::signals&))
		{
			using namespace gui::detail;
			return bedrock::instance().wd_manager.attach_signal(reinterpret_cast<bedrock::core_window_t*>(wd), object, f);
		}

		template<typename Function>
		bool attach_signal(window wd, Function f)
		{
			using namespace gui::detail;
			return bedrock::instance().wd_manager.attach_signal(reinterpret_cast<bedrock::core_window_t*>(wd), f);
		}

		template<typename Event>
		event_handle make_drawer_event(window wd)
		{
			using namespace gui::detail;
			typedef typename std::remove_pointer<typename std::remove_reference<Event>::type>::type event_t;
			if(std::is_base_of<detail::event_type_tag, event_t>::value)
			{
				auto & wd_manager = bedrock::instance().wd_manager;
				internal_scope_guard isg;
				if(wd_manager.available(reinterpret_cast<bedrock::core_window_t*>(wd)))
					return reinterpret_cast<bedrock::core_window_t*>(wd)->drawer.make_event(event_t::identifier, wd);
			}
			return nullptr;
		}

		void attach_drawer(window, drawer_trigger&);
		void detach_drawer(window);
		void umake_drawer_event(window);
		nana::string window_caption(window);
		void window_caption(window, const nana::string& str);

		window create_window(window, bool nested, const rectangle&, const appearance&);
		window create_widget(window, const rectangle&);
		window create_lite_widget(window, const rectangle&);
		window create_frame(window, const rectangle&);

		paint::graphics* window_graphics(window);
	}//end namespace dev

	void exit();

	nana::string transform_shortkey_text(nana::string text, nana::string::value_type &shortkey, nana::string::size_type *skpos);
	bool register_shortkey(window, unsigned long);
	void unregister_shortkey(window);

	nana::size	screen_size();
	rectangle	screen_area_from_point(const point&);
	nana::point	cursor_position();
	rectangle make_center(unsigned width, unsigned height);
	rectangle make_center(window, unsigned width, unsigned height);

	void window_icon_default(const paint::image&);
	void window_icon(window, const paint::image&);
	bool empty_window(window);
	native_window_type root(window);
	window	root(native_window_type);

	void fullscreen(window, bool);
	bool enabled_double_click(window, bool);
	bool insert_frame(window frame, native_window_type);
	native_window_type frame_container(window frame);
	native_window_type frame_element(window frame, unsigned index);
	void close_window(window);
	void show_window(window, bool show);
	void restore_window(window);
	bool visible(window);
	window get_parent_window(window);
	window get_owner_window(window);

	template<typename Event, typename Function>
	event_handle make_event(window wd, Function function)
	{
		typedef typename std::remove_pointer<typename std::remove_reference<Event>::type>::type event_t;
		if(std::is_base_of<detail::event_type_tag, event_t>::value)
		{
			gui::detail::bedrock & b = gui::detail::bedrock::instance();
			return b.evt_manager.make(event_t::identifier, wd, b.category(reinterpret_cast<gui::detail::bedrock::core_window_t*>(wd)), function);
		}
		return 0;
	}

	template<typename Event>
	void raise_event(window wd, const eventinfo& ei)
	{
		typedef typename std::remove_pointer<typename std::remove_reference<Event>::type>::type event_t;
		if(std::is_base_of<detail::event_type_tag, event_t>::value)
			gui::detail::bedrock::raise_event(event_t::identifier, reinterpret_cast<gui::detail::bedrock::core_window_t*>(wd), ei, true);	
	}

	template<typename Event, typename Function>
	event_handle bind_event(window trigger, window listener, Function function)
	{
		typedef typename std::remove_pointer<typename std::remove_reference<Event>::type>::type event_t;
		if(std::is_base_of<detail::event_type_tag, event_t>::value)
		{
			gui::detail::bedrock & b = gui::detail::bedrock::instance();
			return b.evt_manager.bind(event_t::identifier, trigger, listener, b.category(reinterpret_cast<gui::detail::bedrock::core_window_t*>(trigger)), function);
		}
		
		return 0;
	}

	void umake_event(window);
	void umake_event(event_handle);
	nana::point window_position(window);
	void move_window(window, int x, int y);
	void move_window(window, int x, int y, unsigned width, unsigned height);
	bool set_window_z_order(window wd, window wd_after, z_order_action action_if_no_wd_after);

	nana::size window_size(window);
	void window_size(window, unsigned width, unsigned height);
	bool window_rectangle(window, rectangle& rect);
	bool track_window_size(window, const nana::size&, bool true_for_max);
	void window_enabled(window, bool);
	bool window_enabled(window);
	//lazy_refresh:
	//@brief: A widget drawer draws the widget surface in answering an event. this function will tell the drawer to copy the graphics into window after event answering.
	void lazy_refresh();

	//refresh_window
	//@brief:	calls refresh() of a widget's drawer. if currently state is lazy_refresh, Nana.GUI may paste the drawing on the window after an event processing.
	//			param@window: specify a window to be refreshed.
	void refresh_window(window);
	void update_window(window);

	void window_caption(window, const nana::string& title);
	nana::string window_caption(window);

	void window_cursor(window, cursor);
	cursor window_cursor(window);

	bool tray_insert(native_window_type, const char_t* tip, const char_t* ico);
	bool tray_delete(native_window_type);
	void tray_tip(native_window_type, const char_t* text);
	void tray_icon(native_window_type, const char_t* icon);
	bool tray_make_event(native_window_type, unsigned identifier, const std::function<void(const eventinfo&)>&);
	void tray_umake_event(native_window_type);

	bool is_focus_window(window);
	window focus_window();
	void focus_window(window);

	window	capture_window();
	window	capture_window(window, bool);
	void	capture_ignore_children(bool ignore);
	void modal_window(window);

	color_t foreground(window);
	color_t foreground(window, color_t);
	color_t background(window);
	color_t background(window, color_t);
	color_t	active(window);
	color_t	active(window, color_t);

	void create_caret(window, unsigned width, unsigned height);
	void destroy_caret(window);
	void caret_effective_range(window, const rectangle&);
	void caret_pos(window, int x, int y);
	nana::point caret_pos(window);
	nana::size caret_size(window);
	void caret_size(window, const size&);
	void caret_visible(window, bool is_show);
	bool caret_visible(window);

	void tabstop(window);
	void eat_tabstop(window, bool);
	window move_tabstop(window, bool next);
	
	bool glass_window(window);
	bool glass_window(window, bool);

	void take_active(window, bool has_active, window take_if_has_active_false);

	bool window_graphics(window, nana::paint::graphics&);
	bool root_graphics(window, nana::paint::graphics&);
	bool get_visual_rectangle(window, nana::rectangle&);

	void typeface(window, const nana::paint::font&);
	paint::font typeface(window);

	bool calc_screen_point(window, point&);
	bool calc_window_point(window, point&);

	window find_window(const nana::point& mspos);

	void register_menu_window(window, bool has_keyboard);
	bool attach_menubar(window menubar);
	void detach_menubar(window menubar);
	void restore_menubar_taken_window();

	bool is_window_zoomed(window, bool ask_for_max);

	nana::gui::mouse_action mouse_action(window);
}//end namespace API
}//end namespace gui
}//end namespace nana

#endif

