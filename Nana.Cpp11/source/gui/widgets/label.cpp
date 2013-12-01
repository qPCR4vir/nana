/*
 *	A Label Control Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: source/gui/widgets/label.cpp
 */

#include <nana/gui/widgets/label.hpp>
#include <nana/system/platform.hpp>
#include <nana/unicode_bidi.hpp>
#include <nana/paint/text_renderer.hpp>
#include <nana/gui/widgets/skeletons/text_token_stream.hpp>
#include <stdexcept>
#include <sstream>
#include <numeric>	//accumulate

namespace nana
{
namespace gui
{
	namespace drawerbase
	{
		namespace label
		{
			class renderer
			{
				typedef gui::widgets::skeletons::dstream::linecontainer::iterator iterator;

				struct pixel_tag
				{
					int x_base;				//The x position where this line starts.
					std::size_t pixels;
					std::size_t baseline;	//The baseline for drawing text.
					std::vector<iterator> values;	//line values
				};

				//this is a helper variable, it just keeps the status while drawing.
				struct render_status
				{
					unsigned allowed_width;
					align text_align;
					align_v text_align_v;
					
					nana::point pos;
					std::vector<pixel_tag> pixels;
					std::size_t index;
				};

				struct traceable
				{
					nana::rectangle r;
					nana::string id;
				};

				struct calc_metrics
				{
					unsigned total_width;
					unsigned processed_width;
					unsigned max_ascent;
					unsigned max_descent;
					unsigned max_px;
				};
			public:
				typedef nana::paint::graphics& graph_reference;
				typedef gui::widgets::skeletons::dstream dstream;
				typedef gui::widgets::skeletons::fblock fblock;
				typedef gui::widgets::skeletons::data data;

				renderer()
					: fblock_(0), format_enabled_(false)
				{
				}
				
				void parse(const nana::string& s)
				{
					dstream_.parse(s, format_enabled_);
				}

				bool format(bool fm)
				{
					if(fm != format_enabled_)
					{
						format_enabled_ = fm;
						return true;
					}
					return false;
				}
				
				void render(graph_reference graph, nana::color_t fgcolor, align th, align_v tv)
				{
					traceable_.clear();

					nana::paint::font ft = graph.typeface();	//used for restoring the font
					font_ = ft;
					fblock_ = 0;
					
					_m_set_default(ft, fgcolor);


					_m_measure(graph);

					render_status rs;
			
					rs.allowed_width = graph.size().width;
					rs.text_align = th;
					rs.text_align_v = tv;

					std::deque<std::vector<pixel_tag> > pixel_lines;

					std::size_t extent_v_pixels = 0;	//the pixels, in height, that text will be painted.

					for(dstream::iterator i = dstream_.begin(), end = dstream_.end(); i != end; ++i)
					{
						rs.pixels.clear();
						_m_line_pixels(*i, rs);

						for(std::vector<pixel_tag>::iterator u = rs.pixels.begin(); u != rs.pixels.end(); ++u)
							extent_v_pixels += u->pixels;

						std::vector<pixel_tag> tmp;
						pixel_lines.push_back(tmp);
						pixel_lines.back().swap(rs.pixels);

						if(extent_v_pixels >= graph.height())
							break;
					}

					if((tv != align_v::top) && extent_v_pixels < graph.height())
					{
						if(align_v::center == tv)
							rs.pos.y = (graph.height() - extent_v_pixels) >> 1;
						else if(align_v::bottom == tv)
							rs.pos.y = graph.height() - extent_v_pixels;
					}
					else
						rs.pos.y = 0;

					std::deque<std::vector<pixel_tag> >::iterator pixels_iterator = pixel_lines.begin();

					//draw_function df(*this, rs, graph);
					for(dstream::iterator i = dstream_.begin(), end = dstream_.end(); i != end; ++i)
					{
						rs.index = 0;
						rs.pixels.clear();

						dstream::linecontainer & line = *i;

						//_m_line_pixels(line, rs);
						rs.pixels.swap(*pixels_iterator++);

						rs.pos.x = rs.pixels.front().x_base;
						
						//Stop drawing when it goes out of range.
						if(false == _m_each_line(graph, line, rs))
							break;

						rs.pos.y += rs.pixels.back().pixels;
						if(rs.pos.y >= static_cast<int>(graph.height()))
							break;
					}

					graph.typeface(ft);
				}

				bool find(int x, int y, nana::string& id) const
				{
					for(std::deque<traceable>::const_iterator i = traceable_.begin(); i != traceable_.end(); ++i)
					{
						if(i->r.is_hit(x, y))
						{
							id = i->id;
							return true;
						}
					}

					return false;
				}

				nana::size measure(graph_reference graph, unsigned limited, align th, align_v tv)
				{
					nana::size retsize;

					nana::paint::font ft = graph.typeface();	//used for restoring the font
					font_ = ft;
					fblock_ = 0;
					
					_m_set_default(ft, 0);
					_m_measure(graph);

					render_status rs;
					
					rs.allowed_width = limited;
					rs.text_align = th;
					rs.text_align_v = tv;

					for(dstream::iterator i = dstream_.begin(), end = dstream_.end(); i != end; ++i)
					{
						rs.pixels.clear();
						unsigned w = _m_line_pixels(*i, rs);
						if(limited && (w > limited))
							w = limited;
						if(retsize.width < w)
							retsize.width = w;

						for(std::vector<pixel_tag>::iterator u = rs.pixels.begin(); u != rs.pixels.end(); ++u)
							retsize.height += u->pixels;
					}

					return retsize;
				}
			private:
				//Manage the fblock for a specified rectangle if it is a traceable fblock.
				void _m_inser_if_traceable(int x, int y, const nana::size& sz, widgets::skeletons::fblock* fbp)
				{
					if(fbp->target.size())
					{
						traceable tr;
						tr.r.x = x;
						tr.r.y = y;
						tr.r.width = sz.width;
						tr.r.height = sz.height;
						tr.id = fbp->target;

						traceable_.push_back(tr);
					}
				}

				void _m_set_default(const nana::paint::font& ft, nana::color_t fgcolor)
				{
					def_.font_name = ft.name();
					def_.font_size = ft.size();
					def_.font_bold = ft.bold();
					def_.fgcolor = fgcolor;
				}

				nana::color_t _m_fgcolor(nana::gui::widgets::skeletons::fblock* fp)
				{
					while(fp->fgcolor == 0xFFFFFFFF)
					{
						fp = fp->parent;
						if(0 == fp)
							return def_.fgcolor;
					}
					return fp->fgcolor;
				}

				std::size_t _m_font_size(nana::gui::widgets::skeletons::fblock* fp)
				{
					while(fp->font_size == 0xFFFFFFFF)
					{
						fp = fp->parent;
						if(0 == fp)
							return def_.font_size;
					}
					return fp->font_size;			
				}

				std::size_t _m_bold(nana::gui::widgets::skeletons::fblock* fp)
				{
					while(fp->bold_empty)
					{
						fp = fp->parent;
						if(0 == fp)
							return def_.font_bold;
					}
					return fp->bold;	
				}

				const nana::string& _m_fontname(nana::gui::widgets::skeletons::fblock* fp)
				{
					while(fp->font.empty())
					{
						fp = fp->parent;
						if(0 == fp)
							return def_.font_name;
					}
					return fp->font;					
				}

				void _m_change_font(graph_reference graph, nana::gui::widgets::skeletons::fblock* fp)
				{
					if(fp != fblock_)
					{
						const nana::string& name = _m_fontname(fp);
						std::size_t fontsize = _m_font_size(fp);
						bool bold = _m_bold(fp);

						if((fontsize != font_.size()) || bold != font_.bold() || name != font_.name())
						{
							font_.make(name.data(), fontsize, bold);
							graph.typeface(font_);
						}
						fblock_ = fp;
					}
				}

				void _m_measure(graph_reference graph)
				{
					nana::paint::font ft = font_;
					for(dstream::iterator i = dstream_.begin(), end = dstream_.end(); i != end; ++i)
					{
						dstream::linecontainer& line = *i;
						
						for(dstream::linecontainer::iterator u = line.begin(), uend = line.end(); u != uend; ++u)
						{
							_m_change_font(graph, u->fblock_ptr);
							u->data_ptr->measure(graph);
						}
					}
					if(font_ != ft)
					{
						font_ = ft;
						graph.typeface(ft);
						fblock_ = 0;
					}
				}

				void _m_align_x_base(const render_status& rs, pixel_tag & px, unsigned w)
				{
					switch(rs.text_align)
					{
					case align::left:
						px.x_base = 0;
						break;
					case align::center:
						px.x_base = (static_cast<int>(rs.allowed_width - w) >> 1);
						break;
					case align::right:
						px.x_base = static_cast<int>(rs.allowed_width - w);
						break;
					}				
				}

				unsigned _m_line_pixels(dstream::linecontainer& line, render_status & rs)
				{
					unsigned total_w = 0;
					unsigned w = 0;
					unsigned max_ascent = 0;
					unsigned max_descent = 0;
					unsigned max_px = 0;

					//Bidi reorder is requried here

					std::vector<iterator> line_values;

					for(dstream::linecontainer::iterator i = line.begin(), end = line.end(); i != end; ++i)
					{
						data * data_ptr = i->data_ptr;
						nana::size sz = data_ptr->size();
						total_w += sz.width;

						unsigned as = 0;	//ascent
						unsigned ds = 0;	//descent

						if(fblock::aligns::baseline == i->fblock_ptr->text_align)
						{
							as = data_ptr->ascent();
							ds = sz.height - as;

							if(max_descent < ds)
								max_descent = ds;

							if((false == data_ptr->is_text()) && (sz.height < max_ascent + max_descent))
								sz.height = max_ascent + max_descent;
						}

						if(w + sz.width <= rs.allowed_width)
						{
							w += sz.width;

							if(max_ascent < as)		max_ascent = as;
							if(max_descent < ds)	max_descent = ds;
							if(max_px < sz.height)	max_px = sz.height;
							line_values.push_back(i);
						}
						else
						{
							if(w)
							{
								pixel_tag px;

								_m_align_x_base(rs, px, w);

								if(max_ascent + max_descent > max_px)
									max_px = max_descent + max_ascent;
								else
									max_ascent = max_px - max_descent;

								px.pixels = max_px;
								px.baseline = max_ascent;
								px.values.swap(line_values);

								rs.pixels.push_back(px);
								
								w = sz.width;
								max_px = sz.height;
								max_ascent = as;
								max_descent = ds;
								line_values.push_back(i);
							}
							else
							{
								pixel_tag px;

								_m_align_x_base(rs, px, sz.width);
								px.pixels = sz.height;
								px.baseline = as;

								px.values.push_back(i);

								rs.pixels.push_back(px);
								max_px = 0;
								max_ascent = max_descent = 0;
							}
						}
					}
					pixel_tag px;

					_m_align_x_base(rs, px, w);

					if(max_ascent + max_descent > max_px)
						max_px = max_descent + max_ascent;
					else
						max_ascent = max_px - max_descent;

					px.pixels = max_px;
					px.baseline = max_ascent;
					px.values.swap(line_values);
					rs.pixels.push_back(px);
					return total_w;
				}
				
				bool _m_each_line(graph_reference graph, dstream::linecontainer& line, render_status& rs)
				{
					typedef dstream::linecontainer::iterator iterator;
					nana::string text;
					iterator block_start;

					const int lastpos = graph.height() - 1;

					for(std::vector<pixel_tag>::iterator i = rs.pixels.begin(), end = rs.pixels.end(); i != end; ++i)
					{
						std::vector<iterator> & values = i->values;
						
						for(std::vector<iterator>::iterator u = values.begin(), uend = values.end(); u != uend; ++u)
						{
							dstream::linecontainer::value_type & value = *(*u);
							if(false == value.data_ptr->is_text())
							{
								if(text.size())
								{
									_m_draw_block(graph, text, block_start, rs);
									if(lastpos <= rs.pos.y)
										return false;
									text.clear();
								}
								nana::size sz = value.data_ptr->size();

								pixel_tag px = rs.pixels[rs.index];

								//if(_m_overline(rs, rs.pos.x + sz.width, false))
								if(rs.allowed_width < rs.pos.x + sz.width)
								{
									//Change a line.
									rs.pos.y += px.pixels;
									px = rs.pixels[++rs.index];
									rs.pos.x = px.x_base;
								}

								int y = rs.pos.y + _m_text_top(px, value.fblock_ptr, value.data_ptr);

								value.data_ptr->nontext_render(graph, rs.pos.x, y);
								_m_inser_if_traceable(rs.pos.x, y, sz, value.fblock_ptr);
								rs.pos.x += static_cast<int>(sz.width);

								if(lastpos < y)
									return false;
							}
							else
							{
								//hold the block while the text is empty,
								//it stands for the first block
								if(text.empty())
									block_start = *u;

								text += value.data_ptr->text();
							}
						}

						if(text.size())
						{
							_m_draw_block(graph, text, block_start, rs);
							text.clear();
						}
					}
					return (rs.pos.y <= lastpos);
/*
					for(iterator i = line.begin(), end = line.end(); i != end; ++i)
					{
						if(false == i->data_ptr->is_text())
						{
							if(text.size())
							{
								_m_draw_block(graph, text, block_start, rs);
								if(lastpos < rs.pos.y)
									return false;

								text.clear();
							}

							nana::size sz = i->data_ptr->size();

							pixel_tag px = rs.pixels[rs.index];

							//if(_m_overline(rs, rs.pos.x + sz.width, false))
							if(rs.allowed_width < rs.pos.x + sz.width)
							{
								//Change a line.
								rs.pos.y += px.pixels;
								px = rs.pixels[++rs.index];
								rs.pos.x = px.x_base;
							}

							int y = rs.pos.y + _m_text_top(px, i->fblock_ptr, i->data_ptr);

							i->data_ptr->nontext_render(graph, rs.pos.x, y);
							_m_inser_if_traceable(rs.pos.x, y, sz, i->fblock_ptr);
							rs.pos.x += static_cast<int>(sz.width);

							if(lastpos < y)
								return false;
						}
						else
						{
							//hold the block while the text is empty,
							//it stands for the first block
							if(text.empty())
								block_start = i;

							text += i->data_ptr->text();
						}
					}
					
					if(text.size())
						_m_draw_block(graph, text, block_start, rs);

					return (lastpos < rs.pos.y);
*/
				}

				static bool _m_overline(const render_status& rs, int right, bool equal_required)
				{
					if(align::left == rs.text_align)
						return (equal_required ? right >= static_cast<int>(rs.allowed_width) : right > static_cast<int>(rs.allowed_width));

					return (equal_required ? rs.pixels[rs.index].x_base <= 0 : rs.pixels[rs.index].x_base < 0);
				}

				static unsigned _m_text_top(const pixel_tag& px, fblock* fblock_ptr, const data* data_ptr)
				{
					switch(fblock_ptr->text_align)
					{
					case fblock::aligns::center:
						return (px.pixels - data_ptr->size().height) / 2;
					case fblock::aligns::bottom:
						return (px.pixels - data_ptr->size().height);
					case fblock::aligns::baseline:
						return (px.baseline - (data_ptr->is_text() ? data_ptr->ascent() : data_ptr->size().height));
					default:	break;
					}
					return 0;
				}
				
				void _m_draw_block(graph_reference graph, const nana::string& s, dstream::linecontainer::iterator block_start, render_status& rs)
				{
					nana::unicode_bidi bidi;
					std::vector<nana::unicode_bidi::entity> reordered;
					bidi.linestr(s.data(), s.length(), reordered);
					
					pixel_tag px = rs.pixels[rs.index];

					for(std::vector<nana::unicode_bidi::entity>::iterator i = reordered.begin(); i != reordered.end(); ++i)
					{
						std::size_t pos = i->begin - s.data();
						std::size_t len = i->end - i->begin;

						while(true)
						{
							dstream::linecontainer::iterator u = block_start;

							const std::size_t off = _m_locate(u, pos);
							
							fblock * fblock_ptr = u->fblock_ptr;
							data * data_ptr = u->data_ptr;
							
							const int w = static_cast<int>(rs.allowed_width) - rs.pos.x;
							nana::size sz = data_ptr->size();
							if((static_cast<int>(sz.width) > w) && (rs.pos.x != px.x_base))
							{
								//Change a new line
								rs.pos.y += px.pixels;
								px = rs.pixels[++rs.index];
								rs.pos.x = px.x_base;
							}
							
							const int y = rs.pos.y + _m_text_top(px, fblock_ptr, data_ptr);

							_m_change_font(graph, fblock_ptr);
							graph.string(rs.pos.x, y, _m_fgcolor(fblock_ptr), data_ptr->text());
							_m_inser_if_traceable(rs.pos.x, y, sz, fblock_ptr);

							rs.pos.x += static_cast<int>(sz.width);

							if(off < len)
							{
								len -= off;
								pos += off;
							}
							else
								break;
						}
					}
				}
				
				std::size_t _m_locate(dstream::linecontainer::iterator& i, std::size_t pos)
				{
					std::size_t n = i->data_ptr->text().length();
					while(pos >= n)
					{
						pos -= n;
						n = (++i)->data_ptr->text().length();
					}
					
					return (n - pos);
				}
			private:
				dstream dstream_;
				bool format_enabled_;
				nana::paint::font font_;
				struct def_font_tag
				{
					nana::string font_name;
					std::size_t font_size;
					bool	font_bold;
					nana::color_t fgcolor;
				}def_;
				nana::gui::widgets::skeletons::fblock * fblock_;
				std::deque<traceable> traceable_;
			};

			//class trigger
			//@brief: Draw the label
				struct trigger::impl_t
				{
					nana::gui::widget * wd;
					nana::paint::graphics * graph;

					align	text_align;
					align_v	text_align_v;
					
					class renderer renderer;

					nana::string target;	//It indicates which target is tracing.

					impl_t()
						:	wd(0), graph(0), text_align(align::left),
                            format_state_(false)
					{
						//renderer = new simple_renderer;
					}

					~impl_t()
					{
					}

					bool format(bool enabled)
					{
						if((enabled != format_state_) && wd)
						{
							/*
							internal_scope_guard isg;
							renderer_interface * rnd_if = renderer;
							if(enabled)
							{
								renderer = new format_renderer;
								renderer->bind_listener(nana::make_fun(*this, &impl_t::_m_call_listener));
							}
							else
								renderer = new simple_renderer;
							format_state_ = enabled;
							API::refresh_window(wd->handle());
							delete rnd_if;
							return true;
							*/
						}
						return false;
					}

					void add_listener(const std::function<void(command, const nana::string&)> & f)
					{
						listener_ += f;
					}

					void call_listener(command cmd, const nana::string& tar)
					{
						listener_(cmd, tar);
					}
				private:
					bool format_state_;
					nana::fn_group<void(command, const nana::string&)> listener_;
				};

				trigger::trigger()
					:impl_(new impl_t)
				{}

				trigger::~trigger()
				{
					delete impl_;
				}

				void trigger::bind_window(widget_reference w)
				{
					impl_->wd = &w;
				}

				trigger::impl_t * trigger::impl() const
				{
					return impl_;
				}

				void trigger::attached(graph_reference graph)
				{
					impl_->graph = &graph;
					window wd = impl_->wd->handle();
					API::dev::make_drawer_event<events::mouse_move>(wd);
					API::dev::make_drawer_event<events::mouse_leave>(wd);
					API::dev::make_drawer_event<events::click>(wd);
				}

				void trigger::detached()
				{
					API::dev::umake_drawer_event(impl_->wd->handle());
				}

				void trigger::mouse_move(graph_reference, const eventinfo& ei)
				{
					nana::string id;

					if(impl_->renderer.find(ei.mouse.x, ei.mouse.y, id))
					{
						if(id != impl_->target)
						{
							int cur_state = 0;
							if(impl_->target.size())
							{
								impl_->call_listener(command::leave, impl_->target);
								cur_state = 1;	//Set arrow
							}

							impl_->target = id;

							if(id.size())
							{
								impl_->call_listener(command::enter, impl_->target);
								cur_state = 2;	//Set hand
							}

							if(cur_state)
								impl_->wd->cursor(1 == cur_state ? gui::cursor::arrow : gui::cursor::hand);
						}
					}
					else if(impl_->target.size())
					{
						impl_->call_listener(command::leave, impl_->target);
						impl_->target.clear();
						impl_->wd->cursor(gui::cursor::arrow);
					}
				}

				void trigger::mouse_leave(graph_reference, const eventinfo& ei)
				{
					if(impl_->target.size())
					{
						impl_->call_listener(command::leave, impl_->target);
						impl_->target.clear();
						impl_->wd->cursor(gui::cursor::arrow);
					}
				}

				void trigger::click(graph_reference, const eventinfo&)
				{
					if(impl_->target.size())
						impl_->call_listener(command::click, impl_->target);
				}

				void trigger::refresh(graph_reference graph)
				{
					if(0 == impl_->wd) return;

					nana::gui::window wd = impl_->wd->handle();
					if(bground_mode::basic != API::effects_bground_mode(wd))
						graph.rectangle(API::background(wd), true);

					impl_->renderer.render(graph, impl_->wd->foreground(), impl_->text_align, impl_->text_align_v);
				}

			//end class label_drawer
		}//end namespace label
	}//end namespace drawerbase


	//
	//class label
		label::label(){}

		label::label(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		label::label(window wd, const nana::string& text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);
		}

		label::label(window wd, const nana::char_t* text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);
		}

		label::label(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void label::transparent(bool enabled)
		{
			if(enabled)
				API::effects_bground(*this, effects::bground_transparent(0), 0.0);
			else
				API::effects_bground_remove(*this);
		}

		bool label::transparent() const
		{
			return (bground_mode::basic == API::effects_bground_mode(*this));
		}

		void label::format(bool f)
		{
			drawerbase::label::trigger::impl_t * impl = get_drawer_trigger().impl();

			if(impl->renderer.format(f))
			{
				window wd = *this;
				impl->renderer.parse(API::dev::window_caption(wd));
				API::refresh_window(wd);
			}
		}

		void label::add_format_listener(const std::function<void(command, const nana::string&)> & f)
		{
			get_drawer_trigger().impl()->add_listener(f);
		}

		nana::size label::measure(unsigned limited) const
		{
			if(this->empty())	return nana::size();
			drawerbase::label::trigger::impl_t * impl = get_drawer_trigger().impl();
			return impl->renderer.measure(*impl->graph, limited, impl->text_align, impl->text_align_v);
		}

		void label::text_align(align th, align_v tv)
		{
			internal_scope_guard isg;
			drawerbase::label::trigger::impl_t* impl = get_drawer_trigger().impl();

			bool to_update = false;
			if(impl->text_align != th)
			{
				impl->text_align = th;
				to_update = true;
			}

			if(impl->text_align_v != tv)
			{
				impl->text_align_v = tv;
				to_update = true;
			}

			if(to_update)
				API::refresh_window(*this);
		}

		void label::_m_caption(const nana::string& s)
		{
			internal_scope_guard isg;
			window wd = *this;
			get_drawer_trigger().impl()->renderer.parse(s);
			API::dev::window_caption(wd, s);
			API::refresh_window(wd);
		}
	//end class label
}//end namespace gui
}//end namespace nana
