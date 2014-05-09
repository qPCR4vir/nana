/*
 *	A Drawer Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/drawer.cpp
 */

#include <nana/config.hpp>
#include GUI_BEDROCK_HPP
#include <nana/gui/detail/drawer.hpp>
#include <nana/gui/detail/dynamic_drawing_object.hpp>
#include <nana/gui/detail/effects_renderer.hpp>

#if defined(NANA_X11)
	#include <nana/detail/linux_X11/platform_spec.hpp>
#endif

namespace nana
{
namespace gui
{
	typedef detail::edge_nimbus_renderer<detail::bedrock::core_window_t> edge_nimbus_renderer_t;

	//class drawer_trigger
		drawer_trigger::~drawer_trigger(){}
		void drawer_trigger::attached(widget_reference, graph_reference){}	//none-const
		void drawer_trigger::detached(){}	//none-const
		void drawer_trigger::typeface_changed(graph_reference){}
		void drawer_trigger::refresh(graph_reference){}

		void drawer_trigger::resizing(graph_reference, const eventinfo&){}
		void drawer_trigger::resize(graph_reference graph, const eventinfo&)
		{
			refresh(graph);
			detail::bedrock::instance().thread_context_lazy_refresh();
		}

		void drawer_trigger::move(graph_reference, const eventinfo&){}
		void drawer_trigger::click(graph_reference, const eventinfo&){}
		void drawer_trigger::dbl_click(graph_reference, const eventinfo&){}
		void drawer_trigger::mouse_enter(graph_reference, const eventinfo&){}
		void drawer_trigger::mouse_move(graph_reference, const eventinfo&){}
		void drawer_trigger::mouse_leave(graph_reference, const eventinfo&){}
		void drawer_trigger::mouse_down(graph_reference, const eventinfo&){}
		void drawer_trigger::mouse_up(graph_reference, const eventinfo&){}
		void drawer_trigger::mouse_wheel(graph_reference, const eventinfo&){}
		void drawer_trigger::mouse_drop(graph_reference, const eventinfo&){}
		void drawer_trigger::focus(graph_reference, const eventinfo&){}
		void drawer_trigger::key_down(graph_reference, const eventinfo&){}
		void drawer_trigger::key_char(graph_reference, const eventinfo&){}
		void drawer_trigger::key_up(graph_reference, const eventinfo&){}
		void drawer_trigger::shortkey(graph_reference, const eventinfo&){}

	//end class drawer_trigger

	namespace detail
	{
		typedef bedrock bedrock_type;

			class drawer_binder
			{
			public:
				drawer_binder(drawer& object, void (drawer::*routine)(const eventinfo&))
					:this_(object), routine_(routine)
				{}

				void operator()(const eventinfo& ei)
				{
					(this_.*routine_)(ei);
				}
			private:
				drawer& this_;
				void (drawer::* routine_)(const eventinfo&);
			};

		//class drawer
		drawer::drawer()
			:	core_window_(0), realizer_(0), refreshing_(false)
		{
		}

		drawer::~drawer()
		{
			std::vector<dynamic_drawing::object*>::iterator i = dynamic_drawing_objects_.begin();
			for(; i != dynamic_drawing_objects_.end(); ++i)
			{
				delete (*i);
			}
		}

		void drawer::bind(basic_window* cw)
		{
			core_window_ = cw;
		}

		void drawer::typeface_changed()
		{
			if(realizer_)
				realizer_->typeface_changed(graphics);
		}

		void drawer::click(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->click(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::dbl_click(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->dbl_click(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::mouse_enter(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->mouse_enter(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::mouse_move(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->mouse_move(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::mouse_leave(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->mouse_leave(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::mouse_down(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->mouse_down(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::mouse_up(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->mouse_up(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::mouse_wheel(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->mouse_wheel(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::mouse_drop(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->mouse_drop(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::resizing(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->resizing(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::resize(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->resize(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::move(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->move(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::focus(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->focus(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::key_down(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->key_down(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::key_char(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->key_char(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::key_up(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->key_up(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::shortkey(const eventinfo& ei)
		{
			if(realizer_)
			{
				_m_bground_pre();
				realizer_->shortkey(graphics, ei);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
			}
		}

		void drawer::map(window wd)	//Copy the root buffer to screen
		{
			if(wd)
			{
				bedrock_type::core_window_t* iwd = reinterpret_cast<bedrock_type::core_window_t*>(wd);
				bedrock_type::core_window_t* caret_wd = iwd->root_widget->other.attribute.root->focus;
				bool owns_caret = (caret_wd && caret_wd->together.caret && caret_wd->together.caret->visible());

				//The caret in X11 is implemented by Nana, it is different from Windows'
				//the caret in X11 is asynchronous, it is hard to hide and show the caret
				//immediately, and therefore the caret always be flickering when the graphics
				//buffer is mapping to the window.
				if(owns_caret)
				{
#ifndef NANA_X11
					caret_wd->together.caret->visible(false);
#else
					owns_caret = nana::detail::platform_spec::instance().caret_update(iwd->root, *iwd->root_graph, false);
#endif
				}

				if(false == edge_nimbus_renderer_t::instance().render(iwd))
				{
					nana::rectangle vr;
					if(bedrock_type::window_manager_t::wndlayout_type::read_visual_rectangle(iwd, vr))
						iwd->root_graph->paste(iwd->root, vr, vr.x, vr.y);
				}

				if(owns_caret)
				{
#ifndef NANA_X11
					caret_wd->together.caret->visible(true);
#else
					nana::detail::platform_spec::instance().caret_update(iwd->root, *iwd->root_graph, true);
#endif
				}
			}
		}

		void drawer::refresh()
		{
			if(realizer_ && (refreshing_ == false))
			{
				refreshing_ = true;
				_m_bground_pre();
				realizer_->refresh(graphics);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
				graphics.flush();
				refreshing_ = false;
			}
		}

		drawer_trigger* drawer::realizer() const
		{
			return realizer_;
		}

		void drawer::attached(widget& wd, drawer_trigger& realizer)
		{
			realizer_ = &realizer;
			realizer.attached(wd, graphics);
		}

		drawer_trigger* drawer::detached()
		{
			if(realizer_)
			{
				drawer_trigger * old = realizer_;
				realizer_ = 0;
				old->detached();
				return old;
			}
			return 0;
		}

		void drawer::clear()
		{
			std::vector<dynamic_drawing::object*> then;
			std::vector<nana::gui::detail::dynamic_drawing::object*>::iterator i = dynamic_drawing_objects_.begin();
			for(; i != dynamic_drawing_objects_.end(); ++i)
			{
				dynamic_drawing::object * p = *i;
				if(p->diehard())
					then.push_back(p);
				else
					delete p;
			}

			then.swap(dynamic_drawing_objects_);
		}

		void* drawer::draw(const nana::functor<void(paint::graphics&)> & f, bool diehard)
		{
			if(false == f.empty())
			{
				dynamic_drawing::user_draw_function * p = new dynamic_drawing::user_draw_function(f, diehard);
				dynamic_drawing_objects_.push_back(p);
				return (diehard ? p : 0);
			}
			return 0;
		}

		void drawer::erase(void * p)
		{
			if(p)
			{
				std::vector<dynamic_drawing::object*>::iterator i = std::find(dynamic_drawing_objects_.begin(), dynamic_drawing_objects_.end(), p);
				if(i != dynamic_drawing_objects_.end())
					dynamic_drawing_objects_.erase(i);
			}
		}

		void drawer::string(int x, int y, unsigned color, const nana::char_t* text)
		{
			if(text)
			{
				dynamic_drawing_objects_.push_back(new detail::dynamic_drawing::string(x, y, color, text));
			}
		}

		void drawer::line(int x, int y, int x2, int y2, unsigned color)
		{
			dynamic_drawing_objects_.push_back(new detail::dynamic_drawing::line(x, y, x2, y2, color));
		}

		void drawer::rectangle(int x, int y, unsigned width, unsigned height, unsigned color, bool issolid)
		{
			dynamic_drawing_objects_.push_back(new detail::dynamic_drawing::rectangle(x, y, width, height, color, issolid));
		}

		void drawer::shadow_rectangle(int x, int y, unsigned width, unsigned height, nana::color_t beg, nana::color_t end, bool vertical)
		{
			dynamic_drawing_objects_.push_back(new detail::dynamic_drawing::shadow_rectangle(x, y, width, height, beg, end, vertical));
		}

		void drawer::bitblt(int x, int y, unsigned width, unsigned height, const paint::graphics& graph, int srcx, int srcy)
		{
			dynamic_drawing_objects_.push_back(new detail::dynamic_drawing::bitblt<paint::graphics>(x, y, width, height, graph, srcx, srcy));
		}

		void drawer::bitblt(int x, int y, unsigned width, unsigned height, const paint::image& img, int srcx, int srcy)
		{
			dynamic_drawing_objects_.push_back(new detail::dynamic_drawing::bitblt<paint::image>(x, y, width, height, img, srcx, srcy));
		}

		void drawer::stretch(const nana::rectangle & r_dst, const paint::graphics& graph, const nana::rectangle& r_src)
		{
			dynamic_drawing_objects_.push_back(new detail::dynamic_drawing::stretch<paint::graphics>(r_dst, graph, r_src));
		}

		void drawer::stretch(const nana::rectangle & r_dst, const paint::image& img, const nana::rectangle& r_src)
		{
			dynamic_drawing_objects_.push_back(new detail::dynamic_drawing::stretch<paint::image>(r_dst, img, r_src));
		}

		event_handle drawer::make_event(event_code::t evtid, window wd)
		{
			bedrock_type & bedrock = bedrock_type::instance();
			void (drawer::*answer)(const eventinfo&) = 0;
			switch(evtid)
			{
			case event_code::click:
				answer = &drawer::click;	break;
			case event_code::dbl_click:
				answer = &drawer::dbl_click;	break;
			case event_code::mouse_enter:
				answer = &drawer::mouse_enter;	break;
			case event_code::mouse_leave:
				answer = &drawer::mouse_leave;	break;
			case event_code::mouse_down:
				answer = &drawer::mouse_down;	break;
			case event_code::mouse_up:
				answer = &drawer::mouse_up;	break;
			case event_code::mouse_move:
				answer = &drawer::mouse_move;	break;
			case event_code::mouse_wheel:
				answer = &drawer::mouse_wheel;	break;
			case event_code::mouse_drop:
				answer = &drawer::mouse_drop;	break;
			case event_code::sizing:
				answer = &drawer::resizing;	break;
			case event_code::size:
				answer = &drawer::resize;	break;
			case event_code::move:
				answer = &drawer::move;		break;
			case event_code::focus:
				answer = &drawer::focus;	break;
			case event_code::key_down:
				answer = &drawer::key_down;	break;
			case event_code::key_char:
				answer = &drawer::key_char;	break;
			case event_code::key_up:
				answer = &drawer::key_up;	break;
			case event_code::shortkey:
				answer = &drawer::shortkey;	break;
			default:
				break;
			}

			if(answer && (0 == bedrock.evt_manager.the_number_of_handles(wd, evtid, true)))
				return bedrock.evt_manager.make_for_drawer(evtid, wd, bedrock.category(reinterpret_cast<bedrock::core_window_t*>(wd)), drawer_binder(*this, answer));

			return 0;
		}

		void drawer::_m_bground_pre()
		{
			if(core_window_->effect.bground && core_window_->effect.bground_fade_rate < 0.01)
				core_window_->other.glass_buffer.paste(graphics, 0, 0);
		}

		void drawer::_m_bground_end()
		{
			if(core_window_->effect.bground && core_window_->effect.bground_fade_rate >= 0.01)
				core_window_->other.glass_buffer.blend(core_window_->other.glass_buffer.size(), graphics, nana::point(), core_window_->effect.bground_fade_rate);
		}

		void drawer::_m_draw_dynamic_drawing_object()
		{
			std::vector<nana::gui::detail::dynamic_drawing::object*>::iterator it = dynamic_drawing_objects_.begin(), end = dynamic_drawing_objects_.end();
			for(; it != end; ++it)
				(*it)->draw(graphics);
		}
	}//end namespace detail
}//end namespace gui
}//end namespace nana
