/*
 *	A Bedrock Implementation
 *	Copyright(C) 2003-2012 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://nanapro.sourceforge.net/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/linux_X11/bedrock.cpp
 */

#include <nana/config.hpp>
#include PLATFORM_SPEC_HPP
#include GUI_BEDROCK_HPP
#include <nana/gui/detail/eventinfo.hpp>
#include <nana/system/platform.hpp>
#include <errno.h>

namespace nana
{
namespace gui
{
	//class internal_scope_guard
		internal_scope_guard::internal_scope_guard()
		{
			detail::bedrock::instance().wd_manager.internal_lock().lock();
		}
		internal_scope_guard::~internal_scope_guard()
		{
			detail::bedrock::instance().wd_manager.internal_lock().unlock();
		}
	//end class internal_scope_guard
namespace detail
{
#pragma pack(1)
		union event_mask
		{
			struct
			{
				short x;
				short y;
			}pos;

			struct
			{
				short width;
				short height;
			}size;

			struct
			{
				unsigned short vkey;
				short delta;
			}wheel;
		};
#pragma pack()

	struct bedrock::thread_context
	{
		unsigned event_pump_ref_count;

		int		window_count;	//The number of windows
		core_window_t* event_window;

		struct platform_detail_tag
		{
			nana::char_t keychar;
			native_window_type	motion_window;
			nana::point		motion_pointer_pos;
		}platform;

		struct cursor_tag
		{
			core_window_t * window;
			nana::gui::cursor predef_cursor;
			Cursor handle;
		}cursor;

		thread_context()
			: event_pump_ref_count(0), window_count(0), event_window(0)
		{
			cursor.window = 0;
			cursor.predef_cursor = nana::gui::cursor::arrow;
			cursor.handle = 0;
		}
	};
	
	struct bedrock::private_impl
	{
		typedef std::map<unsigned, thread_context> thr_context_container;
		std::recursive_mutex mutex;
		thr_context_container thr_contexts;

		struct cache_type
		{
			struct thread_context_cache
			{
				unsigned tid;
				thread_context *object;
			}tcontext;

			cache_type()
			{
				tcontext.tid = 0;
				tcontext.object = nullptr;
			}
		}cache;

		struct menu_tag
		{
			menu_tag()
				:taken_window(nullptr), window(nullptr), owner(nullptr), has_keyboard(false)
			{}

			core_window_t*	taken_window;
			native_window_type window;
			native_window_type owner;
			bool has_keyboard;
		}menu;

		struct keyboard_tracking_state
		{
			keyboard_tracking_state()
				:has_shortkey_occured(false), has_keyup(true), alt(0)
			{}

			bool has_shortkey_occured;
			bool has_keyup;

			unsigned long alt : 2;
		}keyboard_tracking_state;
	};

	void timer_proc(unsigned);
	void window_proc_dispatcher(Display*, nana::detail::msg_packet_tag&);
	void window_proc_for_packet(Display *, nana::detail::msg_packet_tag&);
	void window_proc_for_xevent(Display*, XEvent&);

	//class bedrock defines a static object itself to implement a static singleton
	//here is the definition of this object
	bedrock bedrock::bedrock_object;

	inline window mycast(bedrock::core_window_t* wd)
	{
		return reinterpret_cast<window>(wd);
	}

	Window event_window(const XEvent& event)
	{
		switch(event.type)
		{
		case MapNotify:
		case UnmapNotify:
		case DestroyNotify:
			return event.xmap.window;
		}
		return event.xkey.window;
	}

	bedrock::bedrock()
		: impl_(new private_impl)
	{
		nana::detail::platform_spec::instance().msg_set(timer_proc, window_proc_dispatcher);
	}

	bedrock::~bedrock()
	{
		delete impl_;
	}

	void bedrock::map_thread_root_buffer(bedrock::core_window_t* wnd)
	{
	//	::PostMessage(reinterpret_cast<HWND>(wnd->root), gui::messages::map_thread_root_buffer, reinterpret_cast<WPARAM>(wnd), 0);
	}

	//inc_window
	//@biref: increament the number of windows
	int bedrock::inc_window(unsigned tid)
	{
		private_impl * impl = instance().impl_;
		std::lock_guard<decltype(impl->mutex)> lock(impl->mutex);

		int & cnt = (impl->thr_contexts[tid ? tid : nana::system::this_thread_id()].window_count);
		return (cnt < 0 ? cnt = 1 : ++cnt);
	}

	bedrock::thread_context* bedrock::open_thread_context(unsigned tid)
	{
		if(0 == tid) tid = nana::system::this_thread_id();

		std::lock_guard<decltype(impl_->mutex)> lock(impl_->mutex);
		if(impl_->cache.tcontext.tid == tid)
			return impl_->cache.tcontext.object;

		bedrock::thread_context* context = 0;

		private_impl::thr_context_container::iterator i = impl_->thr_contexts.find(tid);
		if(i == impl_->thr_contexts.end())
			context = &(impl_->thr_contexts[tid]);
		else
			context = &(i->second);

		impl_->cache.tcontext.tid = tid;
		impl_->cache.tcontext.object = context;
		return context;
	}

	bedrock::thread_context* bedrock::get_thread_context(unsigned tid)
	{
		if(0 == tid) tid = nana::system::this_thread_id();

		std::lock_guard<decltype(impl_->mutex)> lock(impl_->mutex);
		if(impl_->cache.tcontext.tid == tid)
			return impl_->cache.tcontext.object;

		private_impl::thr_context_container::iterator i = impl_->thr_contexts.find(tid);
		if(i != impl_->thr_contexts.end())
		{
			impl_->cache.tcontext.tid = tid;
			return (impl_->cache.tcontext.object = &(i->second));
		}

		impl_->cache.tcontext.tid = 0;
		return 0;
	}

	void bedrock::remove_thread_context(unsigned tid)
	{
		if(0 == tid) tid = nana::system::this_thread_id();

		std::lock_guard<decltype(impl_->mutex)> lock(impl_->mutex);

		if(impl_->cache.tcontext.tid == tid)
		{
			impl_->cache.tcontext.tid = 0;
			impl_->cache.tcontext.object = nullptr;
		}

		impl_->thr_contexts.erase(tid);
	}

	bedrock& bedrock::instance()
	{
		return bedrock_object;
	}

	gui::category::flags bedrock::category(bedrock::core_window_t* wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(wd_manager.available(wd))
				return wd->other.category;
		}
		return gui::category::flags::super;
	}

	bedrock::core_window_t* bedrock::focus()
	{
		core_window_t* wd = wd_manager.root(native_interface::get_focus_window());
		return (wd ? wd->other.attribute.root->focus : 0);
	}

	native_window_type bedrock::root(core_window_t* wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(wd_manager.available(wd))
				return wd->root;
		}
		return nullptr;
	}

	void bedrock::set_menubar_taken(core_window_t* wd)
	{
		impl_->menu.taken_window = wd;
	}

	bedrock::core_window_t* bedrock::get_menubar_taken()
	{
		core_window_t* wd = impl_->menu.taken_window;
		impl_->menu.taken_window = nullptr;
		return wd;
	}

	bool bedrock::close_menu_if_focus_other_window(native_window_type wd)
	{
		if(impl_->menu.window && (impl_->menu.window != wd))
		{
			wd = native_interface::get_owner_window(wd);
			while(wd)
			{
				if(wd != impl_->menu.window)
					wd = native_interface::get_owner_window(wd);
				else
					return false;
			}
			remove_menu();
			return true;
		}
		return false;
	}

	void bedrock::set_menu(native_window_type menu_window, bool has_keyboard)
	{
		if(menu_window && impl_->menu.window != menu_window)
		{
			remove_menu();
			impl_->menu.window = menu_window;
			impl_->menu.owner = native_interface::get_owner_window(menu_window);
			impl_->menu.has_keyboard = has_keyboard;
		}
	}

	native_window_type bedrock::get_menu(native_window_type owner, bool is_keyboard_condition)
	{
		if(	(impl_->menu.owner == nullptr) ||
			(owner && (impl_->menu.owner == owner))
			)
		{
			return ( is_keyboard_condition ? (impl_->menu.has_keyboard ? impl_->menu.window : nullptr) : impl_->menu.window);
		}

		return 0;
	}

	native_window_type bedrock::get_menu()
	{
		return impl_->menu.window;
	}

	void bedrock::remove_menu()
	{
		if(impl_->menu.window)
		{
			native_window_type delwin = impl_->menu.window;
			impl_->menu.window = impl_->menu.owner = nullptr;
			impl_->menu.has_keyboard = false;
			native_interface::close_window(delwin);
		}
	}

	void bedrock::empty_menu()
	{
		if(impl_->menu.window)
		{
			impl_->menu.window = impl_->menu.owner = nullptr;
			impl_->menu.has_keyboard = false;
		}
	}

	void bedrock::get_key_state(nana::gui::detail::tag_keyboard& kb)
	{
		XKeyEvent xkey;
		nana::detail::platform_spec::instance().read_keystate(xkey);
		kb.ctrl = (xkey.state & ControlMask);
	}

	bool bedrock::set_keyboard_shortkey(bool yes)
	{
		bool ret = impl_->keyboard_tracking_state.has_shortkey_occured;
		impl_->keyboard_tracking_state.has_shortkey_occured = yes;
		return ret;
	}

	void make_eventinfo(eventinfo& ei, detail::bedrock::core_window_t* wd, unsigned int msg, const XEvent& event)
	{
		ei.window = reinterpret_cast<window>(wd);
		if(msg == ButtonPress || msg == ButtonRelease)
		{
			if(event.xbutton.button == Button4 || event.xbutton.button == Button5)
			{
				ei.wheel.upwards = (event.xbutton.button == Button4);
				ei.wheel.x = event.xbutton.x - wd->pos_root.x;
				ei.wheel.y = event.xbutton.y - wd->pos_root.y;
			}
			else
			{
				ei.mouse.x = event.xbutton.x - wd->pos_root.x;
				ei.mouse.y = event.xbutton.y - wd->pos_root.y;

				ei.mouse.left_button = ei.mouse.mid_button = ei.mouse.right_button = false;
				ei.mouse.shift = ei.mouse.ctrl = false;
				switch(event.xbutton.button)
				{
				case Button1:
					ei.mouse.left_button = true;
					break;
				case Button2:
					ei.mouse.mid_button = true;
					break;
				case Button3:
					ei.mouse.right_button = true;
					break;
				}
			}
		}
		else if(msg == MotionNotify)
		{
			ei.mouse.x = event.xmotion.x - wd->pos_root.x;
			ei.mouse.y = event.xmotion.y - wd->pos_root.y;
			ei.mouse.left_button = ei.mouse.mid_button = ei.mouse.right_button = false;

			ei.mouse.shift = event.xmotion.state & ShiftMask;
			ei.mouse.ctrl = event.xmotion.state & ControlMask;
			if(event.xmotion.state & Button1Mask)
				ei.mouse.left_button = true;
			else if(event.xmotion.state & Button2Mask)
				ei.mouse.right_button = true;
			else if(event.xmotion.state & Button3Mask)
				ei.mouse.mid_button = true;
		}
		else if(EnterNotify == msg)
		{
			ei.mouse.x = event.xcrossing.x - wd->pos_root.x;
			ei.mouse.y = event.xcrossing.y - wd->pos_root.y;
			ei.mouse.left_button = ei.mouse.mid_button = ei.mouse.right_button = false;

			ei.mouse.shift = event.xcrossing.state & ShiftMask;
			ei.mouse.ctrl = event.xcrossing.state & ControlMask;
			if(event.xcrossing.state & Button1Mask)
				ei.mouse.left_button = true;
			else if(event.xcrossing.state & Button2Mask)
				ei.mouse.right_button = true;
			else if(event.xcrossing.state & Button3Mask)
				ei.mouse.mid_button = true;
		}
	}

	void timer_proc(unsigned tid)
	{
		nana::detail::platform_spec::instance().timer_proc(tid);
	}

	void window_proc_dispatcher(Display* display, nana::detail::msg_packet_tag& msg)
	{
		switch(msg.kind)
		{
		case nana::detail::msg_packet_tag::kind_xevent:
			window_proc_for_xevent(display, msg.u.xevent);
			break;
		case nana::detail::msg_packet_tag::kind_mouse_drop:
			window_proc_for_packet(display, msg);
			break;
		default: break;
		}
	}

	void window_proc_for_packet(Display * display, nana::detail::msg_packet_tag& msg)
	{
		static auto& bedrock = detail::bedrock::instance();

		auto native_window = reinterpret_cast<native_window_type>(msg.u.packet_window);
		auto root_runtime = bedrock.wd_manager.root_runtime(native_window);

		if(root_runtime)
		{
			auto msgwd = root_runtime->window;

			eventinfo ei;
			switch(msg.kind)
			{
			case nana::detail::msg_packet_tag::kind_mouse_drop:
				msgwd = bedrock.wd_manager.find_window(native_window, msg.u.mouse_drop.x, msg.u.mouse_drop.y);
				if(msgwd)
				{
					detail::tag_dropinfo di;
					di.filenames.swap(*msg.u.mouse_drop.files);
					delete msg.u.mouse_drop.files;
					di.pos.x = msg.u.mouse_drop.x - msgwd->pos_root.x;
					di.pos.y = msg.u.mouse_drop.y - msgwd->pos_root.y;
					ei.dropinfo = & di;
					ei.window = reinterpret_cast<window>(msgwd);
					
					bedrock.fire_event(event_tag::mouse_drop, msgwd, ei);
					bedrock.wd_manager.refresh(msgwd);
				}
				break;
			default:
				throw std::runtime_error("Nana.GUI.Bedrock: Undefined message packet");
			}
		}		

	}

	void window_proc_for_xevent(Display* display, XEvent& xevent)
	{
		typedef detail::bedrock::core_window_t core_window_t;
		typedef detail::bedrock::window_manager_t::root_table_type::value_type wm_root_runtime_type;

		static auto& bedrock = detail::bedrock::instance();
		static unsigned long	last_mouse_down_time;
		static core_window_t*	last_mouse_down_window;

		auto native_window = reinterpret_cast<native_window_type>(event_window(xevent));
		auto root_runtime = bedrock.wd_manager.root_runtime(native_window);

		if(root_runtime)
		{
			auto msgwnd = root_runtime->window;
			auto& context = *bedrock.get_thread_context(msgwnd->thread_id);

			auto pre_event_window = context.event_window;
			auto mouse_window = root_runtime->condition.mouse_window;
			auto mousemove_window = root_runtime->condition.mousemove_window;

			eventinfo ei;

			const int message = xevent.type;
			switch(xevent.type)
			{
			case EnterNotify:
				msgwnd = bedrock.wd_manager.find_window(native_window, xevent.xcrossing.x, xevent.xcrossing.y);
				if(msgwnd)
				{
					make_eventinfo(ei, msgwnd, message, xevent);
					msgwnd->flags.action = mouse_action::over;
					
					root_runtime->condition.mousemove_window = msgwnd;
					mousemove_window = msgwnd;
					bedrock.raise_event(event_tag::mouse_enter, msgwnd, ei, true);
					bedrock.raise_event(event_tag::mouse_move, msgwnd, ei, true);
					if(false == bedrock.wd_manager.available(mousemove_window))
						mousemove_window = nullptr;
				}
				break;
			case LeaveNotify:
				if(bedrock.wd_manager.available(mousemove_window) && mousemove_window->flags.enabled)
				{
					ei.mouse.x = ei.mouse.y = 0;
					mousemove_window->flags.action = mouse_action::normal;
					ei.window = reinterpret_cast<window>(mousemove_window);
					bedrock.raise_event(event_tag::mouse_leave, mousemove_window, ei, true);
				}
				mousemove_window = 0;
				break;
			case FocusIn:
				if(msgwnd->flags.enabled && msgwnd->flags.take_active)
				{
					auto focus = msgwnd->other.attribute.root->focus;
					if(focus && focus->together.caret)
						focus->together.caret->set_active(true);
					msgwnd->root_widget->other.attribute.root->context.focus_changed = true;
					ei.focus.getting = true;
					ei.focus.receiver = native_window;
					if(false == bedrock.raise_event(event_tag::focus, focus, ei, true))
						bedrock.wd_manager.set_focus(msgwnd);
				}
				break;
			case FocusOut:
				if(msgwnd->other.attribute.root->focus && native_interface::is_window(msgwnd->root))
				{
					nana::point pos = native_interface::cursor_position();
					auto recv = native_interface::find_window(pos.x, pos.y);

					auto focus = msgwnd->other.attribute.root->focus;
					ei.focus.getting = false;
					ei.focus.receiver = recv;
					if(bedrock.raise_event(event_tag::focus, focus, ei, true))
					{
						if(focus->together.caret)
							focus->together.caret->set_active(false);
					}
					bedrock.close_menu_if_focus_other_window(recv);
				}
				break;
			case ConfigureNotify:
				if(msgwnd->dimension.width != static_cast<unsigned>(xevent.xconfigure.width) || msgwnd->dimension.height != static_cast<unsigned>(xevent.xconfigure.height))
				{
					ei.size.width = xevent.xconfigure.width;
					ei.size.height = xevent.xconfigure.height;
					bedrock.wd_manager.size(msgwnd, ei.size.width, ei.size.height, true, true);
				}
				
				if(msgwnd->pos_native.x != xevent.xconfigure.x || msgwnd->pos_native.y != xevent.xconfigure.y)
				{
					msgwnd->pos_native.x = xevent.xconfigure.x;
					msgwnd->pos_native.y = xevent.xconfigure.y;
					bedrock.event_move(msgwnd, xevent.xconfigure.x, xevent.xconfigure.y);
				}
				break;
			case ButtonPress:
				if(xevent.xbutton.button == Button4 || xevent.xbutton.button == Button5)
					break;
					
				msgwnd = bedrock.wd_manager.find_window(native_window, xevent.xbutton.x, xevent.xbutton.y);
				if(nullptr == msgwnd) break;
					
				if((msgwnd == msgwnd->root_widget->other.attribute.root->menubar) && bedrock.get_menu(msgwnd->root, true))
					bedrock.remove_menu();
				else
					bedrock.close_menu_if_focus_other_window(msgwnd->root);

				if(msgwnd && msgwnd->flags.enabled)
				{
					bool dbl_click = (last_mouse_down_window == msgwnd) && (xevent.xbutton.time - last_mouse_down_time <= 400);
					last_mouse_down_time = xevent.xbutton.time;
					last_mouse_down_window = msgwnd;

					mouse_window = msgwnd;
					auto new_focus = (msgwnd->flags.take_active ? msgwnd : msgwnd->other.active_window);

					if(new_focus)
					{
						context.event_window = new_focus;
						auto kill_focus = bedrock.wd_manager.set_focus(new_focus);
						if(kill_focus != new_focus)
							bedrock.wd_manager.do_lazy_refresh(kill_focus, false);
					}
					msgwnd->root_widget->other.attribute.root->context.focus_changed = false;

					context.event_window = msgwnd;

					make_eventinfo(ei, msgwnd, message, xevent);
					msgwnd->flags.action = mouse_action::pressed;
					if(bedrock.raise_event(dbl_click ? event_tag::dbl_click : event_tag::mouse_down, msgwnd, ei, true))
					{
						if(bedrock.wd_manager.available(mouse_window))
						{
							//If a root window is created during the mouse_down event, Nana.GUI will ignore the mouse_up event.
							if(msgwnd->root_widget->other.attribute.root->context.focus_changed)
							{
								//call the drawer mouse up event for restoring the surface graphics
								msgwnd->flags.action = mouse_action::normal;
								bedrock.fire_event_for_drawer(event_tag::mouse_up, msgwnd, ei, &context);
								bedrock.wd_manager.do_lazy_refresh(msgwnd, false);
							}
						}
						else
							mouse_window = nullptr;
					}
					else
						mouse_window = nullptr;
				}
				break;
			case ButtonRelease:
				if(xevent.xbutton.button == Button4 || xevent.xbutton.button == Button5)
				{
					msgwnd = bedrock.focus();
					if(msgwnd && msgwnd->flags.enabled)
					{
						make_eventinfo(ei, msgwnd, message, xevent);
						bedrock.raise_event(event_tag::mouse_wheel, msgwnd, ei, true);
					}
				}
				else if(bedrock.wd_manager.available(mouse_window) && mouse_window->flags.enabled)
				{
					make_eventinfo(ei, mouse_window, message, xevent);
					msgwnd = bedrock.wd_manager.find_window(native_window, xevent.xbutton.x, xevent.xbutton.y);
					core_window_t* click_window = nullptr;
					if(msgwnd == mouse_window)
					{
						mouse_window->flags.action = mouse_action::over;
						click_window = mouse_window;
						context.event_window = msgwnd;
						bedrock.fire_event_for_drawer(event_tag::click, msgwnd, ei, &context);
					}
					else
					{
						mouse_window->flags.action = mouse_action::normal;
						msgwnd = mouse_window;
					}
					context.event_window = msgwnd;
					bedrock.fire_event_for_drawer(event_tag::mouse_up, msgwnd, ei, &context);
					bedrock.wd_manager.refresh(msgwnd);

					if(click_window)
					{
						context.event_window = click_window;
						bedrock.fire_event(event_tag::click, click_window, ei);
						if(click_window != msgwnd)
							bedrock.wd_manager.do_lazy_refresh(click_window, false);
					}

					context.event_window = msgwnd;
					bedrock.fire_event(event_tag::mouse_up, msgwnd, ei);
					bedrock.wd_manager.do_lazy_refresh(msgwnd, false);

					mouse_window = nullptr;
				}
				break;
			case DestroyNotify:
				{
					auto & spec = nana::detail::platform_spec::instance();
					if(bedrock.wd_manager.available(msgwnd))
					{
						//The msgwnd may be destroyed if the window is destroyed by calling native interface of close_window().
						if(msgwnd->root == bedrock.get_menu())
							bedrock.empty_menu();

						spec.remove(native_window);
						bedrock.wd_manager.destroy(msgwnd);
						bedrock.evt_manager.umake(reinterpret_cast<gui::window>(msgwnd), false);

						bedrock.rt_manager.remove_if_exists(msgwnd);
						bedrock.wd_manager.destroy_handle(msgwnd);
					}
					if(--context.window_count <= 0)
					{

					}
				}
				break;
			case MotionNotify:
				//X may send the MotionNotify with same information repeatly.
				//Nana should ignore the repeated notify.
				if(context.platform.motion_window != native_window || context.platform.motion_pointer_pos != nana::point(xevent.xmotion.x, xevent.xmotion.y))
				{
					context.platform.motion_window = native_window;
					context.platform.motion_pointer_pos = nana::point(xevent.xmotion.x, xevent.xmotion.y);
				}
				else
					break;

				msgwnd = bedrock.wd_manager.find_window(native_window, xevent.xmotion.x, xevent.xmotion.y);
				if(msgwnd)
				{
					if(mousemove_window && (msgwnd != mousemove_window))
					{
						auto leave_wd = mousemove_window;
						root_runtime->condition.mousemove_window = nullptr;
						mousemove_window = nullptr;
						//if current window is not the previous mouse event window.
						make_eventinfo(ei, leave_wd, message, xevent);
						leave_wd->flags.action = mouse_action::normal;
						bedrock.raise_event(event_tag::mouse_leave, leave_wd, ei, true);

						//if msgwnd is neither a captured window nor a child of captured window,
						//redirect the msgwnd to the captured window.
						auto cap_wd = bedrock.wd_manager.capture_redirect(msgwnd);
						if(cap_wd)
							msgwnd = cap_wd;
					}
					else
					{
						make_eventinfo(ei, msgwnd, message, xevent);
						bool prev_captured_inside;
						if(bedrock.wd_manager.capture_window_entered(xevent.xmotion.x, xevent.xmotion.y, prev_captured_inside))
						{
							unsigned eid;
							if(prev_captured_inside)
							{
								eid = event_tag::mouse_leave;
								msgwnd->flags.action = mouse_action::normal;
							}
							else
							{
								eid = event_tag::mouse_enter;
								msgwnd->flags.action = mouse_action::over;
							}
							bedrock.raise_event(eid, msgwnd, ei, false);
						}
					}

					if(msgwnd)
					{
						make_eventinfo(ei, msgwnd, message, xevent);
						msgwnd->flags.action = mouse_action::over;
						if(mousemove_window != msgwnd)
						{
							root_runtime->condition.mousemove_window = msgwnd;
							mousemove_window = msgwnd;
							bedrock.raise_event(event_tag::mouse_enter, msgwnd, ei, true);
						}
						bedrock.raise_event(event_tag::mouse_move, msgwnd, ei, true);
					}
					if(false == bedrock.wd_manager.available(mousemove_window))
						mousemove_window = nullptr;
				}
				break;
			case MapNotify:
			case UnmapNotify:
				bedrock.event_expose(msgwnd, (xevent.type == MapNotify));
				context.platform.motion_window = nullptr;
				break;
			case Expose:
				if(msgwnd->visible && (msgwnd->root_graph->empty() == false))
				{
					nana::detail::platform_scope_guard psg;
					nana::detail::drawable_impl_type* drawer_impl = msgwnd->root_graph->handle();
					::XCopyArea(display, drawer_impl->pixmap, reinterpret_cast<Window>(native_window), drawer_impl->context,
							xevent.xexpose.x, xevent.xexpose.y,
							xevent.xexpose.width, xevent.xexpose.height,
							xevent.xexpose.x, xevent.xexpose.y);
				}
				break;
			case KeyPress:
				nana::detail::platform_spec::instance().write_keystate(xevent.xkey);
				if(msgwnd->flags.enabled)
				{
					if(msgwnd->root != bedrock.get_menu())
						msgwnd = bedrock.focus();

					if(msgwnd)
					{
						KeySym keysym;
						Status status;
						char fixbuf[33];
						char * keybuf = fixbuf;
						int len = 0;
						XIC input_context = nana::detail::platform_spec::instance().caret_input_context(native_window);
						if(input_context)
						{
							nana::detail::platform_scope_guard psg;
#if defined(NANA_UNICODE)
							len = ::Xutf8LookupString(input_context, &xevent.xkey, keybuf, 32, &keysym, &status);
							if(status == XBufferOverflow)
							{
								keybuf = new char[len + 1];
								len = ::Xutf8LookupString(input_context, &xevent.xkey, keybuf, len, &keysym, &status);
							}
#else
							len = ::XmbLookupString(input_context, &xevent.xkey, keybuf, 32, &keysym, &status);
							if(status == XBufferOverflow)
							{
								keybuf = new char[len + 1];
								len = ::XmbLookupString(input_context, &xevent.xkey, keybuf, len, &keysym, &status);
							}
#endif
						}
						else
						{
							nana::detail::platform_scope_guard psg;
							status = XLookupBoth;
							len = ::XLookupString(&xevent.xkey, keybuf, 32, &keysym, 0);
						}

						keybuf[len] = 0;
						nana::char_t keychar;
						switch(status)
						{
						case XLookupKeySym:
						case XLookupBoth:
							switch(keysym)
							{
							case XK_Alt_L: case XK_Alt_R:
								keychar = keyboard::alt; break;
							case XK_BackSpace:
								keychar = keyboard::backspace;	break;
							case XK_Tab:
								keychar = keyboard::tab;		break;
							case XK_Escape:
								keychar = keyboard::escape;		break;
							case XK_Return:
								keychar = keyboard::enter;		break;
							case XK_Cancel:
								keychar = keyboard::copy;		break;	//Ctrl+C
							case XK_Page_Up:
								keychar = keyboard::os_pageup;	break;
							case XK_Page_Down:
								keychar = keyboard::os_pagedown; break;
							case XK_Left: case XK_Up: case XK_Right: case XK_Down:
								keychar = keyboard::os_arrow_left + (keysym - XK_Left); break;
							case XK_Insert:
								keychar = keyboard::os_insert; break;
							case XK_Delete:
								keychar = keyboard::os_del; break;
							default:
								keychar = 0xFF;
							}
							context.platform.keychar = keychar;
							if(keychar == keyboard::tab && (false == (msgwnd->flags.tab & detail::tab_type::eating))) //Tab
							{
								auto the_next = bedrock.wd_manager.tabstop_next(msgwnd);
								if(the_next)
								{
									bedrock.wd_manager.set_focus(the_next);
									bedrock.wd_manager.do_lazy_refresh(the_next, true);
									root_runtime->condition.tabstop_focus_changed = true;
								}
							}
							else if(keychar != 0xFF)
							{
								ei.keyboard.key = keychar;
								bedrock.get_key_state(ei.keyboard);
								bedrock.raise_event(event_tag::key_down, msgwnd, ei, true);
							}

							if(XLookupKeySym == status)
							{
								bedrock.wd_manager.do_lazy_refresh(msgwnd, false);
								break;
							}
						case XLookupChars:
							{
								const nana::char_t * charbuf;
#if defined(NANA_UNICODE)
								nana::detail::charset_conv charset("UTF-32", "UTF-8");
								const std::string& str = charset.charset(std::string(keybuf, keybuf + len));
								charbuf = reinterpret_cast<const nana::char_t*>(str.c_str()) + 1;
								len = str.size() / sizeof(wchar_t) - 1;
#else
								charbuf = keybuf;
#endif
								for(int i = 0; i < len; ++i)
								{
									ei.keyboard.key = charbuf[i];
									bedrock.get_key_state(ei.keyboard);
									ei.keyboard.ignore = false;

									ei.identifier = event_tag::key_char;
									ei.window = reinterpret_cast<window>(msgwnd);

									bedrock.evt_manager.answer(event_tag::key_char, reinterpret_cast<window>(msgwnd), ei, event_manager::event_kind::user);
									if(ei.keyboard.ignore == false && bedrock.wd_manager.available(msgwnd))
										bedrock.fire_event_for_drawer(event_tag::key_char, msgwnd, ei, &context);
								}
							}
							break;
						}
						bedrock.wd_manager.do_lazy_refresh(msgwnd, false);
						if(keybuf != fixbuf)
							delete [] keybuf;
					}
				}
				break;
			case KeyRelease:
				nana::detail::platform_spec::instance().write_keystate(xevent.xkey);
				if(context.platform.keychar != keyboard::alt) //Must NOT be an ALT
				{
					msgwnd = bedrock.focus();
					if(msgwnd)
					{
						ei.keyboard.key = static_cast<nana::char_t>(context.platform.keychar);
						bedrock.get_key_state(ei.keyboard);
						bedrock.raise_event(event_tag::key_up, msgwnd, ei, true);
					}
				}
				else
					bedrock.set_keyboard_shortkey(false);
				break;
			default:
				if(message == ClientMessage)
				{
					auto & atoms = nana::detail::platform_spec::instance().atombase();
					if(atoms.wm_protocols == xevent.xclient.message_type)
					{
						if(msgwnd->flags.enabled && (atoms.wm_delete_window == static_cast<Atom>(xevent.xclient.data.l[0])))
						{
							ei.unload.cancel = false;
							bedrock.raise_event(event_tag::unload, msgwnd, ei, true);
							if(false == ei.unload.cancel)
								native_interface::close_window(native_window);
						}
					}
				}
			}

			root_runtime = bedrock.wd_manager.root_runtime(native_window);
			if(root_runtime)
			{
				context.event_window = pre_event_window;
				root_runtime->condition.mouse_window = mouse_window;
				root_runtime->condition.mousemove_window = mousemove_window;
			}
			else
			{
				auto context = bedrock.get_thread_context();
				if(context) context->event_window = pre_event_window;
			}

			if(msgwnd)
			{
				unsigned tid = nana::system::this_thread_id();

				bedrock.wd_manager.remove_trash_handle(tid);
				bedrock.evt_manager.remove_trash_handle(tid);
			}
		}
	}

	void bedrock::pump_event(window modal_window)
	{
		thread_context * context = open_thread_context();
		if(0 == context->window_count)
		{
			//test if there is not a window
			remove_thread_context();
			return;
		}

		++(context->event_pump_ref_count);
		wd_manager.internal_lock().revert();
		
		native_window_type owner_native = 0;
		core_window_t * owner = 0;
		if(modal_window)
		{
			native_window_type modal = root(reinterpret_cast<core_window_t*>(modal_window));
			owner_native = native_interface::get_owner_window(modal);
			if(owner_native)
			{
				native_interface::enable_window(owner_native, false);
				owner = wd_manager.root(owner_native);
				if(owner)
					owner->flags.enabled = false;
			}	
		}
		
		nana::detail::platform_spec::instance().msg_dispatch(modal_window ? reinterpret_cast<core_window_t*>(modal_window)->root : 0);

		if(owner_native)
		{
			if(owner)
				owner->flags.enabled = true;
			native_interface::enable_window(owner_native, true);
		}
		
		wd_manager.internal_lock().forward();
		if(0 == --(context->event_pump_ref_count))
		{
			if(0 == modal_window || 0 == context->window_count)
				remove_thread_context();
		}

	}//end bedrock::event_loop

	void make_eventinfo_for_mouse(eventinfo& ei, detail::bedrock::core_window_t* wd, unsigned int msg, const event_mask& lparam)
	{
		ei.window = reinterpret_cast<window>(wd);
		ei.mouse.x = lparam.pos.x - wd->pos_root.x;
		ei.mouse.y = lparam.pos.y - wd->pos_root.y;
	}


	bool bedrock::fire_event_for_drawer(unsigned event_id, core_window_t* wd, const eventinfo& ei, thread_context* thrd)
	{
		if(bedrock_object.wd_manager.available(wd) == false) return false;
		if(thrd) thrd->event_window = wd;

		if(wd->other.upd_state == core_window_t::update_state::none)
			wd->other.upd_state = core_window_t::update_state::lazy;
		return bedrock_object.evt_manager.answer(event_id, reinterpret_cast<window>(wd), ei, event_manager::event_kind::trigger);
	}

	bool bedrock::fire_event(unsigned event_id, core_window_t* wd, const eventinfo& ei)
	{
		if(bedrock_object.wd_manager.available(wd) == false) return false;
		return bedrock_object.evt_manager.answer(event_id, reinterpret_cast<window>(wd), ei, event_manager::event_kind::user);
	}

	bool bedrock::raise_event(unsigned eid, core_window_t* wd, const eventinfo& ei, bool ask_update)
	{
		if(bedrock_object.wd_manager.available(wd) == false) return false;
		ei.identifier = eid;
		ei.window = reinterpret_cast<window>(wd);

		thread_context * thrd = bedrock_object.get_thread_context();
		core_window_t * prev_wd;
		if(thrd)
		{
			prev_wd = thrd->event_window;
			thrd->event_window = wd;
			bedrock_object._m_event_filter(eid, wd, thrd);
		}

		if(wd->other.upd_state == core_window_t::update_state::none)
			wd->other.upd_state = core_window_t::update_state::lazy;

		bedrock_object.evt_manager.answer(eid, reinterpret_cast<window>(wd), ei, event_manager::event_kind::trigger);
		bedrock_object.evt_manager.answer(eid, reinterpret_cast<window>(wd), ei, event_manager::event_kind::user);

		if(ask_update)
			bedrock_object.wd_manager.do_lazy_refresh(wd, false);
		else
			wd->other.upd_state = core_window_t::update_state::none;

		if(thrd) thrd->event_window = prev_wd;
		return true;
	}

	void bedrock::event_expose(core_window_t * wd, bool exposed)
	{
		if(wd)
		{
			eventinfo ei;
			ei.exposed = exposed;
			wd->visible = exposed;
			if(raise_event(event_tag::expose, wd, ei, false))
			{
				if(false == exposed)
				{
					if(wd->other.category != category::root_tag::value)
					{
						wd = wd->parent;

						while(wd->other.category == category::lite_widget_tag::value)
							wd = wd->parent;
					}
					else if(wd->other.category == category::frame_tag::value)
						wd = wd_manager.find_window(wd->root, wd->pos_root.x, wd->pos_root.y);
				}

				wd_manager.update(wd, true, true);
			}
		}
	}

	void bedrock::event_move(core_window_t * wd, int x, int y)
	{
		if(wd)
		{
			eventinfo ei;
			ei.move.x = x;
			ei.move.y = y;
			if(raise_event(event_tag::move, wd, ei, false))
				wd_manager.update(wd, true, true);
		}
	}

	void bedrock::thread_context_destroy(core_window_t * wd)
	{
		bedrock::thread_context * thr = get_thread_context(0);
		if(thr && thr->event_window == wd)
			thr->event_window = nullptr;
	}

	void bedrock::thread_context_lazy_refresh()
	{
		thread_context* thrd = get_thread_context(0);
		if(thrd && thrd->event_window)
		{
			//the state none should be tested, becuase in an event, there would be draw after an update,
			//if the none is not tested, the draw after update will not be refreshed.
			switch(thrd->event_window->other.upd_state)
			{
			case core_window_t::update_state::none:
			case core_window_t::update_state::lazy:
				thrd->event_window->other.upd_state = core_window_t::update_state::refresh;
			default:	break;
			}
		}
	}

	void bedrock::update_cursor(core_window_t * wd)
	{
		internal_scope_guard isg;
		if(wd_manager.available(wd))
		{
			thread_context * thrd = get_thread_context(wd->thread_id);
			if(thrd)
			{
				Display * disp = nana::detail::platform_spec::instance().open_display();

				if((wd->predef_cursor == cursor::arrow) && (thrd->cursor.window == wd))
				{
					if(thrd->cursor.handle)
					{
						::XUndefineCursor(disp, reinterpret_cast<Window>(wd->root));
						::XFreeCursor(disp, thrd->cursor.handle);
						thrd->cursor.window = nullptr;
						thrd->cursor.predef_cursor = cursor::arrow;
						thrd->cursor.handle = 0;
					}
					return;
				}

				auto pos = native_interface::cursor_position();
				auto native_handle = native_interface::find_window(pos.x, pos.y);
				if(0 == native_handle)
					return;
				
				native_interface::calc_window_point(native_handle, pos);
				if(wd != wd_manager.find_window(native_handle, pos.x, pos.y))
					return;
				
				if(wd->predef_cursor != thrd->cursor.predef_cursor)
				{
					if(thrd->cursor.handle)
					{
						::XFreeCursor(disp, thrd->cursor.handle);
						thrd->cursor.handle = 0;
						thrd->cursor.window = nullptr;
					}

					if(wd->predef_cursor != cursor::arrow)
					{
						thrd->cursor.handle = ::XCreateFontCursor(disp, static_cast<unsigned>(wd->predef_cursor));
						thrd->cursor.window = wd;
						::XDefineCursor(disp, reinterpret_cast<Window>(wd->root), thrd->cursor.handle);
					}
					thrd->cursor.predef_cursor = wd->predef_cursor;
				}
			}
		}
	}

	void bedrock::_m_event_filter(unsigned event_id, core_window_t * wd, thread_context * thrd)
	{
		auto & spec = nana::detail::platform_spec::instance();
		Display * disp = spec.open_display();
		switch(event_id)
		{
		case events::mouse_enter::identifier:
			if(wd->predef_cursor != cursor::arrow)
			{
				thrd->cursor.window = wd;
				if(wd->predef_cursor != thrd->cursor.predef_cursor)
				{
					if(thrd->cursor.handle)
						::XFreeCursor(disp, thrd->cursor.handle);
					thrd->cursor.handle = ::XCreateFontCursor(disp, static_cast<unsigned>(wd->predef_cursor));
					thrd->cursor.predef_cursor = wd->predef_cursor;
				}
				::XDefineCursor(disp, reinterpret_cast<Window>(wd->root), thrd->cursor.handle);
			}
			break;
		case events::mouse_leave::identifier:
			if(wd->predef_cursor != cursor::arrow)
				::XUndefineCursor(disp, reinterpret_cast<Window>(wd->root));
			break;
		case events::destroy::identifier:
			if(thrd->cursor.handle && (wd == thrd->cursor.window))
			{
				::XUndefineCursor(disp, reinterpret_cast<Window>(wd->root));
				::XFreeCursor(disp, thrd->cursor.handle);
				thrd->cursor.handle = 0;
				thrd->cursor.predef_cursor = cursor::arrow;
				thrd->cursor.window = 0;
			}
			break;
		}
	}
}//end namespace detail
}//end namespace gui
}//end namespace nana
