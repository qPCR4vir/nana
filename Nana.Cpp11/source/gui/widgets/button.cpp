/*
 *	A Button Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/button.cpp
 */

#include <nana/gui/widgets/button.hpp>
#include <nana/paint/text_renderer.hpp>

namespace nana{namespace gui{
namespace drawerbase
{
	namespace button
	{
		struct image_block
		{
			bool enable;
			nana::gui::button::state who;
			nana::point pos;
		};

		struct trigger::bgimage_tag
		{
			nana::paint::image image;
			nana::rectangle valid_area;
			nana::arrange	arrange;
			nana::size		block_size;
			static const int state_number = static_cast<int>(state::disabled) + 1;
			image_block		block[state_number];

			struct stretch_tag
			{
				nana::arrange arrange;
				int beg;
				int end;
			}stretch;

			bgimage_tag()
			{
				for(int i = 0; i < state_number; ++i)
				{
					block[i].enable = true;
					block[i].who = static_cast<state>(i);
				}

				stretch.arrange = nana::arrange::unknown;
				stretch.beg = stretch.end = 0;
			}

			void set_valid_area(nana::arrange arg, const nana::rectangle& r)
			{
				arrange = arg;
				valid_area = r;
			}

			bool enable(state sta, bool eb)
			{
				if(state::normal <= sta && sta <= state::disabled)
				{
					if(block[static_cast<int>(sta)].enable != eb)
					{
						block[static_cast<int>(sta)].enable = eb;
						this->update_blocks();
						return true;
					}
				}
				return false;
			}

			bool join(state to, state from)
			{
				if(to != from)
				{
					if(state::normal <= to && to <= state::disabled && state::normal <= from && from <= state::disabled)
					{
						if(block[static_cast<int>(from)].who != block[static_cast<int>(to)].who)
						{
							block[static_cast<int>(from)].who = block[static_cast<int>(to)].who;
							update_blocks();
							return true;
						}
					}
				}
				return false;
			}

			void set_stretch(nana::arrange arg, int beg, int end)
			{
				stretch.arrange = arg;
				if(beg > end) beg = end;
				stretch.beg = beg;
				stretch.end = end;
			}

			void update_blocks()
			{
				int blocks = 0;
				image_block * blockptr = block;
				for(int i = 0; i < state_number; ++i, ++blockptr)
				{
					if(blockptr->enable && (blockptr->who == static_cast<state>(i)))
						++blocks;
				}

				if(blocks == 0) return;

				unsigned each_pixels = (arrange == nana::arrange::horizontal ? valid_area.width : valid_area.height) / blocks;
				if(arrange == nana::arrange::horizontal)
				{
					each_pixels = valid_area.width / blocks;
					block_size.width = each_pixels;
					block_size.height = valid_area.height;
				}
				else
				{
					each_pixels = valid_area.height / blocks;
					block_size.height = each_pixels;
					block_size.width = valid_area.width;
				}

				int pos = 0;
				blockptr = block;
				for(int i = 0; i < state_number; ++i, ++blockptr)
				{
					if(blockptr->enable && (blockptr->who == static_cast<state>(i)))
					{
						blockptr->pos = nana::point(valid_area.x, valid_area.y);
						if(arrange == nana::arrange::horizontal)
							blockptr->pos.x += pos;
						else
							blockptr->pos.y += pos;
						pos += static_cast<int>(each_pixels);
					}
				}
			}
		};

		//trigger
		//@brief: draw the button
		trigger::trigger()
			:widget_(nullptr), graph_(nullptr), bgimage_(nullptr)
		{
			attr_.omitted = attr_.focused = attr_.pressed = attr_.enable_pushed = false;
			attr_.focus_color = true;
			attr_.icon = nullptr;
			attr_.act_state = state::normal;
		}

		trigger::~trigger()
		{
			delete attr_.icon;
			delete bgimage_;
		}

		void trigger::bind_window(widget_reference wd)
		{
			widget_ = &wd;
		}

		void trigger::attached(graph_reference graph)
		{
			window wd = widget_->handle();

			using namespace API::dev;
			make_drawer_event<events::mouse_enter>(wd);
			make_drawer_event<events::mouse_leave>(wd);
			make_drawer_event<events::mouse_down>(wd);
			make_drawer_event<events::mouse_up>(wd);
			make_drawer_event<events::key_char>(wd);
			make_drawer_event<events::key_down>(wd);
			make_drawer_event<events::focus>(wd);

			API::tabstop(wd);
			API::effects_edge_nimbus(wd, effects::edge_nimbus::active);
			API::effects_edge_nimbus(wd, effects::edge_nimbus::over);

			graph_ = &graph;
		}

		void trigger::detached()
		{
			API::dev::umake_drawer_event(widget_->handle());
		}

		bool trigger::enable_pushed(bool eb)
		{
			attr_.enable_pushed = eb;
			return((eb == false) && pushed(false));
		}

		bool trigger::pushed(bool pshd)
		{
			if(pshd != attr_.pressed)
			{
				attr_.pressed = pshd;
				if(false == pshd)
				{
					window wd = API::find_window(API::cursor_position());
					if(wd == widget_->handle())
						attr_.act_state = state::highlight;
					else
						attr_.act_state = (attr_.focused ? state::focused : state::normal);
				}
				else
					attr_.act_state = state::pressed;

				return true;
			}
			return false;
		}

		bool trigger::pushed() const
		{
			return attr_.pressed;
		}

		void trigger::omitted(bool om)
		{
			attr_.omitted = om;
		}

		bool trigger::focus_color(bool eb)
		{
			if(eb != attr_.focus_color)
			{
				attr_.focus_color = eb;
				return true;
			}
			return false;
		}

		void trigger::refresh(graph_reference graph)
		{
			_m_draw(graph);
		}

		void trigger::mouse_enter(graph_reference graph, const eventinfo&)
		{
			attr_.act_state = (attr_.pressed ?  state::pressed : state::highlight);
			_m_draw(graph);
			API::lazy_refresh();
		}

		void trigger::mouse_leave(graph_reference graph, const eventinfo&)
		{
			if(attr_.enable_pushed && attr_.pressed)
				return;

			attr_.act_state = (attr_.focused ? state::focused : state::normal);
			_m_draw(graph);
			API::lazy_refresh();
		}

		void trigger::mouse_down(graph_reference graph, const eventinfo&)
		{
			attr_.act_state = state::pressed;
			_m_draw(graph);
			API::lazy_refresh();
		}

		void trigger::mouse_up(graph_reference graph, const eventinfo&)
		{
			if(attr_.enable_pushed && (false == attr_.pressed))
			{
				attr_.pressed = true;
			}
			else
			{
				if(attr_.act_state == state::pressed)
					attr_.act_state = state::highlight;
				else
					attr_.act_state = (attr_.focused ? state::focused : state::normal);

				attr_.pressed = false;
				_m_draw(graph);
				API::lazy_refresh();
			}
		}

		void trigger::key_char(graph_reference, const eventinfo& ei)
		{
			if(ei.keyboard.key == static_cast<char_t>(gui::keyboard::enter))
			{
				eventinfo e;
				e.mouse.ctrl = false;
				e.mouse.mid_button = e.mouse.right_button = false;
				e.mouse.left_button = true;
				e.mouse.shift = false;
				e.mouse.x = e.mouse.y = 0;
				API::raise_event<events::click>(widget_->handle(), e);
			}
		}

		void trigger::key_down(graph_reference, const eventinfo& ei)
		{
			bool ch_tabstop_next;
			switch(ei.keyboard.key)
			{
			case keyboard::os_arrow_left: case keyboard::os_arrow_up:
				ch_tabstop_next = false;
				break;
			case keyboard::os_arrow_right: case keyboard::os_arrow_down:
				ch_tabstop_next = true;
				break;
			default:
				return;
			}
			API::move_tabstop(widget_->handle(), ch_tabstop_next);
		}

		void trigger::focus(graph_reference graph, const eventinfo& ei)
		{
			attr_.focused = ei.focus.getting;
			_m_draw(graph);
			API::lazy_refresh();
		}

		void trigger::_m_draw_title(graph_reference graph, bool enabled)
		{
			nana::string text = widget_->caption();

			nana::string::value_type shortkey;
			nana::string::size_type shortkey_pos;
			nana::string str = API::transform_shortkey_text(text, shortkey, &shortkey_pos);

			nana::size ts = graph.text_extent_size(str);
			nana::size gsize = graph.size();

			nana::size icon_sz;
			if(attr_.icon)
			{
				icon_sz = attr_.icon->size();
				icon_sz.width += 5;
			}

			int x = (static_cast<int>(gsize.width  - 1  - ts.width) >> 1);
			int y = (static_cast<int>(gsize.height - 1 - ts.height) >> 1);

			if(x < static_cast<int>(icon_sz.width))
				x = static_cast<int>(icon_sz.width);

			unsigned omitted_pixels = gsize.width - icon_sz.width;
			std::size_t txtlen = str.size();
			const nana::char_t* txtptr = str.c_str();
			if(ts.width)
			{
				nana::paint::text_renderer tr(graph);
				if(enabled)
				{
					if(attr_.act_state == state::pressed)
					{
						++x;
						++y;
					}
					color_t fgcolor = (attr_.focus_color ? (attr_.focused ? 0xFF : attr_.fgcolor) : attr_.fgcolor);
					if(attr_.omitted)
						tr.render(x, y, fgcolor, txtptr, txtlen, omitted_pixels, true);
					else
						graph.bidi_string(x, y, fgcolor, txtptr, txtlen);

					if(shortkey)
					{
						unsigned off_w = (shortkey_pos ? graph.text_extent_size(str, static_cast<unsigned>(shortkey_pos)).width : 0);
						nana::size shortkey_size = graph.text_extent_size(txtptr + shortkey_pos, 1);
						x += off_w;
						y += shortkey_size.height;
						graph.line(x, y, x + shortkey_size.width - 1, y, 0x0);
					}
				}
				else
				{
					if(attr_.omitted)
					{
						tr.render(x + 1, y + 1, 0xFFFFFF, txtptr, txtlen, omitted_pixels, true);
						tr.render(x, y, 0x808080, txtptr, txtlen, omitted_pixels, true);
					}
					else
					{
						graph.bidi_string(x + 1, y + 1, 0xFFFFFF, txtptr, txtlen);
						graph.bidi_string(x, y, 0x808080, txtptr, txtlen);
					}
				}
			}

			if(attr_.icon)
				attr_.icon->paste(graph, 3, (gsize.height - icon_sz.height) / 2);
		}

		void trigger::_m_draw(graph_reference graph)
		{
			window wd = widget_->handle();
			bool eb = API::window_enabled(wd);
			bool def_bground = true;
			attr_.bgcolor = API::background(wd);
			attr_.fgcolor = API::foreground(wd);
			if(bgimage_)
			{
				int which = static_cast<int>(eb ? attr_.act_state : state::disabled);
				image_block & block = bgimage_->block[static_cast<int>(bgimage_->block[which].who)];
				if(block.enable)
				{
					def_bground = false;
					if(bgimage_->stretch.arrange == nana::arrange::horizontal && (bgimage_->stretch.beg < bgimage_->stretch.end))
					{
						unsigned img_beg_width = static_cast<unsigned>(bgimage_->stretch.beg);
						unsigned img_mid_width = static_cast<unsigned>(bgimage_->stretch.end - bgimage_->stretch.beg);
						unsigned img_end_width = bgimage_->block_size.width - static_cast<unsigned>(bgimage_->stretch.end);
						unsigned height = bgimage_->block_size.height;

						if(bgimage_->stretch.beg)
							bgimage_->image.paste(nana::rectangle(block.pos, nana::size(img_beg_width, height)), graph, nana::point());

						unsigned width = graph.width() - (img_beg_width + img_end_width);
						bgimage_->image.stretch(nana::rectangle(block.pos.x + bgimage_->stretch.beg, block.pos.y, img_mid_width, height), graph, nana::rectangle(bgimage_->stretch.beg, 0, width, height));
						if(bgimage_->stretch.end)
							bgimage_->image.paste(nana::rectangle(bgimage_->stretch.end, block.pos.y, img_end_width, height), graph, nana::point(graph.width() - img_end_width, 0));
					}
					else if((bgimage_->stretch.arrange == nana::arrange::horizontal_vertical) && (bgimage_->stretch.beg >= bgimage_->stretch.end))
					{
						bgimage_->image.stretch(nana::rectangle(block.pos, bgimage_->block_size), graph, graph.size());
					}
					else
					{
						if(graph.width() > bgimage_->block_size.width || graph.height() > bgimage_->block_size.height)
						{
							_m_draw_background(graph);
							_m_draw_border(graph);
						}
						bgimage_->image.paste(nana::rectangle(block.pos, bgimage_->block_size), graph, nana::point());
					}
				}
			}
			if(def_bground)
			{
				_m_draw_background(graph);
				_m_draw_border(graph);
			}
			_m_draw_title(graph, eb);
		}

		void trigger::_m_draw_background(graph_reference graph)
		{
			nana::rectangle r(graph.size());
			r.pare_off(1);
			nana::color_t color_start = nana::paint::graphics::mix(attr_.bgcolor, 0xFFFFFF, 0.2);
			nana::color_t color_end = nana::paint::graphics::mix(attr_.bgcolor, 0x0, 0.95);

			if(attr_.act_state == state::pressed)
			{
				r.x = r.y = 2;
				std::swap(color_start, color_end);
			}
			graph.shadow_rectangle(r, color_start, color_end, true);
		}

		void trigger::_m_draw_border(graph_reference graph)
		{
			nana::rectangle r(graph.size());
			int right = r.width - 1;
			int bottom = r.height - 1;
			graph.line(1, 0, right - 1, 0, 0x7F7F7F);
			graph.line(1, bottom, right - 1, bottom, 0x707070);
			graph.line(0, 1, 0, bottom - 1, 0x7F7F7F);
			graph.line(right, 1, right, bottom - 1, 0x707070);

			graph.set_pixel(1, 1, 0x919191);
			graph.set_pixel(right - 1, 1, 0x919191);
			graph.set_pixel(right - 1, bottom - 1, 0x919191);
			graph.set_pixel(1, bottom - 1, 0x919191);

			graph.set_pixel(0, 0, gui::color::button_face);
			graph.set_pixel(right, 0, gui::color::button_face);
			graph.set_pixel(0, bottom, gui::color::button_face);
			graph.set_pixel(right, bottom, gui::color::button_face);

			if(attr_.act_state == state::pressed)
				graph.rectangle(r.pare_off(1), 0xC3C3C3, false);
		}

		void trigger::icon(const nana::paint::image& img)
		{
			if(img.empty())	return;

			if(nullptr == attr_.icon)
				attr_.icon = new paint::image;
			*attr_.icon = img;
		}

		void trigger::image(const nana::paint::image& img)
		{
			delete bgimage_;
			bgimage_ = nullptr;

			if(img)
			{
				bgimage_ = new bgimage_tag;
				bgimage_->image = img;
				bgimage_->set_valid_area(nana::arrange::horizontal, img.size());
			}
		}

		trigger::bgimage_tag * trigger::ref_bgimage()
		{
			return bgimage_;
		}
		//end class trigger
	}//end namespace button
}//end namespace drawerbase

		//button
		//@brief: Defaine a button widget and it provides the interfaces to be operational
			button::button(){}

			button::button(window wd, bool visible)
			{
				create(wd, rectangle(), visible);
			}

			button::button(window wd, const rectangle& r, bool visible)
			{
				create(wd, r, visible);
			}

			void button::icon(const nana::paint::image& img)
			{
				internal_scope_guard isg;
				get_drawer_trigger().icon(img);
				API::refresh_window(handle());
			}

			void button::image(const nana::char_t * filename)
			{
				nana::paint::image img;
				if(img.open(filename))
				{
					internal_scope_guard isg;
					get_drawer_trigger().image(img);
					API::refresh_window(handle());
				}
			}

			void button::image(const nana::paint::image& img)
			{
				internal_scope_guard isg;
				get_drawer_trigger().image(img);
				API::refresh_window(handle());
			}

			void button::image_enable(state sta, bool eb)
			{
				internal_scope_guard isg;
				auto p = get_drawer_trigger().ref_bgimage();
				if(p && p->enable(sta, eb))
					API::refresh_window(handle());
			}

			void button::image_valid_area(nana::arrange arg, const nana::rectangle& r)
			{
				internal_scope_guard isg;
				auto p = get_drawer_trigger().ref_bgimage();
				if(p)
				{
					p->set_valid_area(arg, r);
					p->update_blocks();
					API::refresh_window(handle());
				}
			}

			void button::image_join(state target, state from)
			{
				internal_scope_guard isg;
				auto p = get_drawer_trigger().ref_bgimage();
				if(p && p->join(target, from))
					API::refresh_window(handle());
			}

			void button::image_stretch(nana::arrange arg, int beg, int end)
			{
				internal_scope_guard isg;
				auto p = get_drawer_trigger().ref_bgimage();
				if(p)
				{
					p->set_stretch(arg, beg, end);
					API::refresh_window(handle());
				}
			}

			void button::enable_pushed(bool eb)
			{
				internal_scope_guard isg;
				if(get_drawer_trigger().enable_pushed(eb))
					API::refresh_window(handle());
			}

			bool button::pushed() const
			{
				return get_drawer_trigger().pushed();
			}

			void button::pushed(bool psd)
			{
				internal_scope_guard isg;
				if(get_drawer_trigger().pushed(psd))
					API::refresh_window(handle());
			}

			void button::omitted(bool om)
			{
				internal_scope_guard isg;
				get_drawer_trigger().omitted(om);
				API::refresh_window(handle());
			}

			void button::enable_focus_color(bool eb)
			{
				internal_scope_guard isg;
				if(get_drawer_trigger().focus_color(eb))
					API::refresh_window(handle());
			}

			void button::_m_shortkey()
			{
				eventinfo ei;
				API::raise_event<events::click>(handle(), ei);
			}

			void button::_m_complete_creation()
			{
				make_event<events::shortkey>(*this, &button::_m_shortkey);
			}

			void button::_m_caption(const nana::string& text)
			{
				API::unregister_shortkey(handle());

				nana::char_t shortkey;
				API::transform_shortkey_text(text, shortkey, 0);
				if(shortkey)
					API::register_shortkey(handle(), shortkey);

				base_type::_m_caption(text);
			}
		//end class button
}//end namespace gui
}//end namespace nana

