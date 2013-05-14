/*
 *	Nana GUI Programming Interface Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/programming_interface.cpp
 */

#include <nana/gui/programming_interface.hpp>
#include <nana/system/platform.hpp>

namespace nana{	namespace gui{

	//restrict
	//		this name is only visible for this compiling-unit
	namespace restrict
	{
		typedef gui::detail::bedrock::core_window_t core_window_t;
		typedef gui::detail::bedrock::interface_type	interface_type;

		gui::detail::bedrock& bedrock = gui::detail::bedrock::instance();
		gui::detail::bedrock::window_manager_t& window_manager = bedrock.wd_manager;
	}

namespace API
{
	void effects_edge_nimbus(window wd, effects::edge_nimbus::t en)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				restrict::core_window_t::edge_nimbus_container & cont = iwd->root_widget->other.attribute.root->effects_edge_nimbus;
				if(en)
				{
					if(iwd->effect.edge_nimbus == effects::edge_nimbus::none)
					{
						restrict::core_window_t::edge_nimbus_action act = {iwd};
						cont.push_back(act);
					}

					iwd->effect.edge_nimbus = static_cast<effects::edge_nimbus::t>(iwd->effect.edge_nimbus | en);
				}
				else
				{
					if(iwd->effect.edge_nimbus)
					{
						for(restrict::core_window_t::edge_nimbus_container::iterator i = cont.begin(); i != cont.end(); ++i)
						{
							if(i->window == iwd)
							{
								cont.erase(i);
								break;
							}
						}						
					}
					iwd->effect.edge_nimbus = effects::edge_nimbus::none;
				}
			}
		}
	}

	effects::edge_nimbus::t effects_edge_nimbus(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
				return iwd->effect.edge_nimbus;
		}
		return effects::edge_nimbus::none;
	}

	namespace dev
	{
		void attach_drawer(window wd, drawer_trigger& dr)
		{
			if(wd)
			{
				restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
				internal_scope_guard isg;
				if(restrict::window_manager.available(iwd))
				{
					iwd->drawer.graphics.make(iwd->rect.width, iwd->rect.height);
					iwd->drawer.graphics.rectangle(0, 0, iwd->rect.width, iwd->rect.height, iwd->color.background, true);
					iwd->drawer.attached(dr);
					make_drawer_event<events::size>(wd);
					iwd->drawer.refresh();	//Always redrawe no matter it is visible or invisible. This can make the graphics data correctly.
				}
			}
		}

		void detach_drawer(window wd)
		{
			if(wd)
			{
				internal_scope_guard isg;
				if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
					reinterpret_cast<restrict::core_window_t*>(wd)->drawer.detached();
			}
		}

		void umake_drawer_event(window wd)
		{
			restrict::bedrock.evt_manager.umake(wd, true);
		}

		nana::string window_caption(window wd)
		{
			if(wd)
			{
				restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
				internal_scope_guard isg;

				if(restrict::window_manager.available(iwd))
				{
					if(iwd->other.category == category::root_tag::value)
						return restrict::interface_type::window_caption(iwd->root);
					return iwd->title;
				}
			}
			return nana::string();
		}

		void window_caption(window wd, const nana::string& title)
		{
			if(wd)
			{
				restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
				internal_scope_guard isg;

				if(restrict::window_manager.available(iwd))
				{
					iwd->title = title;
					if(iwd->other.category == category::root_tag::value)
						restrict::interface_type::window_caption(iwd->root, title);
				}
				restrict::window_manager.update(iwd, true, false);
			}
		}

		window create_window(window owner, bool nested, const rectangle& r, const appearance& ap)
		{
			return reinterpret_cast<window>(restrict::window_manager.create_root(reinterpret_cast<restrict::core_window_t*>(owner), nested, r, ap));
		}

		window create_widget(window parent, const rectangle& r)
		{
			return reinterpret_cast<window>(restrict::window_manager.create_widget(reinterpret_cast<restrict::core_window_t*>(parent), r, false));
		}

		window create_lite_widget(window parent, const rectangle& r)
		{
			return reinterpret_cast<window>(restrict::window_manager.create_widget(reinterpret_cast<restrict::core_window_t*>(parent), r, true));
		}

		window create_frame(window parent, const rectangle& r)
		{
			return reinterpret_cast<window>(restrict::window_manager.create_frame(reinterpret_cast<restrict::core_window_t*>(parent), r));
		}

		paint::graphics * window_graphics(window wd)
		{
			if(wd)
			{
				internal_scope_guard isg;
				if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
					return &reinterpret_cast<restrict::core_window_t*>(wd)->drawer.graphics;
			}
			return 0;		
		}
	}//end namespace dev

	//exit
	//close all windows in current thread
	void exit()
	{
		internal_scope_guard isg;

		std::vector<restrict::core_window_t*> v;
		restrict::window_manager.all_handles(v);
		if(v.size())
		{
			std::vector<native_window_type> roots;
			native_window_type root = 0;
			unsigned tid = nana::system::this_thread_id();
			for(std::vector<restrict::core_window_t*>::iterator i = v.begin(), end = v.end(); i != end; ++i)
			{
				restrict::core_window_t * wd = *i;
				if((wd->thread_id == tid) && (wd->root != root))
				{
					root = wd->root;
					if(roots.end() == std::find(roots.begin(), roots.end(), root))
						roots.push_back(root);
				}
			}

			std::for_each(roots.begin(), roots.end(), restrict::interface_type::close_window);
		}
	}

	//transform_shortkey_text
	//@brief:	This function searchs whether the text contains a '&' and removes the character for transforming.
	//			If the text contains more than one '&' charachers, the others are ignored. e.g
	//			text = "&&a&bcd&ef", the result should be "&abcdef", shortkey = 'b', and pos = 2.
	//@param, text: the text is transformed.
	//@param, shortkey: the character which indicates a short key.
	//@param, skpos: retrives the shortkey position if it is not a null_ptr;
	nana::string transform_shortkey_text(nana::string text, nana::string::value_type &shortkey, nana::string::size_type *skpos)
	{
		shortkey = 0;
		nana::string::size_type off = 0;
		while(true)
		{
			nana::string::size_type pos = text.find_first_of('&', off);
			if(pos != nana::string::npos)
			{
				text.erase(pos, 1);
				if(shortkey == 0 && pos < text.length())
				{
					shortkey = text.at(pos);
					if(shortkey == '&')	//This indicates the text contains "&&", it means the symbol have to be ignored.
						shortkey = 0;
					else if(skpos)
						*skpos = pos;
				}
				off = pos + 1;
			}
			else
				break;
		}
		return text;
	}

	bool register_shortkey(window wd, unsigned long key)
	{
		return restrict::window_manager.register_shortkey(reinterpret_cast<restrict::core_window_t*>(wd), key);
	}

	void unregister_shortkey(window wd)
	{
		restrict::window_manager.unregister_shortkey(reinterpret_cast<restrict::core_window_t*>(wd));
	}

	nana::size screen_size()
	{
		return restrict::interface_type::screen_size();
	}

	rectangle screen_area_from_point(const point& pos)
	{
		return restrict::interface_type::screen_area_from_point(pos);
	}

	point	cursor_position()
	{
		return restrict::interface_type::cursor_position();
	}

	rectangle make_center(unsigned width, unsigned height)
	{
		nana::size screen = restrict::interface_type::screen_size();
		nana::rectangle result(
			width > screen.width? 0: (screen.width - width)>>1,
			height > screen.height? 0: (screen.height - height)>> 1,
			width, height
		);

		return result;
	}

	nana::rectangle make_center(window wd, unsigned width, unsigned height)
	{
		nana::rectangle r = make_center(width, height);

		nana::point pos(r.x, r.y);
		calc_window_point(wd, pos);
		r.x = pos.x;
		r.y = pos.y;
		return r;
	}

	void window_icon_default(const paint::image& img)
	{
		restrict::window_manager.default_icon(img);
	}

	void window_icon(window wd, const paint::image& img)
	{
		restrict::window_manager.icon(reinterpret_cast<restrict::core_window_t*>(wd), img);
	}

	bool empty_window(window wd)
	{
		return (restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)) == false);
	}

	native_window_type root(window wd)
	{
		return restrict::bedrock.root(reinterpret_cast<restrict::core_window_t*>(wd));
	}

	window root(native_window_type wd)
	{
		return reinterpret_cast<window>(restrict::window_manager.root(wd));
	}

	bool enabled_double_click(window wd, bool dbl)
	{
		if(wd)
		{
			restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				bool result = iwd->flags.dbl_click;
				iwd->flags.dbl_click = dbl;
				return result;
			}
		}
		return false;
	}

	void fullscreen(window wd, bool v)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
				reinterpret_cast<restrict::core_window_t*>(wd)->flags.fullscreen = v;
		}
	}

	bool insert_frame(window frame, native_window_type native_window)
	{
		return restrict::window_manager.insert_frame(reinterpret_cast<restrict::core_window_t*>(frame), native_window);
	}

	native_window_type frame_container(window frame)
	{
		if(frame)
		{
			internal_scope_guard isg;
			if(reinterpret_cast<restrict::core_window_t*>(frame)->other.category == category::frame_tag::value)
				return reinterpret_cast<restrict::core_window_t*>(frame)->other.attribute.frame->container;
		}
		return 0;
	}

	native_window_type frame_element(window frame, unsigned index)
	{
		if(frame)
		{
			internal_scope_guard isg;
			if(reinterpret_cast<restrict::core_window_t*>(frame)->other.category == category::frame_tag::value)
			{
				if(index < reinterpret_cast<restrict::core_window_t*>(frame)->other.attribute.frame->attach.size())
					return reinterpret_cast<restrict::core_window_t*>(frame)->other.attribute.frame->attach.at(index);
			}
		}
		return 0;
	}

	void close_window(window wd)
	{
		restrict::window_manager.close(reinterpret_cast<restrict::core_window_t*>(wd));
	}

	void show_window(window wd, bool show)
	{
		restrict::window_manager.show(reinterpret_cast<restrict::core_window_t*>(wd), show);
	}

	bool visible(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				if(iwd->other.category == category::root_tag::value)
					return restrict::interface_type::is_window_visible(iwd->root);
				return iwd->visible;
			}
		}
		return false;
	}

	void restore_window(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				if(iwd->other.category == category::root_tag::value)
					restrict::interface_type::restore_window(iwd->root);
			}
		}
	}

	window get_parent_window(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
				return reinterpret_cast<window>(iwd->other.category == category::root_tag::value ? iwd->owner : iwd->parent);
		}
		return 0;
	}

	window get_owner_window(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && (iwd->other.category == category::root_tag::value))
			{
				native_window_type owner = restrict::interface_type::get_owner_window(iwd->root);
				if(owner)
					return reinterpret_cast<window>(restrict::window_manager.root(owner));
			}
		}
		return 0;
	}

	void umake_event(window wd)
	{
		restrict::bedrock.evt_manager.umake(wd, false);
	}

	void umake_event(event_handle eh)
	{
		restrict::bedrock.evt_manager.umake(eh);
	}

	nana::point window_position(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				return ( (iwd->other.category == category::root_tag::value) ?
					restrict::interface_type::window_position(iwd->root) : nana::point(iwd->rect.x, iwd->rect.y));
			}
		}
		return nana::point();
	}

	void move_window(window wd, int x, int y)
	{
		restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		if(restrict::window_manager.move(iwd, x, y, false))
		{
			if(category::root_tag::value != iwd->other.category)
				iwd = reinterpret_cast<restrict::core_window_t*>(API::get_parent_window(wd));
			restrict::window_manager.update(iwd, false, false);
		}
	}

	void move_window(window wd, int x, int y, unsigned width, unsigned height)
	{
		restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		if(restrict::window_manager.move(iwd, x, y, width, height))
		{
			if(category::root_tag::value != iwd->other.category)
				iwd = reinterpret_cast<restrict::core_window_t*>(API::get_parent_window(wd));
			restrict::window_manager.update(iwd, false, false);
		}
	}

	bool set_window_z_order(window wd, window wd_after, z_order_action::t action_if_no_wd_after)
	{
		if(wd)
		{
			restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				if(category::root_tag::value == iwd->other.category)
				{
					if(wd_after)
					{
						restrict::core_window_t * const iwd_after = reinterpret_cast<restrict::core_window_t*>(wd_after);
						if(restrict::window_manager.available(iwd_after) && (iwd_after->other.category == category::root_tag::value))
						{
							restrict::interface_type::set_window_z_order(iwd->root, iwd_after->root, z_order_action::none);
							return true;
						}
					}
					else
					{
						restrict::interface_type::set_window_z_order(iwd->root, 0, action_if_no_wd_after);
						return true;
					}
				}
			}
		}
		return false;
	}

	nana::size window_size(window wd)
	{
		nana::rectangle r;
		API::window_rectangle(wd, r);
		return nana::size(r.width, r.height);
	}

	void window_size(window wd, unsigned width, unsigned height)
	{
		restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		if(restrict::window_manager.size(iwd, width, height, false, false))
		{
			if(category::root_tag::value != iwd->other.category)
				iwd = reinterpret_cast<restrict::core_window_t*>(API::get_parent_window(wd));
			restrict::window_manager.update(iwd, false, false);
		}
	}

	bool window_rectangle(window wd, rectangle& rect)
	{
		if(wd)
		{
			restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				rect.x = iwd->rect.x;
				rect.y = iwd->rect.y;
				if(iwd->other.category == category::root_tag::value)
					restrict::interface_type::get_window_rect(iwd->root, rect);

				rect.width = iwd->rect.width;
				rect.height = iwd->rect.height;
				return true;
			}
		}
		return false;
	}

	bool track_window_size(window wd, const nana::size& sz, bool true_for_max)
	{
		if(wd == 0) return false;

		restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		if(restrict::window_manager.available(iwd) == false) return false;

		nana::size & ts = (true_for_max ? iwd->max_track_size : iwd->min_track_size);
		if(sz.width && sz.height)
		{
			if(true_for_max)
			{
				if(iwd->min_track_size.width <= sz.width && iwd->min_track_size.height <= sz.height)
				{
					ts = restrict::interface_type::check_track_size(sz, iwd->extra_width, iwd->extra_height, true);
					return true;
				}
			}
			else
			{
				if((iwd->max_track_size.width == 0 && iwd->max_track_size.height == 0) || (iwd->max_track_size.width >= sz.width && iwd->max_track_size.height >= sz.height))
				{
					ts = restrict::interface_type::check_track_size(sz, iwd->extra_width, iwd->extra_height, false);
					return true;
				}
			}
			return false;
		}
		else
			ts.width = ts.height = 0;
		return true;
	}

	void window_enabled(window wd, bool enabled)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && (iwd->flags.enabled != enabled))
			{
				iwd->flags.enabled = enabled;
				restrict::window_manager.update(iwd, true, false);
				if(category::root_tag::value == iwd->other.category)
					restrict::interface_type::enable_window(iwd->root, enabled);
			}
		}
	}

	bool window_enabled(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			return (restrict::window_manager.available(iwd) ? iwd->flags.enabled : false);
		}
		return false;

	}

	//lazy_refresh:
	//@brief: A widget drawer draws the widget surface in answering an event. This function will tell the drawer to copy the graphics into window after event answering.
	void lazy_refresh()
	{
		restrict::bedrock.thread_context_lazy_refresh();
	}

	//refresh_window
	//@brief: Refresh the window and display it immediately.
	void refresh_window(window wd)
	{
		restrict::window_manager.update(reinterpret_cast<restrict::core_window_t*>(wd), true, false);
	}

	//update_window
	//@brief: it displays a window immediately without refreshing.
	void update_window(window wnd)
	{
		restrict::window_manager.update(reinterpret_cast<restrict::core_window_t*>(wnd), false, true);
	}

	void window_caption(window wd, const nana::string& title)
	{
		if(wd)
		{
			restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
				restrict::window_manager.signal_fire_caption(iwd, title.c_str());
		}
	}

	nana::string window_caption(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
				return restrict::window_manager.signal_fire_caption(iwd);
		}
		return nana::string();
	}

	void window_cursor(window wd, cursor::t cur)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				iwd->predef_cursor = cur;
				restrict::bedrock.update_cursor(iwd);
			}
		}
	}

	cursor::t window_cursor(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
				return iwd->predef_cursor;
		}
		return cursor::arrow;
	}

	bool tray_insert(native_window_type wd, const nana::char_t* tip, const nana::char_t* ico)
	{
		return restrict::interface_type::notify_icon_add(wd, tip, ico);
	}

	bool tray_delete(native_window_type wd)
	{
		return restrict::interface_type::notify_icon_delete(wd);
	}

	void tray_tip(native_window_type wd, const char_t* text)
	{
		restrict::interface_type::notify_tip(wd, text);
	}

	void tray_icon(native_window_type wd, const char_t* icon)
	{
		restrict::interface_type::notify_icon(wd, icon);
	}

	bool tray_make_event(native_window_type wd, unsigned identifier, const nana::functor<void(const eventinfo&)> & f)
	{
		return restrict::window_manager.tray_make_event(wd, identifier, f);
	}

	void tray_umake_event(native_window_type wd)
	{
		restrict::window_manager.tray_umake_event(wd);
	}

	bool is_focus_window(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
				return (iwd->root_widget->other.attribute.root->focus == iwd);
		}
		return false;
	}

	window focus_window()
	{
		internal_scope_guard isg;
		return reinterpret_cast<window>(restrict::bedrock.focus());
	}

	void focus_window(window wd)
	{
		restrict::window_manager.set_focus(reinterpret_cast<restrict::core_window_t*>(wd));
		restrict::window_manager.update(reinterpret_cast<restrict::core_window_t*>(wd), false, false);
	}

	window capture_window()
	{
		return reinterpret_cast<window>(restrict::window_manager.capture_window());
	}

	window capture_window(window wd, bool value)
	{
		return reinterpret_cast<window>(
					restrict::window_manager.capture_window(reinterpret_cast<restrict::core_window_t*>(wd), value)
		);
	}

	void capture_ignore_children(bool ignore)
	{
		restrict::window_manager.capture_ignore_children(ignore);
	}

	void modal_window(window wd)
	{
		if(wd)
		{
			restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;

			wd = 0;
			if(restrict::window_manager.available(iwd))
			{
				if((iwd->other.category == category::root_tag::value) && (iwd->flags.modal == false))
				{
					iwd->flags.modal = true;
#if defined(NANA_X11)
					restrict::interface_type::set_modal(iwd->root);
#endif
					restrict::window_manager.show(iwd, true);
					wd = reinterpret_cast<window>(iwd);
				}
			}
		}

		if(wd)
		{
			//modal has to guarantee that does not lock the wnd_mgr_lock_ before invokeing the pump_event,
			//otherwise, the modal will prevent the other thread access the window.
			restrict::bedrock.pump_event(wd);
		}
	}

	nana::color_t foreground(window wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
				return reinterpret_cast<restrict::core_window_t*>(wd)->color.foreground;
		}
		return 0;
	}

	color_t foreground(window wd, color_t col)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				color_t prev = iwd->color.foreground;
				if(prev != col)
				{
					iwd->color.foreground = col;
					restrict::window_manager.update(iwd, true, false);
				}
				return prev;
			}
		}
		return 0;
	}

	color_t background(window wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
				return reinterpret_cast<restrict::core_window_t*>(wd)->color.background;
		}
		return 0;
	}

	color_t background(window wd, color_t col)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				color_t prev = iwd->color.background;
				if(prev != col)
				{
					iwd->color.background = col;
					restrict::window_manager.update(iwd, true, false);
				}
				return prev;
			}
		}
		return 0;
	}

	color_t active(window wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
				return reinterpret_cast<restrict::core_window_t*>(wd)->color.active;
		}
		return 0;	
	}

	color_t active(window wd, color_t col)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				color_t prev = iwd->color.active;
				if(prev != col)
				{
					iwd->color.active = col;
					restrict::window_manager.update(iwd, true, false);
				}
				return prev;
			}
		}
		return 0;
	}

	void create_caret(window wd, unsigned width, unsigned height)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && (0 == iwd->together.caret))
				iwd->together.caret = new detail::caret_descriptor<restrict::core_window_t, restrict::interface_type>(iwd, width, height);
		}
	}

	void destroy_caret(window wd)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				detail::caret_descriptor<restrict::core_window_t, restrict::interface_type>* p = iwd->together.caret;
				iwd->together.caret = 0;
				delete p;
			}
		}
	}

	void caret_pos(window wd, int x, int y)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && iwd->together.caret)
				iwd->together.caret->position(x, y);
		}
	}

	nana::point caret_pos(window wd)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && iwd->together.caret)
				return iwd->together.caret->position();
		}
		return point();
	}

	void caret_effective_range(window wd, const nana::rectangle& rect)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && iwd->together.caret)
				iwd->together.caret->effective_range(rect);
		}
	}

	void caret_size(window wd, const nana::size& sz)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && iwd->together.caret)
				iwd->together.caret->size(sz);
		}
	}

	nana::size caret_size(window wd)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && iwd->together.caret)
				return iwd->together.caret->size();
		}
		return nana::size();
	}

	void caret_visible(window wd, bool is_show)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && iwd->together.caret)
				iwd->together.caret->visible(is_show);
		}
	}

	bool caret_visible(window wd)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && iwd->together.caret)
				return iwd->together.caret->visible();
		}
		return false;
	}

	void tabstop(window wnd)
	{
		restrict::window_manager.tabstop(reinterpret_cast<restrict::core_window_t*>(wnd));
	}

	//eat_tabstop
	//@brief: set a eating tab window that it processes a pressing of tab itself
	void eat_tabstop(window wd, bool eat)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				if(eat)
					iwd->flags.tab |= detail::tab_type::eating;
				else
					iwd->flags.tab &= ~detail::tab_type::eating;
			}
		}
	}

	window move_tabstop(window wd, bool next)
	{
		restrict::core_window_t* ts_wd;
		if(next)
			ts_wd = restrict::window_manager.tabstop_next(reinterpret_cast<restrict::core_window_t*>(wd));
		else
			ts_wd = restrict::window_manager.tabstop_prev(reinterpret_cast<restrict::core_window_t*>(wd));

		restrict::window_manager.set_focus(ts_wd);
		restrict::window_manager.update(ts_wd, false, false);

		return reinterpret_cast<window>(ts_wd);
	}

	//glass_window
	//@brief: Test a window whether it is a glass attribute.
	bool glass_window(window wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
				return reinterpret_cast<restrict::core_window_t*>(wd)->flags.glass;
		}
		return false;
	}

	bool glass_window(window wd, bool isglass)
	{
		return restrict::window_manager.glass_window(reinterpret_cast<restrict::core_window_t*>(wd), isglass);
	}

	void take_active(window wd, bool active, window take_if_active_false)
	{
		if(wd)
		{
			restrict::core_window_t * const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			restrict::core_window_t * take_if_false = reinterpret_cast<restrict::core_window_t*>(take_if_active_false);
			internal_scope_guard isg;

			if(active || (take_if_false && (restrict::window_manager.available(take_if_false) == false)))
				take_if_false = 0;

			if(restrict::window_manager.available(iwd) == false) return;

			iwd->flags.take_active = active;
			iwd->other.active_window = take_if_false;
		}
	}

	bool window_graphics(window wd, nana::paint::graphics& graph)
	{
		return restrict::window_manager.get_graphics(reinterpret_cast<restrict::core_window_t*>(wd), graph);
	}

	bool root_graphics(window wd, nana::paint::graphics& graph)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				graph = *(iwd->root_graph);
				return true;
			}
		}
		return false;
	}

	bool get_visual_rectangle(window wd, nana::rectangle& r)
	{
		return restrict::window_manager.get_visual_rectangle(reinterpret_cast<restrict::core_window_t*>(wd), r);
	}

	void typeface(window wd, const nana::paint::font& font)
	{
		if(wd)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				iwd->drawer.graphics.typeface(font);
				restrict::window_manager.update(iwd, true, false);
			}
		}
	}

	nana::paint::font typeface(window wd)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
				return iwd->drawer.graphics.typeface();
		}
		return nana::paint::font();
	}

	bool calc_screen_point(window wd, nana::point& pos)
	{
		if(wd)
		{
			restrict::core_window_t* iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				pos.x += iwd->root_x;
				pos.y += iwd->root_y;
				return restrict::interface_type::calc_screen_point(iwd->root, pos);
			}
		}
		return false;
	}

	bool calc_window_point(window wd, nana::point& pos)
	{
		return restrict::window_manager.calc_window_point(reinterpret_cast<restrict::core_window_t*>(wd), pos);
	}

	window find_window(const nana::point& pos)
	{
		native_window_type wd = restrict::interface_type::find_window(pos.x, pos.y);
		if(wd)
		{
			nana::point clipos(pos.x, pos.y);
			restrict::interface_type::calc_window_point(wd, clipos);
			return reinterpret_cast<window>(
						restrict::window_manager.find_window(wd, clipos.x, clipos.y));
		}
		return 0;
	}

	void register_menu_window(window wd, bool has_keyboard)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
				restrict::bedrock.set_menu(reinterpret_cast<restrict::core_window_t*>(wd)->root, has_keyboard);
		}
	}

	bool attach_menubar(window menubar)
	{
		if(menubar)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(menubar);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) && (0 == iwd->root_widget->other.attribute.root->menubar))
			{
				iwd->root_widget->other.attribute.root->menubar = iwd;
				return true;
			}
		}
		return false;
	}

	void detach_menubar(window menubar)
	{
		if(menubar)
		{
			restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(menubar);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd) == false) return;
			if(iwd->root_widget->other.attribute.root->menubar == iwd)
				iwd->root_widget->other.attribute.root->menubar = 0;
		}
	}

	void restore_menubar_taken_window()
	{
		restrict::core_window_t * wd = restrict::bedrock.get_menubar_taken();
		if(wd)
		{
			internal_scope_guard isg;
			restrict::window_manager.set_focus(wd);
			restrict::window_manager.update(wd, true, false);
		}
	}

	bool is_window_zoomed(window wd, bool ask_for_max)
	{
		internal_scope_guard isg;
		restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		if(restrict::window_manager.available(iwd))
			return detail::bedrock::interface_type::is_window_zoomed(iwd->root, ask_for_max);
		return false;
	}

	gui::mouse_action::t mouse_action(window wd)
	{
		internal_scope_guard isg;
		restrict::core_window_t * iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		if(restrict::window_manager.available(iwd))
			return iwd->flags.action;
		return nana::gui::mouse_action::normal;
	}
}//end namespace API
}//end namespace gui
}//end namespace nana
