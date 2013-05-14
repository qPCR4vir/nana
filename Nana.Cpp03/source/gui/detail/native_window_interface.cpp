/*
 *	Platform Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/native_window_interface.cpp
 */

#include <nana/config.hpp>
#include PLATFORM_SPEC_HPP
#include <nana/gui/detail/native_window_interface.hpp>
#if defined(NANA_WINDOWS)
	#include <nana/paint/detail/image_ico.hpp>
	#include <nana/threads/mutex.hpp>
	#include <map>
#elif defined(NANA_X11)
	#include <nana/system/platform.hpp>
	#include GUI_BEDROCK_HPP
#endif

namespace nana{
	namespace paint
	{
		class image_accessor
		{
		public:
#if defined(NANA_WINDOWS)
			static HICON icon(const nana::paint::image& img)
			{
				image::image_impl_interface * impl = (img.ref_image_.empty() ? 0 : img.ref_image_.handle());
				if(impl)
				{
					paint::detail::image_ico * ico = dynamic_cast<paint::detail::image_ico*>(impl);
					if(ico)
					{
						if(false == ico->ref().empty())
							return ico->ref().handle();
					}
				}
				return 0;
			}
#endif
		};
	}

	namespace gui{	namespace detail{

#if defined(NANA_WINDOWS)
	class tray_manager
	{
		struct window_extra_t
		{
			HICON ico;

			window_extra_t()
				: ico(0)
			{}
		};

		typedef std::map<native_window_type, window_extra_t> map_t;

	private:
		tray_manager(){}
	public:
		typedef window_extra_t extra_t;

		static tray_manager& instance()
		{
			static tray_manager object;
			return object;
		}

		bool remove(native_window_type wd, extra_t & ext)
		{
			threads::lock_guard<threads::recursive_mutex> lock(mutex_);

			map_t::iterator i = map_.find(wd);
			if(i != map_.end())
			{
				ext = i->second;
				map_.erase(i);
				return true;
			}
			return false;
		}

		HICON set_icon(native_window_type wd, HICON ico)
		{
			threads::lock_guard<threads::recursive_mutex> lock(mutex_);

			map_t::iterator i = map_.find(wd);
			if(i != map_.end())
			{
				HICON ret = i->second.ico;
				i->second.ico = ico;
				return ret;
			}

			map_[wd].ico = ico;
			return 0;
		}
	private:
		threads::recursive_mutex mutex_;
		map_t map_;
	};
#elif defined(NANA_X11)
	namespace restrict
	{
		nana::detail::platform_spec & spec = nana::detail::platform_spec::instance();
	}
#endif

	//struct native_interface
		nana::size native_interface::screen_size()
		{
#if defined(NANA_WINDOWS)
			return nana::size(::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			Screen* s = ::XScreenOfDisplay(restrict::spec.open_display(), ::XDefaultScreen(restrict::spec.open_display()));
			return nana::size(::XWidthOfScreen(s), ::XHeightOfScreen(s));
#endif
		}

		rectangle native_interface::screen_area_from_point(const point& pos)
		{
#if defined(NANA_WINDOWS)
			POINT native_pos = {pos.x, pos.y};
			HMONITOR monitor = ::MonitorFromPoint(native_pos, 2 /*MONITOR_DEFAULTTONEAREST*/);

			MONITORINFO mi;
			mi.cbSize = sizeof mi;
			if(::GetMonitorInfo(monitor, &mi))
			{
				return rectangle(mi.rcWork.left, mi.rcWork.top,
								mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top);
			}

#elif defined(NANA_X11)
#endif
			return screen_size();
		}

		//platform-dependent
		native_interface::window_result native_interface::create_window(native_window_type owner, bool nested, const rectangle& r, const appearance& app)
		{
#if defined(NANA_WINDOWS)
			DWORD style = WS_SYSMENU | WS_CLIPCHILDREN;
			DWORD style_ex= WS_EX_NOPARENTNOTIFY;

			if(app.minimize)	style |= WS_MINIMIZEBOX;
			if(app.maximize)	style |= WS_MAXIMIZEBOX;

			if(app.sizable)	style |= WS_THICKFRAME;

			if(app.decoration)
				style |= WS_OVERLAPPED | WS_CAPTION;

			style |= (nested ? WS_CHILD : WS_POPUP);
			style_ex |= (app.taskbar ? WS_EX_APPWINDOW : WS_EX_TOOLWINDOW);

			if(app.floating)	style_ex |= WS_EX_TOPMOST;

			POINT pt = {r.x, r.y};

			if(owner && (nested == false))
				::ClientToScreen(reinterpret_cast<HWND>(owner), &pt);

			HWND wnd = ::CreateWindowEx(style_ex, STR("NanaWindowInternal"), STR("Nana Window"),
											style, pt.x, pt.y, 100, 100,
											reinterpret_cast<HWND>(owner), 0, ::GetModuleHandle(0), 0);

			//A window may have a border, this should be adjusted the client area fit for the specified size.
			::RECT client;
			::GetClientRect(wnd, &client);	//The right and bottom of client by GetClientRect indicate the width and height of the area
			::RECT wd_area;
			::GetWindowRect(wnd, &wd_area);
			wd_area.right -= wd_area.left;
			wd_area.bottom -= wd_area.top;
			if(nested)
			{
				wd_area.left = pt.x;
				wd_area.top = pt.y;
			}

			int delta_w = static_cast<int>(r.width) - client.right;
			int delta_h = static_cast<int>(r.height) - client.bottom;

			::MoveWindow(wnd, wd_area.left, wd_area.top, wd_area.right + delta_w, wd_area.bottom + delta_h, true);

			::GetClientRect(wnd, &client);
			::GetWindowRect(wnd, &wd_area);
			wd_area.right -= wd_area.left;
			wd_area.bottom -= wd_area.top;

			window_result result = {reinterpret_cast<native_window_type>(wnd),
										static_cast<unsigned>(client.right), static_cast<unsigned>(client.bottom),
										static_cast<unsigned>(wd_area.right - client.right), static_cast<unsigned>(wd_area.bottom - client.bottom)};
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;

			XSetWindowAttributes win_attr;
			unsigned long attr_mask =	CWBackPixmap | CWBackPixel | CWBorderPixel |
										CWWinGravity | CWBitGravity | CWColormap | CWEventMask;

			Display * disp = restrict::spec.open_display();
			win_attr.colormap = restrict::spec.colormap();

			win_attr.background_pixmap = None;
			win_attr.background_pixel = 0xFFFFFF;
			win_attr.border_pixmap = None;
			win_attr.border_pixel = 0x0;
			win_attr.bit_gravity = 0;
			win_attr.win_gravity = NorthWestGravity;
			win_attr.backing_store = 0;
			win_attr.backing_planes = 0;
			win_attr.backing_pixel = 0;
			win_attr.colormap = restrict::spec.colormap();

			if(app.decoration == false)
			{
				win_attr.override_redirect = True;
				attr_mask |= CWOverrideRedirect;
			}

			Window parent = (owner ? reinterpret_cast<Window>(owner) : restrict::spec.root_window());
			nana::point pos(r.x, r.y);
			if((false == nested) && owner)
			{
				win_attr.save_under = True;
				attr_mask |= CWSaveUnder;
				parent = restrict::spec.root_window();
				calc_screen_point(owner, pos);
			}

			win_attr.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask;

			Window handle = ::XCreateWindow(disp, parent,
							pos.x, pos.y, (r.width ? r.width : 1), (r.height ? r.height : 1), 0,
							restrict::spec.screen_depth(), InputOutput, restrict::spec.screen_visual(),
							attr_mask, &win_attr);
			if(handle)
			{
				if(owner)
					restrict::spec.make_owner(owner, reinterpret_cast<native_window_type>(handle));

				XTextProperty name;
				char text[] = "Nana Window";
				char * str = text;
				::XStringListToTextProperty(&str, 1, &name);
				::XSetWMName(disp, handle, &name);

				const nana::detail::atombase_tag & ab = restrict::spec.atombase();
				::XSetWMProtocols(disp, handle, const_cast<Atom*>(&ab.wm_delete_window), 1);

				struct
				{
					long flags;
					long functions;
					long decorations;
					long input;
					long status;
				}motif;
				//MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;// | MWM_HINTS_INPUT_MODE;
				motif.flags = 1 | 2; //| 4;
				motif.functions = 4;//MWM_FUNC_MOVE;
				motif.decorations = 0;
				motif.input = 0;//MWM_INPUT_MODELESS;
				motif.status = 0;

				XSizeHints hints;
				hints.flags = USPosition;
				hints.x = pos.x;
				hints.y = pos.y;

				if(app.sizable)
				{
					motif.decorations |= 4; //MWM_DECOR_RESIZEH;
					motif.functions |= 2; //MWM_FUNC_RESIZE;
				}
				else
				{
					hints.min_width = hints.max_width = r.width;
					hints.min_height = hints.max_height = r.height;
					hints.flags |= (PMinSize | PMaxSize);
				}
				::XSetWMNormalHints(disp, handle, &hints);

				if(app.decoration)
				{
					if(app.minimize)
					{
						motif.decorations |= (1 << 5);	//MWM_DECOR_MINIMIZE;
						motif.functions |= (1 << 3);	//MWM_FUNC_MINIMIZE;
					}

					if(app.maximize)
					{
						motif.decorations |= (1 << 6);//MWM_DECOR_MAXIMIZE;
						motif.functions |= ( 1 << 4);//MWM_FUNC_MAXIMIZE;
					}
					motif.functions |= (1<<5); //MWM_FUNC_CLOSE
					motif.decorations |= (2) | 8; //MWM_DECOR_BORDER | MWM_DECOR_TITLE
				}

				if((false == nested) && owner)
				{
					::XChangeProperty(disp, handle, ab.net_wm_window_type, XA_ATOM, 32, PropModeReplace,
							reinterpret_cast<unsigned char*>(const_cast<Atom*>(&ab.net_wm_window_type_dialog)), 1);
					::XSetTransientForHint(disp, handle, reinterpret_cast<Window>(owner));
				}

				::XChangeProperty(disp, handle, ab.motif_wm_hints, ab.motif_wm_hints, 32, PropModeReplace,
									reinterpret_cast<unsigned char*>(&motif), sizeof(motif)/sizeof(long));

				if(app.floating)
				{
					::XChangeProperty(disp, handle, ab.net_wm_window_type, XA_ATOM, 32, PropModeReplace,
								reinterpret_cast<unsigned char*>(const_cast<Atom*>(&ab.net_wm_window_type_normal)), 1);
					::XSetTransientForHint(disp, handle, restrict::spec.root_window());
				}

				if(false == app.taskbar)
				{
					::XChangeProperty(disp, handle, ab.net_wm_state, XA_ATOM, 32, PropModeAppend,
										reinterpret_cast<unsigned char*>(const_cast<Atom*>(&ab.net_wm_state_skip_taskbar)), 1);
				}
			}
			window_result result = {reinterpret_cast<native_window_type>(handle), r.width, r.height, 0, 0};
			restrict::spec.msg_insert(reinterpret_cast<native_window_type>(handle));
#endif
			return result;
		}

		native_window_type native_interface::create_child_window(native_window_type parent, const rectangle& r)
		{
			if(0 == parent) return 0;
#if defined(NANA_WINDOWS)
			HWND handle = ::CreateWindowEx(WS_EX_CONTROLPARENT,		// Extended possibilites for variation
										STR("NanaWindowInternal"),
										STR("Nana Child Window"),	// Title Text
										WS_CHILD | WS_VISIBLE | WS_TABSTOP  | WS_CLIPSIBLINGS,
										r.x, r.y, r.width, r.height,
										reinterpret_cast<HWND>(parent),	// The window is a child-window to desktop
										0, ::GetModuleHandle(0), 0);
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;

			XSetWindowAttributes win_attr;
			unsigned long attr_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
							CWWinGravity | CWBitGravity | CWColormap | CWEventMask;

			Display * disp = restrict::spec.open_display();
			win_attr.colormap = restrict::spec.colormap();

			win_attr.background_pixmap = None;
			win_attr.background_pixel = 0xFFFFFF;
			win_attr.border_pixmap = None;
			win_attr.border_pixel = 0x0;
			win_attr.bit_gravity = 0;
			win_attr.win_gravity = NorthWestGravity;
			win_attr.backing_store = 0;
			win_attr.backing_planes = 0;
			win_attr.backing_pixel = 0;
			win_attr.colormap = restrict::spec.colormap();

			win_attr.override_redirect = True;
			attr_mask |= CWOverrideRedirect;

			nana::point pos(r.x, r.y);
			win_attr.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask;

			Window handle = ::XCreateWindow(disp, reinterpret_cast<Window>(parent),
							pos.x, pos.y, (r.width ? r.width : 1), (r.height ? r.height : 1), 0,
							restrict::spec.screen_depth(), InputOutput, restrict::spec.screen_visual(),
							attr_mask, &win_attr);

			if(handle)
			{
				restrict::spec.make_owner(parent, reinterpret_cast<native_window_type>(handle));

				XTextProperty name;
				char text[] = "Nana Child Window";
				char * str = text;
				::XStringListToTextProperty(&str, 1, &name);
				::XSetWMName(disp, handle, &name);

				const nana::detail::atombase_tag & ab = restrict::spec.atombase();
				::XSetWMProtocols(disp, handle, const_cast<Atom*>(&ab.wm_delete_window), 1);

				struct
				{
					long flags;
					long functions;
					long decorations;
					long input;
					long status;
				}motif;
				//MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;// | MWM_HINTS_INPUT_MODE;
				motif.flags = 1 | 2; //| 4;
				motif.functions = 4;//MWM_FUNC_MOVE;
				motif.decorations = 0;
				motif.input = 0;//MWM_INPUT_MODELESS;
				motif.status = 0;

				XSizeHints hints;
				hints.flags = USPosition;
				hints.x = pos.x;
				hints.y = pos.y;
				hints.min_width = hints.max_width = r.width;
				hints.min_height = hints.max_height = r.height;
				hints.flags |= (PMinSize | PMaxSize);
				::XSetWMNormalHints(disp, handle, &hints);

				::XChangeProperty(disp, handle, ab.motif_wm_hints, ab.motif_wm_hints, 32, PropModeReplace,
									reinterpret_cast<unsigned char*>(&motif), sizeof(motif)/sizeof(long));

				::XChangeProperty(disp, handle, ab.net_wm_state, XA_ATOM, 32, PropModeAppend,
						reinterpret_cast<unsigned char*>(const_cast<Atom*>(&ab.net_wm_state_skip_taskbar)), 1);
			}
#endif
			return reinterpret_cast<native_window_type>(handle);
		}

#if defined(NANA_X11)
		void native_interface::set_modal(native_window_type wd)
		{
			Window owner = reinterpret_cast<Window>(restrict::spec.get_owner(wd));
			if(wd && owner)
			{
				if(is_window_visible(wd))
					show_window(wd, false, true);
				Display* disp = restrict::spec.open_display();
				const nana::detail::atombase_tag  & atombase = restrict::spec.atombase();
				::XSetTransientForHint(disp, reinterpret_cast<Window>(wd), owner);
				::XChangeProperty(disp, reinterpret_cast<Window>(wd),
								atombase.net_wm_state, XA_ATOM, sizeof(int) * 8,
								PropModeReplace,
								reinterpret_cast<const unsigned char*>(&atombase.net_wm_state_modal), 1);
			}
		}
#endif

		void native_interface::enable_window(native_window_type wd, bool is_enabled)
		{
#if defined(NANA_WINDOWS)
			::EnableWindow(reinterpret_cast<HWND>(wd), is_enabled);
#else
			int mask = ExposureMask | StructureNotifyMask;
			if(is_enabled)
			{
				mask |= (ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
				mask |= (KeyPressMask | KeyReleaseMask);
				mask |= (EnterWindowMask | LeaveWindowMask | FocusChangeMask);
			}

			::XSelectInput(restrict::spec.open_display(), reinterpret_cast<Window>(wd), mask);
#endif
		}

		bool native_interface::window_icon(native_window_type wd, const nana::paint::image& img)
		{
#if defined(NANA_WINDOWS)
			HICON ico = paint::image_accessor::icon(img);
			if(ico)
			{
				nana::detail::platform_spec::instance().keep_window_icon(wd, img);
				::SendMessage(reinterpret_cast<HWND>(wd), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(ico));
				::SendMessage(reinterpret_cast<HWND>(wd), WM_SETICON, ICON_SMALL, reinterpret_cast<WPARAM>(ico));
				return true;
			}
#elif defined(NANA_X11)
			if(wd && (false == img.empty()))
			{

				const nana::paint::graphics & graph = restrict::spec.keep_window_icon(wd, img);
				XWMHints hints;
				hints.flags = IconPixmapHint;
				hints.icon_pixmap = graph.handle()->pixmap;

				nana::detail::platform_scope_guard psg;
				::XSetWMHints(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &hints);
				return true;
			}
#endif
			return false;
		}

		void native_interface::active_owner(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
			if(owner)
			{
				if(::GetWindowThreadProcessId(owner, 0) == ::GetCurrentThreadId())
				{
					::EnableWindow(owner, true);
					::SetActiveWindow(owner);
					::SetForegroundWindow(owner);
				}
				else
					::PostMessage(owner, nana::detail::messages::async_active_owner, 0, 0);
			}
#elif defined(NANA_X11)

#endif
		}

		//close_window
		//@brief:Destroy a window
		void native_interface::close_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			if(wd && (::DestroyWindow(reinterpret_cast<HWND>(wd)) == false))
			{
				//DestroyWindow would be failed if the calling thread is not the window thread
				//It should send a WM_DESTROY message into window thread for destroying window
				if(::GetLastError() == ERROR_ACCESS_DENIED)
					::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_destroy_window, 0, 0);
			}
#elif defined(NANA_X11)
			//Under X, XDestroyWindow destroys the specified window and generats a DestroyNotify
			//event, when the client receives the event, the specified window has been already
			//destroyed. This is a feature which is different from Windows. So the following
			//works should be handled before calling XDestroyWindow.
			nana::gui::detail::bedrock & bedrock = nana::gui::detail::bedrock::instance();
			if(wd == bedrock.get_menu())
				bedrock.empty_menu();

			Display * disp = restrict::spec.open_display();
			restrict::spec.remove(wd);
			nana::gui::detail::bedrock::core_window_t * iwd = bedrock.wd_manager.root(wd);
			if(iwd)
			{
				//Before calling window_manager::destroy, make sure the window is invisible.
				//It is a behavior like Windows.
				{
					nana::detail::platform_scope_guard psg;
					restrict::spec.set_error_handler();
					::XUnmapWindow(disp, reinterpret_cast<Window>(wd));
					::XFlush(disp);
					restrict::spec.rev_error_handler();
				}
				bedrock.wd_manager.destroy(iwd);
				bedrock.evt_manager.umake(reinterpret_cast<gui::window>(iwd), false);
				bedrock.rt_manager.remove_if_exists(iwd);
				bedrock.wd_manager.destroy_handle(iwd);
			}

			nana::detail::platform_scope_guard psg;
			restrict::spec.set_error_handler();
			::XDestroyWindow(disp, reinterpret_cast<Window>(wd));
			restrict::spec.rev_error_handler();
#endif
		}

		void native_interface::show_window(native_window_type wd, bool show, bool active)
		{
#if defined(NANA_WINDOWS)
			int cmd = (show ? (active ? SW_SHOW : SW_SHOWNA) : SW_HIDE);
			bool async = true;
			const DWORD tid = ::GetCurrentThreadId();
			if(tid == ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
			{
				HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
				if((0 == owner) || (tid == ::GetWindowThreadProcessId(owner, 0)))
				{
					async = false;
					HWND owned = ::GetWindow(reinterpret_cast<HWND>(wd), GW_HWNDPREV);
					while(owned)
					{
						if(::GetWindow(owned, GW_OWNER) == reinterpret_cast<HWND>(wd))
						{
							if(tid != ::GetWindowThreadProcessId(owned, 0))
							{
								async = true;
								break;
							}
						}
						owned = ::GetWindow(owned, GW_HWNDPREV);
					}
				}
			}
			if(async)
				::ShowWindowAsync(reinterpret_cast<HWND>(wd), cmd);
			else
				::ShowWindow(reinterpret_cast<HWND>(wd), cmd);
#elif defined(NANA_X11)
			if(wd)
			{
				nana::detail::platform_scope_guard psg;
				Display* disp = restrict::spec.open_display();
				if(show)
				{
					::XMapWindow(disp, reinterpret_cast<Window>(wd));
					Window grab = restrict::spec.grab(0);
					if(grab == reinterpret_cast<Window>(wd))
						capture_window(wd, true);
				}
				else
					::XUnmapWindow(disp, reinterpret_cast<Window>(wd));

				::XFlush(disp);
			}
#endif
		}

		void native_interface::restore_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			::ShowWindowAsync(reinterpret_cast<HWND>(wd), SW_RESTORE);
#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();
			const nana::detail::atombase_tag & atombase = restrict::spec.atombase();
			XEvent evt;
			evt.xclient.type = ClientMessage;
			evt.xclient.display = restrict::spec.open_display();
			evt.xclient.message_type = atombase.net_wm_state;
			evt.xclient.format = 32;
			evt.xclient.window = reinterpret_cast<Window>(wd);
			evt.xclient.data.l[0] = 0;
			evt.xclient.data.l[1] = atombase.net_wm_state_maximized_horz;
			evt.xclient.data.l[2] = atombase.net_wm_state_maximized_vert;
			evt.xclient.data.l[3] = evt.xclient.data.l[4] = 0;

			nana::detail::platform_scope_guard psg;
			::XSendEvent(disp, XDefaultRootWindow(disp), False, SubstructureRedirectMask | SubstructureNotifyMask, &evt);
			evt.xclient.data.l[1] = atombase.net_wm_state_fullscreen;
			evt.xclient.data.l[2] = evt.xclient.data.l[3] = evt.xclient.data.l[4] = 0;
			::XSendEvent(disp, XDefaultRootWindow(disp), False, SubstructureRedirectMask | SubstructureNotifyMask, &evt);
			::XMapWindow(disp, reinterpret_cast<Window>(wd));
#endif
		}

		void native_interface::refresh_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			RECT rect;
			::GetClientRect(reinterpret_cast<HWND>(wd), &rect);
			::InvalidateRect(reinterpret_cast<HWND>(wd), &rect, true);
#elif defined(NANA_X11)
#endif
		}

		bool native_interface::is_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			return (FALSE != ::IsWindow(reinterpret_cast<HWND>(wd)));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			XWindowAttributes attr;
			restrict::spec.set_error_handler();
			::XGetWindowAttributes(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &attr);
			return (BadWindow != restrict::spec.rev_error_handler());
#endif
		}

		bool native_interface::is_window_visible(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			return (FALSE != ::IsWindowVisible(reinterpret_cast<HWND>(wd)));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			XWindowAttributes attr;
			restrict::spec.set_error_handler();
			::XGetWindowAttributes(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &attr);
			return (BadWindow != restrict::spec.rev_error_handler() &&
					attr.map_state != IsUnmapped);
#endif
		}

		bool native_interface::is_window_zoomed(native_window_type wd, bool ask_for_max)
		{
#if defined(NANA_WINDOWS)
			return (FALSE != (ask_for_max ? ::IsZoomed(reinterpret_cast<HWND>(wd)) : ::IsIconic(reinterpret_cast<HWND>(wd))));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			bool zoomed = false;
    		unsigned long n,i; Atom type; unsigned char *prop; int format;
			if(Success== ::XGetWindowProperty(restrict::spec.open_display(), reinterpret_cast<Window>(wd), restrict::spec.atombase().net_wm_state, 0, 2, false, AnyPropertyType, &type, &format, &n, &i, &prop))
			{
				if(32 == format)
				{
					if(ask_for_max)
					{
						if(type == XA_ATOM)
						{
							for(i=0; i<n; i++)
							{
								if(reinterpret_cast<Atom*>(prop)[i] == restrict::spec.atombase().net_wm_state_fullscreen)
								{
									zoomed = true;
									break;
								}
          					}
						}
					}
					else
						zoomed = (IconicState == *reinterpret_cast<unsigned*>(prop));
				}
				XFree(prop);
			}
			return zoomed;
#endif
		}

		nana::point native_interface::window_position(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			::RECT r;
			::GetWindowRect(reinterpret_cast<HWND>(wd), & r);
			HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
			if(owner)
			{
				::POINT pos = {r.left, r.top};
				::ScreenToClient(owner, &pos);
				return nana::point(pos.x, pos.y);
			}
			return nana::point(r.left, r.top);
#elif defined(NANA_X11)
			int x, y;
			nana::detail::platform_scope_guard psg;
			Window root = reinterpret_cast<Window>(restrict::spec.get_owner(wd));
			if(root == 0) root = restrict::spec.root_window();
			Window child;
			if(True == ::XTranslateCoordinates(restrict::spec.open_display(), reinterpret_cast<Window>(wd), root, 0, 0, &x, &y, &child))
				return nana::point(x, y);
			return nana::point(0, 0);
#endif
		}

		void native_interface::move_window(native_window_type wd, int x, int y)
		{
#if defined(NANA_WINDOWS)
			if(::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0) != ::GetCurrentThreadId())
			{
				nana::detail::messages::move_window * mw = new nana::detail::messages::move_window;
				mw->x = x;
				mw->y = y;
				mw->ignore = mw->Size;
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_move_window, reinterpret_cast<WPARAM>(mw), 0);
			}
			else
			{
				::RECT r;
				::GetWindowRect(reinterpret_cast<HWND>(wd), &r);
				HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
				if(owner)
				{
					::RECT owner_rect;
					::GetWindowRect(owner, &owner_rect);
					::POINT pos = {owner_rect.left, owner_rect.top};
					::ScreenToClient(owner, &pos);
					x += (owner_rect.left - pos.x);
					y += (owner_rect.top - pos.y);
				}
				::MoveWindow(reinterpret_cast<HWND>(wd), x, y, r.right - r.left, r.bottom - r.top, true);
			}
#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();

			nana::detail::platform_scope_guard psg;
			Window owner = reinterpret_cast<Window>(restrict::spec.get_owner(wd));
			if(owner)
			{
				Window child;
				::XTranslateCoordinates(disp, owner, restrict::spec.root_window(),
										x, y, &x, &y, &child);
			}

			XWindowAttributes attr;
			::XGetWindowAttributes(disp, reinterpret_cast<Window>(wd), &attr);
			if(attr.map_state == IsUnmapped)
			{
				XSizeHints hints;
				hints.flags = USPosition;
				hints.x = x;
				hints.y = y;
				::XSetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints);
			}

			::XMoveWindow(disp, reinterpret_cast<Window>(wd), x, y);
#endif
		}

		void native_interface::move_window(native_window_type wd, int x, int y, unsigned width, unsigned height)
		{
#if defined(NANA_WINDOWS)
			if(::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0) != ::GetCurrentThreadId())
			{
				nana::detail::messages::move_window * mw = new nana::detail::messages::move_window;
				mw->x = x;
				mw->y = y;
				mw->width = width;
				mw->height = height;
				mw->ignore = 0;
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_move_window, reinterpret_cast<WPARAM>(mw), 0);
			}
			else
			{
				HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
				if(owner)
				{
					::RECT owner_rect;
					::GetWindowRect(owner, &owner_rect);
					::POINT pos = {owner_rect.left, owner_rect.top};
					::ScreenToClient(owner, &pos);
					x += (owner_rect.left - pos.x);
					y += (owner_rect.top - pos.y);
				}
				::MoveWindow(reinterpret_cast<HWND>(wd), x, y, width, height, true);
			}
#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();
			long supplied;
			XSizeHints hints;
			nana::detail::platform_scope_guard psg;

			::XGetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints, &supplied);
			if((hints.flags & (PMinSize | PMaxSize)) && (hints.min_width == hints.max_width) && (hints.min_height == hints.max_height))
			{
				hints.flags = PMinSize | PMaxSize;
				hints.min_width = hints.max_width = width;
				hints.min_height = hints.max_height = height;
			}
			else
				hints.flags = 0;

			Window owner = reinterpret_cast<Window>(restrict::spec.get_owner(wd));
			if(owner)
			{
				Window child;
				::XTranslateCoordinates(disp, owner, restrict::spec.root_window(),
										x, y, &x, &y, &child);
			}

			XWindowAttributes attr;
			::XGetWindowAttributes(disp, reinterpret_cast<Window>(wd), &attr);
			if(attr.map_state == IsUnmapped)
			{
				hints.flags |= (USPosition | USSize);
				hints.x = x;
				hints.y = y;
				hints.width = width;
				hints.height = height;
			}

			if(hints.flags)
				::XSetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints);

			::XMoveResizeWindow(disp, reinterpret_cast<Window>(wd), x, y, width, height);
#endif
		}

		void native_interface::set_window_z_order(native_window_type wd, native_window_type wd_after, z_order_action::t action_if_no_wd_after)
		{
#if defined(NANA_WINDOWS)
			HWND wa = reinterpret_cast<HWND>(wd_after);
			if(wa == 0)
			{
				switch(action_if_no_wd_after)
				{
				case z_order_action::bottom : wa = HWND_BOTTOM;	break;
				case z_order_action::top: wa = HWND_TOP;		break;
				case z_order_action::topmost: wa = HWND_TOPMOST;	break;
				case z_order_action::foreground:
					::SetForegroundWindow(reinterpret_cast<HWND>(wd));
					return;
				default:
					wa = HWND_NOTOPMOST;
				}
			}
			if(::GetCurrentThreadId() != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_set_window_pos, reinterpret_cast<WPARAM>(wa), SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
			else
				::SetWindowPos(reinterpret_cast<HWND>(wd), wa, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			Display * disp = restrict::spec.open_display();
			if(0 == wd_after)
			{
				switch(action_if_no_wd_after)
				{
				case z_order_action::bottom:
					::XLowerWindow(disp, reinterpret_cast<Window>(wd));
					break;
				case z_order_action::foreground:
				case z_order_action::top:
				case z_order_action::topmost:
					::XRaiseWindow(disp, reinterpret_cast<Window>(wd));
					break;
				default:	//z_order_action::none
					break;
				}
			}
			else
			{
				//If the handle wd is a top level, XConfigureWindow() will be failed with a BadMatch.
				//The fix is to use XReconfigureWMWindow() instead.

				XWindowChanges values;
				values.sibling = reinterpret_cast<Window>(wd_after);
				values.stack_mode = Below;
				::XReconfigureWMWindow(disp, reinterpret_cast<Window>(wd), ::XDefaultScreen(disp), CWSibling | CWStackMode, &values);
			}
#endif
		}

		void native_interface::window_size(native_window_type wd, unsigned width, unsigned height)
		{
#if defined(NANA_WINDOWS)
			if(::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0) != ::GetCurrentThreadId())
			{
				nana::detail::messages::move_window * mw = new nana::detail::messages::move_window;
				mw->width = width;
				mw->height = height;
				mw->ignore = mw->Pos;
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_move_window, reinterpret_cast<WPARAM>(mw), 0);
			}
			else
			{
				::RECT r;
				::GetWindowRect(reinterpret_cast<HWND>(wd), &r);
				HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
				HWND parent = ::GetParent(reinterpret_cast<HWND>(wd));
				if(parent && (parent != owner))
				{
					::POINT pos = {r.left, r.top};
					::ScreenToClient(parent, &pos);
					r.left = pos.x;
					r.top = pos.y;
				}
				::MoveWindow(reinterpret_cast<HWND>(wd), r.left, r.top, static_cast<int>(width), static_cast<int>(height), true);
			}
#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();
			nana::detail::platform_scope_guard psg;

			//Check the XSizeHints for testing whether the window is sizable.
			XSizeHints hints;
			long supplied;
			::XGetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints, &supplied);
			if((hints.flags & (PMinSize | PMaxSize)) && (hints.min_width == hints.max_width) && (hints.min_height == hints.max_height))
			{
				hints.flags = PMinSize | PMaxSize;
				hints.min_width = hints.max_width = width;
				hints.min_height = hints.max_height = height;
				::XSetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints);
			}
			::XResizeWindow(disp, reinterpret_cast<Window>(wd), width, height);
#endif
		}

		void native_interface::get_window_rect(native_window_type wd, rectangle& r)
		{
#if defined(NANA_WINDOWS)
			::RECT winr;
			::GetWindowRect(reinterpret_cast<HWND>(wd), &winr);
			r.x = winr.left;
			r.y = winr.top;
			r.width = winr.right - winr.left;
			r.height = winr.bottom - winr.top;
#elif defined(NANA_X11)
			Window root;
			int x, y;
			unsigned border, depth;
			nana::detail::platform_scope_guard psg;
			::XGetGeometry(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &root, &x, &y, &r.width, &r.height, &border, &depth);
#endif
		}

		void native_interface::window_caption(native_window_type wd, const nana::string& title)
		{
#if defined(NANA_WINDOWS)
			if(::GetCurrentThreadId() != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
			{
				wchar_t * wstr = 0;
#if defined(NANA_UNICODE)
				wstr = new wchar_t[title.length() + 1];
				wcscpy(wstr, title.c_str());
#else
				std::wstring str = nana::charset(title);
				wstr = new wchar_t[str.length() + 1];
				wcscpy(wstr, str.c_str());
#endif
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_set_window_text, reinterpret_cast<WPARAM>(wstr), 0);
			}
			else
				::SetWindowText(reinterpret_cast<HWND>(wd), title.c_str());
#elif defined(NANA_X11)
			::XTextProperty name;
	#if defined(NANA_UNICODE)
			std::string mbstr = nana::charset(title);
			char* text = const_cast<char*>(mbstr.c_str());
	#else
			char* text = const_cast<char*>(title.c_str());
	#endif
			nana::detail::platform_scope_guard psg;
			::XStringListToTextProperty(&text, 1, &name);
			::XSetWMName(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &name);
#endif
		}

		nana::string native_interface::window_caption(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			int length = ::GetWindowTextLength(reinterpret_cast<HWND>(wd));
			if(length > 0)
			{
				//One for NULL terminator which will written by GetWindowText.
				nana::string str;
				str.resize(length + 1);

				::GetWindowText(reinterpret_cast<HWND>(wd), &(str[0]), static_cast<int>(str.size()));

				//Remove the null terminator that writtien by GetWindowText
				str.resize(length);
				return str;
			}
			return nana::string();
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			::XTextProperty txtpro;
			if(::XGetWMName(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &txtpro))
			{
				char ** strlist;
				int size;
				if(::XTextPropertyToStringList(&txtpro, &strlist, &size))
				{
					if(size > 1)
					{
					#if defined(NANA_UNICODE)
						nana::string str = nana::charset(*strlist);
					#else
						nana::string str = *strlist;
					#endif
						::XFreeStringList(strlist);
						return str;
					}
				}
			}
			return STR("Nana Window");
#endif
		}

		void native_interface::capture_window(native_window_type wd, bool cap)
		{
#if defined(NANA_WINDOWS)
			if(cap)
				::SetCapture(reinterpret_cast<HWND>(wd));
			else
				::ReleaseCapture();
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			if(cap)
			{
				const unsigned mask = ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask;
				if(GrabNotViewable == ::XGrabPointer(restrict::spec.open_display(), reinterpret_cast<Window>(wd), false, mask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime))
				{
					restrict::spec.grab(reinterpret_cast<Window>(wd));
				}
			}
			else
			{
				::XUngrabPointer(restrict::spec.open_display(), CurrentTime);
				::XFlush(restrict::spec.open_display());
				restrict::spec.grab(0);
			}
#endif
		}

		nana::point native_interface::cursor_position()
		{
#if defined(NANA_WINDOWS)
			POINT point;
			::GetCursorPos(&point);
			return nana::point(point.x, point.y);
#elif defined(NANA_X11)
			nana::point pos;
			Window drop_wd;
			int x, y;
			unsigned mask;
			nana::detail::platform_scope_guard psg;
			::XQueryPointer(restrict::spec.open_display(), restrict::spec.root_window(), &drop_wd, &drop_wd,  &pos.x, &pos.y, &x, &y, &mask);
			return pos;
#endif
		}

		native_window_type native_interface::get_owner_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			return reinterpret_cast<native_window_type>(::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER));
#elif defined(NANA_X11)
			return restrict::spec.get_owner(wd);
#endif
		}

		void native_interface::caret_create(native_window_type wd, unsigned width, unsigned height)
		{
#if defined(NANA_WINDOWS)
			::CreateCaret(reinterpret_cast<HWND>(wd), 0, int(width), int(height));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			restrict::spec.caret_open(wd, width, height);
#endif
		}

		void native_interface::caret_destroy(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			if(::GetCurrentThreadId() != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::operate_caret, 1, 0);
			else
				::DestroyCaret();
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			restrict::spec.caret_close(wd);
#endif
		}

		void native_interface::caret_pos(native_window_type wd, int x, int y)
		{
#if defined(NANA_WINDOWS)
			if(::GetCurrentThreadId() != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
			{
				nana::detail::messages::caret* c = new nana::detail::messages::caret;
				c->x = x;
				c->y = y;
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::operate_caret, 2, reinterpret_cast<LPARAM>(c));
			}
			else
				::SetCaretPos(x, y);
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			restrict::spec.caret_pos(wd, x, y);
#endif
		}

		void native_interface::caret_visible(native_window_type wd, bool vis)
		{
#if defined(NANA_WINDOWS)
			(vis ? ::ShowCaret : ::HideCaret)(reinterpret_cast<HWND>(wd));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			restrict::spec.caret_visible(wd, vis);
#endif
		}

		bool native_interface::notify_icon_add(native_window_type wd, const char_t* tip, const char_t* ico)
		{
#if defined(NANA_WINDOWS)
			NOTIFYICONDATA icon_data;
			memset(&icon_data, 0, sizeof icon_data);
			icon_data.cbSize = sizeof icon_data;
			icon_data.hWnd = reinterpret_cast<HWND>(wd);
			icon_data.uID = 0;

			if(tip)
				icon_data.uFlags		= NIF_ICON | NIF_MESSAGE | NIF_TIP;
			else
				icon_data.uFlags		= NIF_ICON | NIF_MESSAGE;

			icon_data.uCallbackMessage	= nana::detail::messages::tray;
			icon_data.hIcon				= (ico ? (HICON)::LoadImage(0, ico, IMAGE_ICON, 0, 0, LR_LOADFROMFILE) : 0);

			strcpy(icon_data.szTip, (tip ? tip : STR("")));

			if(icon_data.hIcon)
				tray_manager::instance().set_icon(wd, icon_data.hIcon);

			return (TRUE == ::Shell_NotifyIcon(NIM_ADD, &icon_data));
#elif defined(NANA_X11)
			return true;
#endif
		}

		bool native_interface::notify_icon_delete(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			NOTIFYICONDATA icon_data;
			memset(&icon_data, 0, sizeof icon_data);

			icon_data.cbSize = sizeof icon_data;
			icon_data.hWnd = reinterpret_cast<HWND>(wd);
			icon_data.uID = 0;
			if(TRUE == ::Shell_NotifyIcon(NIM_DELETE, &icon_data))
			{
				tray_manager::extra_t ext;
				if(tray_manager::instance().remove(wd, ext))
				{
					if(ext.ico) ::DestroyIcon(ext.ico);
				}
				return true;
			}
			return false;
#elif defined(NANA_X11)
			return true;
#endif
		}

		void native_interface::notify_tip(native_window_type wd, const char_t* text)
		{
#if defined(NANA_WINDOWS)
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof nid;
			nid.hWnd = reinterpret_cast<HWND>(wd);
			nid.uID = 0;
			nid.uFlags = NIF_TIP;
			std::size_t len = nana::strlen(text);
			if(len >= 64) len = 63;
			memcpy(nid.szTip, text, sizeof(char_t) * len);
			nid.szTip[len] = 0;

			::Shell_NotifyIcon(NIM_MODIFY, &nid);
#endif
		}

		void native_interface::notify_icon(native_window_type wd, const char_t* icon)
		{
#if defined(NANA_WINDOWS)
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof nid;
			nid.hWnd = reinterpret_cast<HWND>(wd);
			nid.uID = 0;
			nid.uFlags = NIF_ICON;
			nid.hIcon = (HICON)::LoadImage(0, icon, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
			::Shell_NotifyIcon(NIM_MODIFY, &nid);

			HICON prev = tray_manager::instance().set_icon(wd, nid.hIcon);
			if(prev)
				::DestroyIcon(prev);
#endif
		}

		void native_interface::set_focus(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			HWND focus = ::GetFocus();
			if(wd && focus != reinterpret_cast<HWND>(wd))
			{
				if(::GetWindowThreadProcessId(focus, 0) != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
					::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::async_set_focus, 0, 0);
				else
					::SetFocus(reinterpret_cast<HWND>(wd));
			}
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			::XSetInputFocus(restrict::spec.open_display(), reinterpret_cast<Window>(wd), RevertToPointerRoot, CurrentTime);
#endif
		}

		native_window_type native_interface::get_focus_window()
		{
#if defined(NANA_WINDOWS)
			return reinterpret_cast<native_window_type>(::GetFocus());
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			Window wd;
			int revert;
			::XGetInputFocus(restrict::spec.open_display(), &wd, &revert);
			return reinterpret_cast<native_window_type>(wd);
#endif
		}

		bool native_interface::calc_screen_point(native_window_type wd, nana::point& pos)
		{
#if defined(NANA_WINDOWS)
			POINT point = {pos.x, pos.y};
			if(::ClientToScreen(reinterpret_cast<HWND>(wd), &point))
			{
				pos.x = point.x;
				pos.y = point.y;
				return true;
			}
			return false;
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			int x = pos.x, y = pos.y;
			Window child;
			return (True == ::XTranslateCoordinates(restrict::spec.open_display(),
													reinterpret_cast<Window>(wd), restrict::spec.root_window(), x, y, &pos.x, &pos.y, &child));
#endif
		}

		bool native_interface::calc_window_point(native_window_type wd, nana::point& pos)
		{
#if defined(NANA_WINDOWS)
			POINT point = {pos.x, pos.y};
			if(::ScreenToClient(reinterpret_cast<HWND>(wd), &point))
			{
				pos.x = point.x;
				pos.y = point.y;
				return true;
			}
			return false;
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			int x = pos.x, y = pos.y;
			Window child;
			return (True == ::XTranslateCoordinates(restrict::spec.open_display(),
													restrict::spec.root_window(), reinterpret_cast<Window>(wd), x, y, &pos.x, &pos.y, &child));
#endif
		}

		native_window_type native_interface::find_window(int x, int y)
		{
#if defined(NANA_WINDOWS)
			POINT pos = {x, y};
			return reinterpret_cast<native_window_type>(::WindowFromPoint(pos));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;

			Window root = restrict::spec.root_window();
			Window wd = root;
			Window child = 0;
			int dropx = 0, dropy = 0;
			while(True == ::XTranslateCoordinates(restrict::spec.open_display(), root, wd, x, y, &dropx, &dropy, &child))
			{
				if(0 == child) break;
				wd = child;
			}
			return reinterpret_cast<native_window_type>(wd);
#endif
		}

		nana::size native_interface::check_track_size(nana::size sz, unsigned ext_width, unsigned ext_height, bool true_for_max)
		{
#if defined(NANA_WINDOWS)
			int x;
			int y;
			if(true_for_max)
			{
				x = ::GetSystemMetrics(SM_CXMAXTRACK);
				y = ::GetSystemMetrics(SM_CYMAXTRACK);
				if(static_cast<unsigned>(x) < sz.width + ext_width)
					sz.width = static_cast<unsigned>(x);
				if(static_cast<unsigned>(y) < sz.height + ext_height)
					sz.height = static_cast<unsigned>(y);
			}
			else
			{
				x = ::GetSystemMetrics(SM_CXMINTRACK);
				y = ::GetSystemMetrics(SM_CYMINTRACK);
				if(static_cast<unsigned>(x) > sz.width + ext_width)
					sz.width = static_cast<unsigned>(x);
				if(static_cast<unsigned>(y) > sz.height + ext_height)
					sz.height = static_cast<unsigned>(y);
			}
#endif
			return sz;
		}
	//end struct native_interface
	}//end namespace detail
	}//end namespace gui
}//end namespace nana
