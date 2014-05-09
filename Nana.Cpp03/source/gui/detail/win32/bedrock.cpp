/*
 *	A Bedrock Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/win32/bedrock.cpp
 */

#include <nana/config.hpp>
#include PLATFORM_SPEC_HPP
#include GUI_BEDROCK_HPP
#include <nana/gui/detail/eventinfo.hpp>
#include <nana/gui/detail/element_store.hpp>
#include <nana/system/platform.hpp>
#include <sstream>
#include <windows.h>
#include <nana/system/timepiece.hpp>

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL	0x020A
#endif

typedef void (CALLBACK *win_event_proc_t)(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

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
	namespace restrict
	{
		typedef struct tagTRACKMOUSEEVENT{
			   unsigned long cbSize;
			   unsigned long dwFlags;
			   void* hwndTrack;
			   unsigned long dwHoverTime;
		} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

		typedef int (__stdcall* track_mouse_event_type)(LPTRACKMOUSEEVENT);

		int __stdcall dummy_track_mouse_event(LPTRACKMOUSEEVENT)
		{
			return 1;
		}

		track_mouse_event_type track_mouse_event;

		typedef HIMC (__stdcall * imm_get_context_type)(HWND);
		imm_get_context_type imm_get_context;

		typedef BOOL (__stdcall* imm_release_context_type)(HWND, HIMC);
		imm_release_context_type imm_release_context;

		typedef BOOL (__stdcall* imm_set_composition_font_type)(HIMC, LOGFONTW*);
		imm_set_composition_font_type imm_set_composition_font;

		typedef BOOL (__stdcall* imm_set_composition_window_type)(HIMC, LPCOMPOSITIONFORM);
		imm_set_composition_window_type imm_set_composition_window;
	}

#pragma pack(1)
	//Decoder of WPARAM and LPARAM
	struct wparam_button
	{
		bool left:1;
		bool right:1;
		bool shift:1;
		bool ctrl:1;
		bool middle:1;
		bool place_holder:3;
		char place_holder_c[1];
		short wheel_delta;
	};

	template<int Bytes>
	struct param_mouse
	{
		wparam_button button;
		short x;
		short y;
	};

	template<>
	struct param_mouse<8>
	{
		wparam_button button;
		char _x64_placeholder[4];
		short x;
		short y;
	};

	template<int Bytes>
	struct param_size
	{
		unsigned long state;
		short width;
		short height;
	};

	template<>
	struct param_size<8>
	{
		unsigned long state;
		char _x64_placeholder[4];
		short width;
		short height;
	};

	union parameter_decoder
	{
		struct
		{
			WPARAM wparam;
			LPARAM lparam;
		}raw_param;

		param_mouse<sizeof(LPARAM)> mouse;
		param_size<sizeof(LPARAM)> size;
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
		}platform;

		struct cursor_tag
		{
			core_window_t * window;
			nana::gui::cursor::t	predef_cursor;
		}cursor;

		thread_context()
			: event_pump_ref_count(0), window_count(0), event_window(0)
		{
			cursor.window = 0;
			cursor.predef_cursor = nana::gui::cursor::arrow;
		}
	};

	struct bedrock::private_impl
	{
		typedef std::map<unsigned, thread_context> thr_context_container;
		nana::threads::recursive_mutex mutex;
		thr_context_container thr_contexts;

		element_store estore;

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
				tcontext.object = 0;
			}
		}cache;

		struct menu_tag
		{
			menu_tag()
				:taken_window(0), window(0), owner(0), has_keyboard(false)
			{}

			core_window_t*	taken_window;
			native_window_type window;
			native_window_type owner;
			bool has_keyboard;
		}menu;

		struct keyboard_tracking_state_tag
		{
			keyboard_tracking_state_tag()
				:has_shortkey_occured(false), has_keyup(true), alt(0)
			{}

			bool has_shortkey_occured;
			bool has_keyup;
			unsigned long alt : 2;
		}keyboard_tracking_state;
	};

	//class bedrock defines a static object itself to implement a static singleton
	//here is the definition of this object
	bedrock bedrock::bedrock_object;

	static LRESULT WINAPI Bedrock_WIN32_WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	bedrock::bedrock()
		:impl_(new private_impl)
	{
		nana::detail::platform_spec::instance(); //to guaranty the platform_spec object is initialized before using.


		WNDCLASSEX wincl;
		wincl.hInstance = ::GetModuleHandle(0);
		wincl.lpszClassName = STR("NanaWindowInternal");
		wincl.lpfnWndProc = &Bedrock_WIN32_WindowProc;
		wincl.style = CS_DBLCLKS | CS_OWNDC;
		wincl.cbSize = sizeof(wincl);
		wincl.hIcon = ::LoadIcon (0, IDI_APPLICATION);
		wincl.hIconSm = wincl.hIcon;
		wincl.hCursor = ::LoadCursor (0, IDC_ARROW);
		wincl.lpszMenuName = 0;
		wincl.cbClsExtra = 0;
		wincl.cbWndExtra = 0;
		wincl.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

		::RegisterClassEx(&wincl);

		restrict::track_mouse_event = (restrict::track_mouse_event_type)::GetProcAddress(::GetModuleHandleA("User32.DLL"), "TrackMouseEvent");

		if(!restrict::track_mouse_event)
			restrict::track_mouse_event = restrict::dummy_track_mouse_event;

		HMODULE imm32 = ::GetModuleHandleA("Imm32.DLL");
		restrict::imm_get_context = reinterpret_cast<restrict::imm_get_context_type>(
				::GetProcAddress(imm32, "ImmGetContext"));

		restrict::imm_release_context = reinterpret_cast<restrict::imm_release_context_type>(
				::GetProcAddress(imm32, "ImmReleaseContext"));

		restrict::imm_set_composition_font = reinterpret_cast<restrict::imm_set_composition_font_type>(
				::GetProcAddress(imm32, "ImmSetCompositionFontW"));

		restrict::imm_set_composition_window = reinterpret_cast<restrict::imm_set_composition_window_type>(
				::GetProcAddress(imm32, "ImmSetCompositionWindow"));
	}

	bedrock::~bedrock()
	{
		if(wd_manager.number_of_core_window())
		{
			std::stringstream ss;
			ss<<"Nana.GUI detects a memory leaks in window_manager, "<<static_cast<unsigned>(wd_manager.number_of_core_window())<<" window(s) are not uninstalled.";
			::MessageBoxA(0, ss.str().c_str(), ("Nana C++ Library"), MB_OK);
		}

		if(evt_manager.size())
		{
			std::stringstream ss;
			ss<<"Nana.GUI detects a memory leaks in event_manager, "<<static_cast<unsigned>(evt_manager.size())<<" event(s) are not uninstalled.";
			::MessageBoxA(0, ss.str().c_str(), ("Nana C++ Library"), MB_OK);
		}
		delete impl_;
	}

	//inc_window
	//@brief: increament the number of windows
	int bedrock::inc_window(unsigned tid)
	{
		//impl refers to the object of private_impl, the object is created when bedrock is creating.
		private_impl * impl = instance().impl_;
		threads::lock_guard<threads::recursive_mutex> lock(impl->mutex);

		int & cnt = (impl->thr_contexts[tid ? tid : nana::system::this_thread_id()].window_count);
		return (cnt < 0 ? cnt = 1 : ++cnt);
	}

	bedrock::thread_context* bedrock::open_thread_context(unsigned tid)
	{
		if(0 == tid) tid = nana::system::this_thread_id();
		threads::lock_guard<threads::recursive_mutex> lock(impl_->mutex);
		if(impl_->cache.tcontext.tid == tid)
			return impl_->cache.tcontext.object;

		impl_->cache.tcontext.tid = tid;
		private_impl::thr_context_container::iterator i = impl_->thr_contexts.find(tid);
		thread_context * context = (i == impl_->thr_contexts.end() ? &(impl_->thr_contexts[tid]) : &(i->second));
		impl_->cache.tcontext.object = context;
		return context;
	}

	bedrock::thread_context* bedrock::get_thread_context(unsigned tid)
	{
		if(0 == tid) tid = nana::system::this_thread_id();

		threads::lock_guard<threads::recursive_mutex> lock(impl_->mutex);

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

		threads::lock_guard<threads::recursive_mutex> lock(impl_->mutex);

		if(impl_->cache.tcontext.tid == tid)
		{
			impl_->cache.tcontext.tid = 0;
			impl_->cache.tcontext.object = 0;
		}

		impl_->thr_contexts.erase(tid);
	}

	bedrock& bedrock::instance()
	{
		return bedrock_object;
	}

	void bedrock::map_thread_root_buffer(core_window_t* wd)
	{
		::PostMessage(reinterpret_cast<HWND>(wd->root), nana::detail::messages::map_thread_root_buffer, reinterpret_cast<WPARAM>(wd), 0);
	}

	void interior_helper_for_menu(MSG& msg, native_window_type menu_window)
	{
		switch(msg.message)
		{
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_KEYUP:
			msg.hwnd = reinterpret_cast<HWND>(menu_window);
			break;
		}
	}

	void bedrock::pump_event(window modal_window)
	{
		const unsigned tid = ::GetCurrentThreadId();
		thread_context * context = open_thread_context(tid);
		if(0 == context->window_count)
		{
			//test if there is not a window
			//GetMessage may block if there is not a window
			remove_thread_context();
			return;
		}

		++(context->event_pump_ref_count);

		wd_manager.internal_lock().revert();

		try
		{
			MSG msg;
			if(modal_window)
			{
				HWND ntv_modal = reinterpret_cast<HWND>(
									root(reinterpret_cast<core_window_t*>(modal_window)));

				HWND owner = ::GetWindow(ntv_modal, GW_OWNER);
				if(owner && owner != ::GetDesktopWindow())
					::EnableWindow(owner, false);

				while(IsWindow(ntv_modal))
				{
					::WaitMessage();
					while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
					{
						if(msg.message == WM_QUIT)	break;

						if((msg.message == WM_CHAR || msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) || !::IsDialogMessage(ntv_modal, &msg))
						{
							native_window_type menu = get_menu(reinterpret_cast<native_window_type>(msg.hwnd), true);
							if(menu) interior_helper_for_menu(msg, menu);

							::TranslateMessage(&msg);
							::DispatchMessage(&msg);

							wd_manager.remove_trash_handle(tid);
							evt_manager.remove_trash_handle(0);
						}
					}
				}
			}
			else
			{
				while(context->window_count)
				{
					if(-1 != ::GetMessage(&msg, 0, 0, 0))
					{
						native_window_type menu = get_menu(reinterpret_cast<native_window_type>(msg.hwnd), true);
						if(menu) interior_helper_for_menu(msg, menu);

						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}

					wd_manager.remove_trash_handle(tid);
					evt_manager.remove_trash_handle(0);
				}//end while

				//Empty these rest messages, there is not a window to process these messages.
				while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE));
			}
		}
		catch(...)
		{
			internal_scope_guard isg;

			std::vector<core_window_t*> v;
			wd_manager.all_handles(v);
			if(v.size())
			{
				std::vector<native_window_type> roots;
				native_window_type root = 0;
				unsigned tid = nana::system::this_thread_id();
				for(std::vector<core_window_t*>::iterator i = v.begin(), end = v.end(); i != end; ++i)
				{
					core_window_t * wd = *i;
					if((wd->thread_id == tid) && (wd->root != root))
					{
						root = wd->root;
						if(roots.end() == std::find(roots.begin(), roots.end(), root))
							roots.push_back(root);
					}
				}

				std::for_each(roots.begin(), roots.end(), interface_type::close_window);
			}

			wd_manager.internal_lock().forward();

			if(0 == --(context->event_pump_ref_count))
			{
				if(0 == modal_window || 0 == context->window_count)
					remove_thread_context();
			}

			throw;
		}

		wd_manager.internal_lock().forward();

		if(0 == --(context->event_pump_ref_count))
		{
			if(0 == modal_window || 0 == context->window_count)
				remove_thread_context();
		}
	}//end pump_event

	void make_eventinfo(eventinfo& ei, bedrock::core_window_t* wnd, unsigned msg, const parameter_decoder& pmdec)
	{
		ei.window = reinterpret_cast<window>(wnd);

		switch(msg)
		{
		case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
		case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
		case WM_LBUTTONDBLCLK: case WM_MBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
		case WM_MOUSEMOVE:
			ei.mouse.x = pmdec.mouse.x - wnd->pos_root.x;
			ei.mouse.y = pmdec.mouse.y - wnd->pos_root.y;
			ei.mouse.shift = pmdec.mouse.button.shift;
			ei.mouse.ctrl = pmdec.mouse.button.ctrl;

			switch(msg)
			{
			case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
				ei.mouse.left_button = (WM_LBUTTONUP == msg);
				ei.mouse.right_button = (WM_RBUTTONUP == msg);
				ei.mouse.mid_button = (WM_MBUTTONUP == msg);
				break;
			default:
				ei.mouse.left_button = pmdec.mouse.button.left;
				ei.mouse.mid_button = pmdec.mouse.button.middle;
				ei.mouse.right_button = pmdec.mouse.button.right;
			}
			break;
		}
	}

	//trivial_message
	//	The Windows messaging always sends a message to the window thread queue when the calling is in other thread.
	//If messages can be finished without expecting Nana's window manager, the trivail_message function would
	//handle those messages. This is a method to avoid a deadlock, that calling waits for the handling and they require
	//Nana's window manager.
	bool trivial_message(HWND wd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT & ret)
	{
		bedrock & bedrock = bedrock::instance();
		switch(msg)
		{
		case nana::detail::messages::async_activate:
			::EnableWindow(wd, true);
			::SetActiveWindow(wd);
			return true;
		case nana::detail::messages::async_set_focus:
			::SetFocus(wd);
			return true;
		case nana::detail::messages::operate_caret:
			//Refer to basis.hpp for this specification.
			switch(wParam)
			{
			case 1: //Delete
				::DestroyCaret();
				break;
			case 2: //SetPos
				::SetCaretPos(reinterpret_cast<nana::detail::messages::caret*>(lParam)->x, reinterpret_cast<nana::detail::messages::caret*>(lParam)->y);
				delete reinterpret_cast<nana::detail::messages::caret*>(lParam);
				break;
			}
			return true;
		case nana::detail::messages::map_thread_root_buffer:
			bedrock.wd_manager.map(reinterpret_cast<bedrock::core_window_t*>(wParam));
			::UpdateWindow(wd);
			return true;
		case nana::detail::messages::remote_thread_move_window:
			{
				nana::detail::messages::move_window * mw = reinterpret_cast<nana::detail::messages::move_window*>(wParam);

				::RECT r;
				::GetWindowRect(wd, &r);
				if(mw->ignore & mw->Pos)
				{
					mw->x = r.left;
					mw->y = r.top;
				}
				else
				{
					HWND owner = ::GetWindow(wd, GW_OWNER);
					if(owner)
					{
						::RECT owr;
						::GetWindowRect(owner, &owr);
						::POINT pos = {owr.left, owr.top};
						::ScreenToClient(owner, &pos);
						mw->x += (owr.left - pos.x);
						mw->y += (owr.top - pos.y);
					}
				}

				if(mw->ignore & mw->Size)
				{
					mw->width = r.right - r.left;
					mw->height = r.bottom - r.top;
				}
				::MoveWindow(wd, mw->x, mw->y, mw->width, mw->height, true);
				delete mw;
			}
			return true;
		case nana::detail::messages::remote_thread_set_window_pos:
			::SetWindowPos(wd, reinterpret_cast<HWND>(wParam), 0, 0, 0, 0, static_cast<UINT>(lParam));
			return true;
		case nana::detail::messages::remote_thread_set_window_text:
			::SetWindowTextW(wd, reinterpret_cast<wchar_t*>(wParam));
			delete [] reinterpret_cast<wchar_t*>(wParam);
			return true;
		case nana::detail::messages::remote_thread_destroy_window:
			detail::native_interface::close_window(reinterpret_cast<native_window_type>(wd));	//The owner would be actived before the message has posted in current thread.
			{
				threads::lock_guard<reversible_mutex> lock(bedrock.wd_manager.internal_lock());
				bedrock::thread_context * thrd = bedrock.get_thread_context();
				if(thrd && thrd->window_count == 0)
					::PostQuitMessage(0);
			}
			ret = ::DefWindowProc(wd, msg, wParam, lParam);
			return true;
		case nana::detail::messages::tray:
			if(wd)
			{
				eventinfo ei;
				switch(lParam)
				{
				case WM_LBUTTONDBLCLK:
				case WM_MBUTTONDBLCLK:
				case WM_RBUTTONDBLCLK:
					ei.mouse.left_button = (lParam == WM_LBUTTONDBLCLK);
					ei.mouse.mid_button = (lParam == WM_MBUTTONDBLCLK);
					ei.mouse.right_button = (lParam == WM_RBUTTONDBLCLK);
					ei.identifier = events::dbl_click::identifier;
					break;
				case WM_LBUTTONDOWN:
				case WM_MBUTTONDOWN:
				case WM_RBUTTONDOWN:
					ei.mouse.left_button = (lParam == WM_LBUTTONDOWN);
					ei.mouse.mid_button = (lParam == WM_MBUTTONDOWN);
					ei.mouse.right_button = (lParam == WM_RBUTTONDOWN);
					ei.identifier = events::mouse_down::identifier;
					break;
				case WM_MOUSEMOVE:
					ei.mouse.left_button = false;
					ei.mouse.mid_button = false;
					ei.mouse.right_button = false;
					ei.identifier = events::mouse_move::identifier;
					break;
				case WM_LBUTTONUP:
				case WM_MBUTTONUP:
				case WM_RBUTTONUP:
					ei.mouse.left_button = (lParam == WM_LBUTTONUP);
					ei.mouse.mid_button = (lParam == WM_MBUTTONUP);
					ei.mouse.right_button = (lParam == WM_RBUTTONUP);
					ei.identifier = events::mouse_up::identifier;
					break;
				case WM_MOUSELEAVE:
					ei.mouse.left_button = false;
					ei.mouse.mid_button = false;
					ei.mouse.right_button = false;
					ei.identifier = events::mouse_leave::identifier;
					break;
				}
				::POINT pos;
				::GetCursorPos(&pos);
				ei.mouse.x = static_cast<short>(pos.x);
				ei.mouse.y = static_cast<short>(pos.y);
				bedrock.wd_manager.tray_fire(reinterpret_cast<native_window_type>(wd), ei.identifier, ei);
			}
			return true;
		}

		switch(msg)
		{
		case WM_DESTROY:
		case WM_SIZING:
		case WM_SIZE:
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_PAINT:
		case WM_CLOSE:
		case WM_MOUSEACTIVATE:
		case WM_GETMINMAXINFO:
		case WM_WINDOWPOSCHANGED:
		case WM_NCDESTROY:
		case WM_NCLBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_NCMBUTTONDOWN:
		case WM_IME_STARTCOMPOSITION:
		case WM_DROPFILES:
		case WM_MOUSELEAVE:
		case WM_MOUSEWHEEL:	//The WM_MOUSELAST may not include the WM_MOUSEWHEEL when the version of SDK is low.
			return false;
		default:
			if((WM_MOUSEFIRST <= msg && msg <= WM_MOUSELAST) || (WM_KEYFIRST <= msg && msg <= WM_KEYLAST))
				return false;
		}

		ret = ::DefWindowProc(wd, msg, wParam, lParam);
		return true;
	}

	void adjust_sizing(bedrock::core_window_t* wd, ::RECT * const r, int edge, unsigned req_width, unsigned req_height)
	{
		unsigned width = static_cast<unsigned>(r->right - r->left) - wd->extra_width;
		unsigned height = static_cast<unsigned>(r->bottom - r->top) - wd->extra_height;

		if(wd->max_track_size.width && (wd->max_track_size.width < req_width))
			req_width = wd->max_track_size.width;
		else if(wd->min_track_size.width && (wd->min_track_size.width > req_width))
			req_width = wd->min_track_size.width;

		if(wd->max_track_size.height && (wd->max_track_size.height < req_height))
			req_height = wd->max_track_size.height;
		else if(wd->min_track_size.height && (wd->min_track_size.height > req_height))
			req_height = wd->min_track_size.height;

		if(req_width != width)
		{
			switch(edge)
			{
			case WMSZ_LEFT:
			case WMSZ_BOTTOMLEFT:
			case WMSZ_TOPLEFT:
				r->left = r->right - static_cast<int>(req_width) - wd->extra_width;
				break;
			case WMSZ_RIGHT:
			case WMSZ_BOTTOMRIGHT:
			case WMSZ_TOPRIGHT:
			case WMSZ_TOP:
			case WMSZ_BOTTOM:
				r->right = r->left + static_cast<int>(req_width) + wd->extra_width;
				break;
			}
		}

		if(req_height != height)
		{
			switch(edge)
			{
			case WMSZ_TOP:
			case WMSZ_TOPLEFT:
			case WMSZ_TOPRIGHT:
				r->top = r->bottom - static_cast<int>(req_height) - wd->extra_height;
				break;
			case WMSZ_BOTTOM:
			case WMSZ_BOTTOMLEFT:
			case WMSZ_BOTTOMRIGHT:
			case WMSZ_LEFT:
			case WMSZ_RIGHT:
				r->bottom = r->top + static_cast<int>(req_height) + wd->extra_height;
				break;
			}
		}
	}

	LRESULT CALLBACK Bedrock_WIN32_WindowProc(HWND root_window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT window_proc_value = 0;
		if(trivial_message(root_window, message, wParam, lParam, window_proc_value))
			return window_proc_value;

		typedef nana::gui::detail::bedrock::core_window_t * wnd_type;
		typedef detail::bedrock::window_manager_t::root_table_type::value_type wm_root_runtime_type;

		static nana::gui::detail::bedrock& bedrock = nana::gui::detail::bedrock::instance();
		static restrict::TRACKMOUSEEVENT track = {sizeof track, 0x00000002};

		native_window_type native_window = reinterpret_cast<native_window_type>(root_window);
		wm_root_runtime_type* root_runtime = bedrock.wd_manager.root_runtime(native_window);

		bool def_window_proc = false;
		if(root_runtime)
		{
			bedrock::thread_context& context = *bedrock.get_thread_context();

			wnd_type mouse_window = root_runtime->condition.mouse_window;
			wnd_type mousemove_window = root_runtime->condition.mousemove_window;

			parameter_decoder pmdec;
			pmdec.raw_param.lparam = lParam;
			pmdec.raw_param.wparam = wParam;

			eventinfo ei;

			threads::lock_guard<reversible_mutex> lock(bedrock.wd_manager.internal_lock());
			wnd_type msgwnd = root_runtime->window;

			switch(message)
			{
			case WM_IME_STARTCOMPOSITION:
				if(msgwnd->other.attribute.root->ime_enabled)
				{
					nana::paint::native_font_type native_font = msgwnd->drawer.graphics.typeface().handle();
					LOGFONTW logfont;
					::GetObjectW(reinterpret_cast<HFONT>(native_font), sizeof logfont, &logfont);

					HIMC imc = restrict::imm_get_context(root_window);
					restrict::imm_set_composition_font(imc, &logfont);

					POINT pos;
					::GetCaretPos(&pos);

					COMPOSITIONFORM cf = {CFS_POINT};
					cf.ptCurrentPos = pos;
					restrict::imm_set_composition_window(imc, &cf);
					restrict::imm_release_context(root_window, imc);
				}
				def_window_proc = true;
				break;
			case WM_GETMINMAXINFO:
				{
					bool take_over = false;
					MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);

					if(msgwnd->min_track_size.width && msgwnd->min_track_size.height)
					{
						mmi->ptMinTrackSize.x = static_cast<LONG>(msgwnd->min_track_size.width + msgwnd->extra_width);
						mmi->ptMinTrackSize.y = static_cast<LONG>(msgwnd->min_track_size.height + msgwnd->extra_height);
						take_over = true;
					}

					if(false == msgwnd->flags.fullscreen)
					{
						if(msgwnd->max_track_size.width && msgwnd->max_track_size.height)
						{
							mmi->ptMaxTrackSize.x = static_cast<LONG>(msgwnd->max_track_size.width + msgwnd->extra_width);
							mmi->ptMaxTrackSize.y = static_cast<LONG>(msgwnd->max_track_size.height + msgwnd->extra_height);
							if(mmi->ptMaxSize.x > mmi->ptMaxTrackSize.x)
								mmi->ptMaxSize.x = mmi->ptMaxTrackSize.x;
							if(mmi->ptMaxSize.y > mmi->ptMaxTrackSize.y)
								mmi->ptMaxSize.y = mmi->ptMaxTrackSize.y;

							take_over = true;
						}
					}

					if(take_over)
						return 0;
				}
				break;
			case WM_WINDOWPOSCHANGED:
				if((reinterpret_cast<WINDOWPOS*>(lParam)->flags & SWP_SHOWWINDOW) && (msgwnd->visible == false))
					bedrock.event_expose(msgwnd, true);
				else if((reinterpret_cast<WINDOWPOS*>(lParam)->flags & SWP_HIDEWINDOW) && msgwnd->visible)
					bedrock.event_expose(msgwnd, false);

				if(reinterpret_cast<WINDOWPOS*>(lParam)->flags & ~SWP_NOMOVE)
					bedrock.event_move(msgwnd, reinterpret_cast<WINDOWPOS*>(lParam)->x, reinterpret_cast<WINDOWPOS*>(lParam)->y);

				def_window_proc = true;
				break;
			case WM_SETFOCUS:
				if(msgwnd->flags.enabled && msgwnd->flags.take_active)
				{
					wnd_type focus = msgwnd->other.attribute.root->focus;

					if(focus && focus->together.caret)
						focus->together.caret->set_active(true);

					msgwnd->root_widget->other.attribute.root->context.focus_changed = true;

					ei.focus.getting = true;
					ei.focus.receiver = native_window;
					if(false == bedrock.raise_event(event_code::focus, focus, ei, true))
						bedrock.wd_manager.set_focus(msgwnd);
				}
				break;
			case WM_KILLFOCUS:
				if(msgwnd->other.attribute.root->focus)
				{
					wnd_type focus = msgwnd->other.attribute.root->focus;

					ei.focus.getting = false;
					ei.focus.receiver = reinterpret_cast<native_window_type>(wParam);
					if(bedrock.raise_event(event_code::focus, focus, ei, true))
					{
						if(focus->together.caret)
							focus->together.caret->set_active(false);
					}

					//wParam indicates a handle of window that receives the focus.
					bedrock.close_menu_if_focus_other_window(reinterpret_cast<native_window_type>(wParam));
				}
				//focus_changed means that during an event procedure if the focus is changed
				if(bedrock.wd_manager.available(msgwnd))
					msgwnd->root_widget->other.attribute.root->context.focus_changed = true;
				break;
			case WM_MOUSEACTIVATE:
				if(msgwnd->flags.take_active == false)
					return MA_NOACTIVATE;
				break;
			case WM_LBUTTONDBLCLK: case WM_MBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
				msgwnd = bedrock.wd_manager.find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if(msgwnd && msgwnd->flags.enabled)
				{
					mouse_window = msgwnd;
					if(msgwnd->flags.take_active)
						bedrock.wd_manager.set_focus(msgwnd);

					make_eventinfo(ei, msgwnd, message, pmdec);
					//After event, the mouse_window may be deleted.
					if(bedrock.raise_event(msgwnd->flags.dbl_click?event_code::dbl_click:event_code::mouse_down, msgwnd, ei, true))
					{
						if(false == bedrock.wd_manager.available(msgwnd))
							mouse_window = 0;
					}
					else
						mouse_window = 0;
				}
				break;
			case WM_NCLBUTTONDOWN: case WM_NCMBUTTONDOWN: case WM_NCRBUTTONDOWN:
				bedrock.close_menu_if_focus_other_window(native_window);
				def_window_proc = true;
				break;
			case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
				msgwnd = bedrock.wd_manager.find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if(0 == msgwnd) break;

				//if event on the menubar, just remove the menu if it is not associating with the menubar
				if((msgwnd == msgwnd->root_widget->other.attribute.root->menubar) && bedrock.get_menu(msgwnd->root, true))
					bedrock.remove_menu();
				else
					bedrock.close_menu_if_focus_other_window(msgwnd->root);

				if(msgwnd->flags.enabled)
				{
					mouse_window = msgwnd;
					wnd_type new_focus = (msgwnd->flags.take_active ? msgwnd : msgwnd->other.active_window);
					if(new_focus)
					{
						wnd_type kill_focus = bedrock.wd_manager.set_focus(new_focus);
						if(kill_focus != new_focus)
						{
							bedrock.wd_manager.do_lazy_refresh(kill_focus, false);
							msgwnd->root_widget->other.attribute.root->context.focus_changed = false;
						}
					}

					make_eventinfo(ei,msgwnd, message, pmdec);
					msgwnd->flags.action = mouse_action::pressed;
					if(bedrock.raise_event(event_code::mouse_down, msgwnd, ei, true))
					{
						//If a root_window is created during the mouse_down event, Nana.GUI will ignore the mouse_up event.
						if(msgwnd->root_widget->other.attribute.root->context.focus_changed)
						{
							nana::point pos = native_interface::cursor_position();
							native_window_type rootwd = native_interface::find_window(pos.x, pos.y);
							native_interface::calc_window_point(rootwd, pos);
							if(msgwnd != bedrock.wd_manager.find_window(rootwd, pos.x, pos.y))
							{
								//call the drawer mouse up event for restoring the surface graphics
								msgwnd->flags.action = mouse_action::normal;
								bedrock.fire_event_for_drawer(event_code::mouse_up, msgwnd, ei, &context);
								bedrock.wd_manager.do_lazy_refresh(msgwnd, false);
							}
						}
					}
					else
						mouse_window = 0;
				}
				break;
			//mouse_click, mouse_up
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
				msgwnd = bedrock.wd_manager.find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if(0 == msgwnd)
					break;

				msgwnd->flags.action = mouse_action::normal;
				if(msgwnd->flags.enabled)
				{
					make_eventinfo(ei, msgwnd, message, pmdec);
					bool hit = is_hit_the_rectangle(msgwnd->dimension, ei.mouse.x, ei.mouse.y);

					bool fire_click = false;
					if(bedrock.wd_manager.available(mouse_window) && (msgwnd == mouse_window))
					{
						if(msgwnd->flags.enabled && hit)
						{
							msgwnd->flags.action = mouse_action::over;
							bedrock.fire_event_for_drawer(event_code::click, msgwnd, ei, &context);
							fire_click = true;
						}
					}
				
					//Do mouse_up, this handle may be closed by click handler.
					if(bedrock.wd_manager.available(msgwnd) && msgwnd->flags.enabled)
					{
						if(hit)
							msgwnd->flags.action = mouse_action::over;

						bedrock.fire_event_for_drawer(event_code::mouse_up, msgwnd, ei, &context);

						if(fire_click)
							bedrock.fire_event(event_code::click, msgwnd, ei);

						bedrock.fire_event(event_code::mouse_up, msgwnd, ei);
						bedrock.wd_manager.do_lazy_refresh(msgwnd, false);
					}
					else if(fire_click)
					{
						bedrock.fire_event(event_code::click, msgwnd, ei);
						bedrock.wd_manager.do_lazy_refresh(msgwnd, false);					
					}
				}
				mouse_window = 0;
				break;
			case WM_MOUSEMOVE:
				msgwnd = bedrock.wd_manager.find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if(bedrock.wd_manager.available(mousemove_window) && (msgwnd != mousemove_window))
				{
					wnd_type leave_wd = mousemove_window;
					root_runtime->condition.mousemove_window = 0;
					mousemove_window = 0;
					//if current window is not the previous mouse event window
					make_eventinfo(ei, leave_wd, message, pmdec);
					leave_wd->flags.action = mouse_action::normal;
					bedrock.raise_event(event_code::mouse_leave, leave_wd, ei, true);

					//if msgwnd is neither captured window nor the child of captured window,
					//redirect the msgwnd to the captured window.
					wnd_type wd = bedrock.wd_manager.capture_redirect(msgwnd);
					if(wd)
						msgwnd = wd;
				}
				else if(msgwnd)
				{
					make_eventinfo(ei, msgwnd, message, pmdec);
					bool prev_captured_inside;
					if(bedrock.wd_manager.capture_window_entered(pmdec.mouse.x, pmdec.mouse.y, prev_captured_inside))
					{
						event_code::t eid;
						if(prev_captured_inside)
						{
							eid = event_code::mouse_leave;
							msgwnd->flags.action = mouse_action::normal;
						}
						else
						{
							eid = event_code::mouse_enter;
							msgwnd->flags.action = mouse_action::over;
						}
						bedrock.raise_event(eid, msgwnd, ei, true);
					}
				}

				if(msgwnd)
				{
					make_eventinfo(ei, msgwnd, message, pmdec);
					msgwnd->flags.action = mouse_action::over;
					if(mousemove_window != msgwnd)
					{
						root_runtime->condition.mousemove_window = msgwnd;
						mousemove_window = msgwnd;
						bedrock.raise_event(event_code::mouse_enter, msgwnd, ei, true);
					}
					bedrock.raise_event(event_code::mouse_move, msgwnd, ei, true);
					track.hwndTrack = native_window;
					restrict::track_mouse_event(&track);
				}
				if(false == bedrock.wd_manager.available(mousemove_window))
					mousemove_window = 0;
				break;
			case WM_MOUSELEAVE:
				if(bedrock.wd_manager.available(mousemove_window) && mousemove_window->flags.enabled)
				{
					ei.mouse.x = ei.mouse.y = 0;
					mousemove_window->flags.action = mouse_action::normal;
					bedrock.raise_event(event_code::mouse_leave, mousemove_window, ei, true);
					mousemove_window = 0;
				}
				break;
			case WM_MOUSEWHEEL:
				msgwnd = bedrock.focus();
				if(msgwnd && msgwnd->flags.enabled)
				{
					POINT point = {pmdec.mouse.x, pmdec.mouse.y};
					::ScreenToClient(reinterpret_cast<HWND>(msgwnd->root), &point);

					ei.wheel.upwards = (pmdec.mouse.button.wheel_delta >= 0);
					ei.wheel.x = static_cast<short>(point.x - msgwnd->pos_root.x);
					ei.wheel.y = static_cast<short>(point.y - msgwnd->pos_root.y);

					bedrock.raise_event(event_code::mouse_wheel, msgwnd, ei, true);
				}
				break;
			case WM_DROPFILES:
				{
					HDROP drop = reinterpret_cast<HDROP>(wParam);
					POINT pos;
					::DragQueryPoint(drop, &pos);

					msgwnd = bedrock.wd_manager.find_window(native_window, pos.x, pos.y);
					if(msgwnd)
					{
						gui::detail::tag_dropinfo di;

						nana::char_t * tmpbuf = 0;
						std::size_t bufsize = 0;

						unsigned size = ::DragQueryFile(drop, 0xFFFFFFFF, 0, 0);
						for(unsigned i = 0; i < size; ++i)
						{
							unsigned reqlen = ::DragQueryFile(drop, i, 0, 0) + 1;
							if(bufsize < reqlen)
							{
								delete [] tmpbuf;
								tmpbuf = new nana::char_t[reqlen];
								bufsize = reqlen;
							}

							::DragQueryFile(drop, i, tmpbuf, reqlen);
							di.filenames.push_back(tmpbuf);
						}

						delete [] tmpbuf;
						ei.dropinfo = &di;

						while(msgwnd && (msgwnd->flags.dropable == false))
							msgwnd = msgwnd->parent;

						if(msgwnd)
						{
							ei.dropinfo->pos.x = pos.x;
							ei.dropinfo->pos.y = pos.y;

							bedrock.wd_manager.calc_window_point(msgwnd, ei.dropinfo->pos);
							ei.window = reinterpret_cast<window>(msgwnd);

							bedrock.fire_event(event_code::mouse_drop, msgwnd, ei);
							bedrock.wd_manager.do_lazy_refresh(msgwnd, false);
						}
					}

					::DragFinish(drop);
					window_proc_value = 0;
				}
				break;
			case WM_SIZING:
				{
					::RECT* const r = reinterpret_cast<RECT*>(lParam);
					unsigned width = static_cast<unsigned>(r->right - r->left) - msgwnd->extra_width;
					unsigned height = static_cast<unsigned>(r->bottom - r->top) - msgwnd->extra_height;

					if(msgwnd->max_track_size.width || msgwnd->min_track_size.width)
					{
						if(wParam == WMSZ_LEFT || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_TOPLEFT)
						{
							if(msgwnd->max_track_size.width && (width > msgwnd->max_track_size.width))
								r->left = r->right - static_cast<int>(msgwnd->max_track_size.width) - msgwnd->extra_width;
							if(msgwnd->min_track_size.width && (width < msgwnd->min_track_size.width))
								r->left = r->right - static_cast<int>(msgwnd->min_track_size.width) - msgwnd->extra_width;
						}
						else if(wParam == WMSZ_RIGHT || wParam == WMSZ_BOTTOMRIGHT || wParam == WMSZ_TOPRIGHT)
						{
							if(msgwnd->max_track_size.width && (width > msgwnd->max_track_size.width))
								r->right = r->left + static_cast<int>(msgwnd->max_track_size.width) + msgwnd->extra_width;
							if(msgwnd->min_track_size.width && (width < msgwnd->min_track_size.width))
								r->right = r->left + static_cast<int>(msgwnd->min_track_size.width) + msgwnd->extra_width;
						}
					}

					if(msgwnd->max_track_size.height || msgwnd->min_track_size.height)
					{
						if(wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT)
						{
							if(msgwnd->max_track_size.height && (height > msgwnd->max_track_size.height))
								r->top = r->bottom - static_cast<int>(msgwnd->max_track_size.height) - msgwnd->extra_height;
							if(msgwnd->min_track_size.height && (height < msgwnd->min_track_size.height))
								r->top = r->bottom - static_cast<int>(msgwnd->min_track_size.height) - msgwnd->extra_height;
						}
						else if(wParam == WMSZ_BOTTOM || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT)
						{
							if(msgwnd->max_track_size.height && (height > msgwnd->max_track_size.height))
								r->bottom = r->top + static_cast<int>(msgwnd->max_track_size.height) + msgwnd->extra_height;
							if(msgwnd->min_track_size.height && (height < msgwnd->min_track_size.height))
								r->bottom = r->top + static_cast<int>(msgwnd->min_track_size.height) + msgwnd->extra_height;
						}
					}

					nana::size size_before(	static_cast<unsigned>(r->right - r->left - msgwnd->extra_width),
											static_cast<unsigned>(r->bottom - r->top - msgwnd->extra_height));

					eventinfo ei;
					ei.identifier = event_code::sizing;
					ei.window = reinterpret_cast<window>(msgwnd);
					ei.sizing.width = size_before.width;
					ei.sizing.height = size_before.height;

					switch(wParam)
					{
					case WMSZ_LEFT:
						ei.sizing.border = window_border::left;		break;
					case WMSZ_RIGHT:
						ei.sizing.border = window_border::right;	break;
					case WMSZ_BOTTOM:
						ei.sizing.border = window_border::bottom;	break;
					case WMSZ_BOTTOMLEFT:
						ei.sizing.border = window_border::bottom_left;	break;
					case WMSZ_BOTTOMRIGHT:
						ei.sizing.border = window_border::bottom_right;	break;
					case WMSZ_TOP:
						ei.sizing.border = window_border::top;	break;
					case WMSZ_TOPLEFT:
						ei.sizing.border = window_border::top_left;	break;
					case WMSZ_TOPRIGHT:
						ei.sizing.border = window_border::top_right;	break;
					}

					bedrock.raise_event(event_code::sizing, msgwnd, ei, false);

					if(ei.sizing.width != width || ei.sizing.height != height)
					{
						adjust_sizing(msgwnd, r, static_cast<int>(wParam), ei.sizing.width, ei.sizing.height);
						return TRUE;
					}
				}
				break;
			case WM_SIZE:
				if(wParam != SIZE_MINIMIZED)
					bedrock.wd_manager.size(msgwnd, pmdec.size.width, pmdec.size.height, true, true);
				break;
			case WM_PAINT:
				{
					::PAINTSTRUCT ps;
					::HDC dc = ::BeginPaint(root_window, &ps);

					if((ps.rcPaint.left != ps.rcPaint.right) && (ps.rcPaint.bottom != ps.rcPaint.top))
					{
						::BitBlt(dc,
								ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
								reinterpret_cast<HDC>(msgwnd->root_graph->handle()->context),
								ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
					}
					::EndPaint(root_window, &ps);
				}
				break;
			case WM_SYSCHAR:
				bedrock.set_keyboard_shortkey(true);
				msgwnd = bedrock.wd_manager.find_shortkey(native_window, static_cast<unsigned long>(wParam));
				if(msgwnd)
				{
					ei.keyboard.key = static_cast<wchar_t>(wParam < 0x61 ? wParam + 0x61 - 0x41 : wParam);
					bedrock.raise_event(event_code::shortkey, msgwnd, ei, true);
				}
				break;
			case WM_SYSKEYDOWN:
				if(bedrock.whether_keyboard_shortkey() == false)
				{
					msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
					if(msgwnd)
					{
						bedrock.wd_manager.set_focus(msgwnd);

						ei.keyboard.key = static_cast<nana::char_t>(wParam);
						bedrock.get_key_state(ei.keyboard);

						bedrock.raise_event(event_code::key_down, msgwnd, ei, true);
					}
					else if(bedrock.get_menu())
						bedrock.remove_menu();
				}
				break;
			case WM_SYSKEYUP:
				if(bedrock.set_keyboard_shortkey(false) == false)
				{
					msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
					if(msgwnd)
					{
						ei.keyboard.key = static_cast<nana::char_t>(wParam);
						bedrock.get_key_state(ei.keyboard);

						bedrock.raise_event(event_code::key_up, msgwnd, ei, true);
					}
				}
				break;
			case WM_KEYDOWN:
				if(msgwnd->flags.enabled)
				{
					if(msgwnd->root != bedrock.get_menu())
						msgwnd = bedrock.focus();

					if(msgwnd)
					{
						if((wParam == 9) && (false == (msgwnd->flags.tab & nana::gui::detail::tab_type::eating))) //Tab
						{
							wnd_type the_next = bedrock.wd_manager.tabstop_next(msgwnd);
							if(the_next)
							{
								bedrock.wd_manager.set_focus(the_next);
								bedrock.wd_manager.do_lazy_refresh(msgwnd, false);
								bedrock.wd_manager.do_lazy_refresh(the_next, true);
								root_runtime->condition.tabstop_focus_changed = true;
							}
						}
						else
						{
							ei.keyboard.key = static_cast<nana::char_t>(wParam);
							bedrock.get_key_state(ei.keyboard);

							bedrock.raise_event(event_code::key_down, msgwnd, ei, true);
						}
					}
				}
				break;
			case WM_CHAR:
				msgwnd = bedrock.focus();
				if(false == root_runtime->condition.tabstop_focus_changed)
				{
					if(msgwnd && msgwnd->flags.enabled)
					{
						ei.keyboard.key = static_cast<nana::char_t>(wParam);
						bedrock.get_key_state(ei.keyboard);
						ei.keyboard.ignore = false;

						ei.identifier = event_code::key_char;
						ei.window = reinterpret_cast<window>(msgwnd);

						bedrock.evt_manager.answer(event_code::key_char, reinterpret_cast<window>(msgwnd), ei, event_manager::event_kind::user);

						if((ei.keyboard.ignore == false) && bedrock.wd_manager.available(msgwnd))
							bedrock.fire_event_for_drawer(event_code::key_char, msgwnd, ei, &context);

						bedrock.wd_manager.do_lazy_refresh(msgwnd, false);
					}
				}
				else
					root_runtime->condition.tabstop_focus_changed = false;
				return 0;
			case WM_KEYUP:
				if(wParam != 18) //MUST NOT BE AN ALT
				{
					msgwnd = bedrock.focus();
					if(msgwnd)
					{
						ei.keyboard.key = static_cast<nana::char_t>(wParam);
						bedrock.get_key_state(ei.keyboard);

						bedrock.raise_event(event_code::key_up, msgwnd, ei, true);
					}
				}
				else
					bedrock.set_keyboard_shortkey(false);
				break;
			case WM_CLOSE:
				ei.unload.cancel = false;
				bedrock.raise_event(event_code::unload, msgwnd, ei, true);
				if(false == ei.unload.cancel)
				{
					def_window_proc = true;
					//Activate is owner, refer to the window_manager::close for the explaination
					if(msgwnd->flags.modal || (msgwnd->owner == 0) || msgwnd->owner->flags.take_active)
						native_interface::activate_owner(msgwnd->root);
				}
				break;
			case WM_DESTROY:
				if(msgwnd->root == bedrock.get_menu())
					bedrock.empty_menu();
				bedrock.wd_manager.destroy(msgwnd);
				bedrock.evt_manager.umake(reinterpret_cast<window>(msgwnd), false);
				nana::detail::platform_spec::instance().release_window_icon(msgwnd->root);
				break;
			case WM_NCDESTROY:
				bedrock.rt_manager.remove_if_exists(msgwnd);
				bedrock.wd_manager.destroy_handle(msgwnd);

				if(--context.window_count <= 0)
				{
					::PostQuitMessage(0);
					def_window_proc = true;
				}
				break;
			default:
				def_window_proc = true;
			}

			root_runtime = bedrock.wd_manager.root_runtime(native_window);
			if(root_runtime)
			{
				root_runtime->condition.mouse_window = mouse_window;
				root_runtime->condition.mousemove_window = mousemove_window;
			}
		}
		else
			def_window_proc = true;

		if(def_window_proc)
			return ::DefWindowProc(root_window, message, wParam, lParam);
		else
			return window_proc_value;
	}

	unsigned bedrock::category(bedrock::core_window_t* wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(wd_manager.available(wd))
				return wd->other.category;
		}
		return 0;
	}

	bedrock::core_window_t* bedrock::focus()
	{
		core_window_t* wd = wd_manager.root(native_interface::get_focus_window());
		return (wd ? wd->other.attribute.root->focus : 0);
	}

	native_window_type bedrock::root(bedrock::core_window_t* wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(wd_manager.available(wd))
				return wd->root;
		}
		return 0;
	}

	void bedrock::set_menubar_taken(bedrock::core_window_t* wd)
	{
		impl_->menu.taken_window = wd;
	}

	bedrock::core_window_t* bedrock::get_menubar_taken()
	{
		core_window_t* wd = impl_->menu.taken_window;
		impl_->menu.taken_window = 0;
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
		if(	(impl_->menu.owner == 0) ||
			(owner && (impl_->menu.owner == owner))
			)
		{
			return ((!is_keyboard_condition) || impl_->menu.has_keyboard ? impl_->menu.window : 0);
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
			impl_->menu.window = impl_->menu.owner = 0;
			impl_->menu.has_keyboard = false;
			native_interface::close_window(delwin);
		}
	}

	void bedrock::empty_menu()
	{
		if(impl_->menu.window)
		{
			impl_->menu.window = impl_->menu.owner = 0;
			impl_->menu.has_keyboard = false;
		}
	}

	void bedrock::get_key_state(nana::gui::detail::tag_keyboard& kb)
	{
		kb.ctrl = (0 != (::GetKeyState(VK_CONTROL) & 0x80));
	}

	bool bedrock::set_keyboard_shortkey(bool yes)
	{
		bool ret = impl_->keyboard_tracking_state.has_shortkey_occured;
		impl_->keyboard_tracking_state.has_shortkey_occured = yes;
		return ret;
	}

	bool bedrock::whether_keyboard_shortkey() const
	{
		return impl_->keyboard_tracking_state.has_shortkey_occured;
	}

	element_store& bedrock::get_element_store() const
	{
		return impl_->estore;
	}

	bool bedrock::fire_event_for_drawer(event_code::t code, core_window_t* wd, eventinfo& ei, thread_context* thrd)
	{
		if(bedrock_object.wd_manager.available(wd) == false)
			return false;

		core_window_t* prev_event_wd;
		if(thrd)
		{
			prev_event_wd = thrd->event_window;
			thrd->event_window = wd;
		}

		if(wd->other.upd_state == core_window_t::update_state::none)
			wd->other.upd_state = core_window_t::update_state::lazy;

		bool ret = bedrock_object.evt_manager.answer(code, reinterpret_cast<window>(wd), ei, event_manager::event_kind::trigger);

		if(thrd) thrd->event_window = prev_event_wd;
		return ret;
	}

	bool bedrock::fire_event(event_code::t code, core_window_t* wd, eventinfo& ei)
	{
		if(bedrock_object.wd_manager.available(wd) == false)
			return false;

		return bedrock_object.evt_manager.answer(code, reinterpret_cast<window>(wd), ei, event_manager::event_kind::user);
	}

	bool bedrock::raise_event(event_code::t code, core_window_t* wd, eventinfo& ei, bool ask_update)
	{
		if(bedrock_object.wd_manager.available(wd) == false)
			return false;

		thread_context * thrd = bedrock_object.get_thread_context();
		core_window_t* prev_event_wd;
		if(thrd)
		{
			prev_event_wd = thrd->event_window;
			thrd->event_window = wd;
			bedrock_object._m_event_filter(code, wd, thrd);
		}

		if(wd->other.upd_state == core_window_t::update_state::none)
			wd->other.upd_state = core_window_t::update_state::lazy;

		bedrock_object.evt_manager.answer(code, reinterpret_cast<window>(wd), ei, event_manager::event_kind::both);

		if(ask_update)
			bedrock_object.wd_manager.do_lazy_refresh(wd, false);
		else if(bedrock_object.wd_manager.available(wd))
			wd->other.upd_state = core_window_t::update_state::none;

		if(thrd)	thrd->event_window = prev_event_wd;
		return true;
	}

	void bedrock::event_expose(core_window_t * wd, bool exposed)
	{
		if(0 == wd) return;

		eventinfo ei;
		ei.exposed = exposed;
		wd->visible = exposed;
		if(raise_event(event_code::expose, wd, ei, false))
		{
			if(exposed == false)
			{
				if(wd->other.category != static_cast<category::flags::t>(category::root_tag::value))
				{
					//If the wd->parent is a lite_widget then find a parent until it is not a lite_widget
					wd = wd->parent;

					while(wd->other.category == static_cast<category::flags::t>(category::lite_widget_tag::value))
						wd = wd->parent;
				}
				else if(wd->other.category == static_cast<category::flags::t>(category::frame_tag::value))
					wd = wd_manager.find_window(wd->root, wd->pos_root.x, wd->pos_root.y);
			}

			wd_manager.refresh_tree(wd);
			wd_manager.map(wd);
		}
	}

	void bedrock::event_move(core_window_t* wd, int x, int y)
	{
		if(wd)
		{
			eventinfo ei;
			ei.move.x = x;
			ei.move.y = y;
			if(raise_event(event_code::move, wd, ei, false))
				wd_manager.update(wd, true, true);
		}	
	}

	void bedrock::thread_context_destroy(core_window_t * wd)
	{
		bedrock::thread_context * thr = get_thread_context(0);
		if(thr && thr->event_window == wd)
			thr->event_window = 0;
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

	const nana::char_t* translate(nana::gui::cursor::t id)
	{
		const nana::char_t* name = IDC_ARROW;

		switch(id)
		{
		case nana::gui::cursor::arrow:
			name = IDC_ARROW;	break;
		case nana::gui::cursor::wait:
			name = IDC_WAIT;	break;
		case nana::gui::cursor::hand:
			name = IDC_HAND;	break;
		case nana::gui::cursor::size_we:
			name = IDC_SIZEWE;	break;
		case nana::gui::cursor::size_ns:
			name = IDC_SIZENS;	break;
		case nana::gui::cursor::size_bottom_left:
		case nana::gui::cursor::size_top_right:
			name = IDC_SIZENESW;	break;
		case nana::gui::cursor::size_top_left:
		case nana::gui::cursor::size_bottom_right:
			name = IDC_SIZENWSE;	break;
		case nana::gui::cursor::iterm:
			name = IDC_IBEAM;	break;
		}
		return name;
	}

	void SetCursor(bedrock::core_window_t* wd, nana::gui::cursor::t cur)
	{
#ifdef _WIN64
			::SetClassLongPtr(reinterpret_cast<HWND>(wd->root), GCLP_HCURSOR,
					reinterpret_cast<LONG_PTR>(::LoadCursor(0, translate(cur))));
#else
			::SetClassLong(reinterpret_cast<HWND>(wd->root), GCL_HCURSOR,
					static_cast<unsigned long>(reinterpret_cast<size_t>(::LoadCursor(0, translate(cur)))));
#endif
	}

	void bedrock::update_cursor(core_window_t * wd)
	{
		internal_scope_guard isg;
		if(wd_manager.available(wd))
		{
			thread_context * thrd = get_thread_context(wd->thread_id);
			if(0 == thrd)
				return;
			
			if((wd->predef_cursor == nana::gui::cursor::arrow) && (thrd->cursor.window == wd))
			{
				if(thrd->cursor.predef_cursor != nana::gui::cursor::arrow)
				{
					SetCursor(wd, nana::gui::cursor::arrow);
					thrd->cursor.window = 0;
					thrd->cursor.predef_cursor = nana::gui::cursor::arrow;
				}
				return;
			}

			nana::point pos = native_interface::cursor_position();
			native_window_type native_handle = native_interface::find_window(pos.x, pos.y);
			if(0 == native_handle)
				return;

			native_interface::calc_window_point(native_handle, pos);
			if(wd != wd_manager.find_window(native_handle, pos.x, pos.y))
				return;

			if(wd->predef_cursor != thrd->cursor.predef_cursor)
			{
				if(thrd->cursor.predef_cursor != nana::gui::cursor::arrow)
					thrd->cursor.window = 0;

				if(wd->predef_cursor != cursor::arrow)
				{
					thrd->cursor.window = wd;
					SetCursor(wd, wd->predef_cursor);
				}
				thrd->cursor.predef_cursor = wd->predef_cursor;
			}
		}
	}

	void bedrock::_m_event_filter(event_code::t code, core_window_t * wd, thread_context * thrd)
	{
		switch(code)
		{
		case events::mouse_enter::identifier:
			if(wd->predef_cursor != cursor::arrow)
			{
				thrd->cursor.window = wd;
				if(wd->predef_cursor != thrd->cursor.predef_cursor)
					thrd->cursor.predef_cursor = wd->predef_cursor;
				SetCursor(wd, thrd->cursor.predef_cursor);
			}
			break;
		case events::mouse_leave::identifier:
			if(wd->predef_cursor != cursor::arrow)
				SetCursor(wd, nana::gui::cursor::arrow);
			break;
		case events::destroy::identifier:
			if(wd == thrd->cursor.window)
			{
				SetCursor(wd, nana::gui::cursor::arrow);
				thrd->cursor.predef_cursor = cursor::arrow;
				thrd->cursor.window = 0;
			}
			break;
		default:
			break;
		}
	}
}//end namespace detail
}//end namespace gui
}//end namespace nana
