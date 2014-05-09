/*
 *	Window Layout Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/window_layout.hpp
 *
 */

#ifndef NANA_GUI_DETAIL_WINDOW_LAYOUT_HPP
#define NANA_GUI_DETAIL_WINDOW_LAYOUT_HPP

#include <nana/gui/basis.hpp>

#include "native_window_interface.hpp"
#include "basic_window.hpp"
#include "../layout_utility.hpp"

namespace nana{	namespace gui{
namespace detail
{

	//class window_layout
	template<typename CoreWindow>
	class window_layout
	{
	public:
		typedef CoreWindow	core_window_t;

		struct wd_rectangle
		{
			core_window_t * window;
			rectangle r;
		};
	public:
		static void paint(core_window_t* wd, bool is_redraw, bool is_child_refreshed)
		{
			if(0 == wd->effect.bground)
			{
				if(is_redraw)
				{
					if(wd->flags.refreshing)	return;

					wd->flags.refreshing = true;
					wd->drawer.refresh();
					wd->flags.refreshing = false;
				}
				maproot(wd, is_child_refreshed);
			}
			else
				_m_paint_glass_window(wd, is_redraw, is_child_refreshed, false);
		}

		static bool maproot(core_window_t* wd, bool is_child_refreshed)
		{
			nana::rectangle vr;
			if(read_visual_rectangle(wd, vr))
			{
				//get the root graphics
				nana::paint::graphics& root_graph = *(wd->root_graph);

				if(wd->other.category != static_cast<category::flags::t>(category::lite_widget_tag::value))
					root_graph.bitblt(vr, wd->drawer.graphics, nana::point(vr.x - wd->pos_root.x, vr.y - wd->pos_root.y));

				_m_paste_children(wd, is_child_refreshed, vr, root_graph, nana::point());

				if(wd->parent)
				{
					std::vector<wd_rectangle>	blocks;
					blocks.reserve(10);
					if(read_overlaps(wd, vr, blocks))
					{
						nana::point p_src;
						typename std::vector<wd_rectangle>::iterator i = blocks.begin(), end = blocks.end();
						for(; i != end; ++i)
						{
							core_window_t* ov_wd = i->window;
							const nana::rectangle& r = i->r;

							if(ov_wd->other.category == static_cast<category::flags::t>(category::frame_tag::value))
							{
								native_window_type container = ov_wd->other.attribute.frame->container;
								native_interface::refresh_window(container);
								root_graph.bitblt(r, container);
							}
							else
							{
								p_src.x = r.x - ov_wd->pos_root.x;
								p_src.y = r.y - ov_wd->pos_root.y;
								root_graph.bitblt(r, (ov_wd->drawer.graphics), p_src);
							}

							_m_paste_children(ov_wd, is_child_refreshed, r, root_graph, nana::point());
						}
					}
				}
				_m_notify_glasses(wd, vr);
				return true;
			}
			return false;
		}

		static void paste_children_to_graphics(core_window_t* wd, nana::paint::graphics& graph)
		{
			_m_paste_children(wd, false, rectangle(wd->pos_root, wd->dimension), graph, wd->pos_root);
		}

		//read_visual_rectangle
		//@brief:	Reads the visual rectangle of a window, the visual rectangle's reference frame is to root widget,
		//			the visual rectangle is a rectangular block that a window should be displayed on screen.
		//			The result is a rectangle that is a visible area for its ancesters.
		static bool read_visual_rectangle(core_window_t* wd, nana::rectangle& visual)
		{
			if(false == wd->visible)	return false;

			visual = rectangle(wd->pos_root, wd->dimension);

			if(wd->root_widget != wd)
			{
				//Test if the root widget is overlapped the specified widget
				//the pos of root widget is (0, 0)
				if(nana::gui::overlap(visual, rectangle(wd->root_widget->pos_root, wd->root_widget->dimension)) == false)
					return false;
			}

			for(const core_window_t* parent = wd->parent; parent; parent = parent->parent)
			{
				nana::rectangle self_rect = visual;
				nana::gui::overlap(rectangle(parent->pos_root, parent->dimension), self_rect, visual);
			}

			return true;
		}

		//read_overlaps
		//	reads the overlaps that are overlapped a rectangular block
		static bool read_overlaps(core_window_t* wd, const nana::rectangle& vis_rect, std::vector<wd_rectangle>& blocks)
		{
			wd_rectangle block;
			while(wd->parent)
			{
				//It should be checked that whether the window is still a chlid of its parent.
				if(wd->parent->children.size())
				{
					typename core_window_t::container::value_type *end = &(wd->parent->children[0]) + wd->parent->children.size();
					typename core_window_t::container::value_type *i = std::find(&(wd->parent->children[0]), end, wd);

					if(i != end)
					{
						//move to the widget that next to wd
						for(++i; i != end; ++i)
						{
							core_window_t* cover = *i;
							if(cover->visible && (0 == cover->effect.bground))
							{
								if(overlap(vis_rect, rectangle(cover->pos_root, cover->dimension), block.r))
								{
									block.window = cover;
									blocks.push_back(block);
								}
							}
						}
					}
				}
				wd = wd->parent;
			}

			return (blocks.size() != 0);
		}

		static bool enable_effects_bground(core_window_t * wd, bool enabled)
		{
			if(wd->other.category != static_cast<category::flags::t>(category::widget_tag::value))
				return false;

			if(false == enabled)
			{
				delete wd->effect.bground;
				wd->effect.bground = 0;
				wd->effect.bground_fade_rate = 0;
			}

			//Find the window whether it is registered for the bground effects
			typename std::vector<core_window_t*>::iterator i = std::find(data_sect.effects_bground_windows.begin(),
																	data_sect.effects_bground_windows.end(),
																	wd);

			if(i != data_sect.effects_bground_windows.end())
			{
				//If it has already registered, do nothing.
				if(enabled)
					return false;

				//Disable the effect.
				data_sect.effects_bground_windows.erase(i);
				wd->other.glass_buffer.release();
				return true;
			}
			
			//No such effect has registered.
			if(false == enabled)
				return false;

			//Enable the effect.
			data_sect.effects_bground_windows.push_back(wd);
			wd->other.glass_buffer.make(wd->dimension.width, wd->dimension.height);
			make_bground(wd);
			return true;
		}

		//make_bground
		//		update the glass buffer of a glass window
		static void make_bground(core_window_t* wd)
		{
			nana::point rpos(wd->pos_root);
			nana::paint::graphics & glass_buffer = wd->other.glass_buffer;

			if(wd->parent->other.category == static_cast<category::flags::t>(category::lite_widget_tag::value))
			{
				std::vector<core_window_t*> layers;
				core_window_t * beg = wd->parent;
				while(beg && (beg->other.category == static_cast<category::flags::t>(category::lite_widget_tag::value)))
				{
					layers.push_back(beg);
					beg = beg->parent;
				}

				glass_buffer.bitblt(wd->dimension, beg->drawer.graphics, nana::point(wd->pos_root.x - beg->pos_root.x, wd->pos_root.y - beg->pos_root.y));

				nana::rectangle r(wd->pos_owner, wd->dimension);
				typename std::vector<core_window_t*>::reverse_iterator layers_rend = layers.rend();

				for(typename std::vector<core_window_t*>::reverse_iterator i = layers.rbegin(); i != layers_rend; ++i)
				{
					core_window_t * pre = *i;
					if(false == pre->visible)
						continue;

					core_window_t * term = ((i + 1 != layers_rend) ? *(i + 1) : wd);
					r.x = wd->pos_root.x - pre->pos_root.x;
					r.y = wd->pos_root.y - pre->pos_root.y;
					for(typename std::vector<core_window_t*>::iterator u = pre->children.begin(); u != pre->children.end(); ++u)
					{
						core_window_t * child = *u;
						if(child->index >= term->index)
							break;

						nana::rectangle ovlp;
						if(child->visible && nana::gui::overlap(r, rectangle(child->pos_owner, child->dimension), ovlp))
						{
							if(child->other.category != static_cast<category::flags::t>(category::lite_widget_tag::value))
								glass_buffer.bitblt(nana::rectangle(ovlp.x - pre->pos_owner.x, ovlp.y - pre->pos_owner.y, ovlp.width, ovlp.height), child->drawer.graphics, nana::point(ovlp.x - child->pos_owner.x, ovlp.y - child->pos_owner.y));
							ovlp.x += pre->pos_root.x;
							ovlp.y += pre->pos_root.y;
							_m_paste_children(child, false, ovlp, glass_buffer, rpos);
						}
					}
				}
			}
			else
				glass_buffer.bitblt(wd->dimension, wd->parent->drawer.graphics, wd->pos_owner);

			rectangle r_of_wd(wd->pos_owner, wd->dimension);
			for(typename core_window_t::container::iterator i = wd->parent->children.begin(); i != wd->parent->children.end(); ++i)
			{
				core_window_t * child = *i;
				if(child->index >= wd->index)
					break;

				nana::rectangle ovlp;
				if(child->visible && overlap(r_of_wd, rectangle(child->pos_owner, child->dimension), ovlp))
				{
					if(child->other.category != static_cast<category::flags::t>(category::lite_widget_tag::value))
						glass_buffer.bitblt(nana::rectangle(ovlp.x - wd->pos_owner.x, ovlp.y - wd->pos_owner.y, ovlp.width, ovlp.height), child->drawer.graphics, nana::point(ovlp.x - child->pos_owner.x, ovlp.y - child->pos_owner.y));

					ovlp.x += wd->pos_root.x;
					ovlp.y += wd->pos_root.y;
					_m_paste_children(child, false, ovlp, glass_buffer, rpos);
				}
			}

			if(wd->effect.bground)
				wd->effect.bground->take_effect(reinterpret_cast<window>(wd), glass_buffer);
		}
	private:

		//_m_paste_children
		//@brief:paste children window to the root graphics directly. just paste the visual rectangle
		static void _m_paste_children(core_window_t* wd, bool is_child_refreshed, const nana::rectangle& parent_rect, nana::paint::graphics& graph, const nana::point& graph_rpos)
		{
			nana::rectangle rect;
			nana::rectangle child_rect;

			for(typename core_window_t::container::iterator i = wd->children.begin(), end = wd->children.end(); i != end; ++i)
			{
				core_window_t * child = *i;

				//it will not past children if no drawer and visible is false.
				if((false == child->visible) || (child->drawer.graphics.empty() && (child->other.category != static_cast<category::flags::t>(category::lite_widget_tag::value))))
					continue;

				if(0 == child->effect.bground)
				{
					child_rect = child->pos_root;
					child_rect = child->dimension;

					if(nana::gui::overlap(child_rect, parent_rect, rect))
					{
						if(child->other.category != static_cast<category::flags::t>(category::lite_widget_tag::value))
						{
							if(is_child_refreshed && (false == child->flags.refreshing))
								paint(child, true, true);

							graph.bitblt(nana::rectangle(rect.x - graph_rpos.x, rect.y - graph_rpos.y, rect.width, rect.height),
										child->drawer.graphics,
										nana::point(rect.x - child->pos_root.x, rect.y - child->pos_root.y));
						}

						_m_paste_children(child, is_child_refreshed, rect, graph, graph_rpos);
					}
				}
				else
					_m_paint_glass_window(child, false, is_child_refreshed, false);
			}
		}

		static void _m_paint_glass_window(core_window_t* wd, bool is_redraw, bool is_child_refreshed, bool called_by_notify)
		{
			if(wd->flags.refreshing && is_redraw)	return;

			nana::rectangle vr;
			if(read_visual_rectangle(wd, vr))
			{
				if(is_redraw || called_by_notify)
				{
					if(called_by_notify)
						make_bground(wd);
					wd->flags.refreshing = true;
					wd->other.glass_buffer.paste(wd->drawer.graphics, 0, 0);
					wd->drawer.refresh();
					wd->flags.refreshing = false;
				}

				nana::paint::graphics& root_graph = *(wd->root_graph);

				//Map Root
				root_graph.bitblt(vr, wd->drawer.graphics, nana::point(vr.x - wd->pos_root.x, vr.y - wd->pos_root.y));
				_m_paste_children(wd, is_child_refreshed, vr, root_graph, nana::point());

				if(wd->parent)
				{
					std::vector<wd_rectangle>	blocks;
					read_overlaps(wd, vr, blocks);
					typename std::vector<wd_rectangle>::iterator i = blocks.begin(), end = blocks.end();
					for(; i != end; ++i)
					{
						wd_rectangle & wr = *i;
						root_graph.bitblt(wr.r, (wr.window->drawer.graphics), nana::point(wr.r.x - wr.window->pos_root.x, wr.r.y - wr.window->pos_root.y));
					}
				}
				_m_notify_glasses(wd, vr);
			}
		}

		//_m_notify_glasses
		//@brief:	Notify the glass windows that are overlapped with the specified vis_rect
		static void _m_notify_glasses(core_window_t* const sigwd, const nana::rectangle& r_visual)
		{
			typedef gui::category::flags cat_flags;

			nana::rectangle r_of_sigwd(sigwd->pos_root.x, sigwd->pos_root.y, sigwd->dimension.width, sigwd->dimension.height);
			for(typename std::vector<core_window_t*>::iterator i = data_sect.effects_bground_windows.begin(), end = data_sect.effects_bground_windows.end();
				i != end; ++i)
			{
				core_window_t* x = *i;
				if(	x == sigwd || !x->visible ||
					(false == nana::gui::overlap(nana::rectangle(x->pos_root, x->dimension), r_of_sigwd)))
					continue;

				//Test a parent of the glass window is invisible.
				for(core_window_t *p = x->parent; p; p = p->parent)
				{
					if(false == p->visible)
						continue;
				}

				if(sigwd->parent == x->parent)
				{
					if(sigwd->index < x->index)
						_m_paint_glass_window(x, true, false, true);
				}
				else if(sigwd == x->parent)
				{
					_m_paint_glass_window(x, true, false, true);
				}
				else if (x->parent && (cat_flags::lite_widget == x->parent->other.category))
				{
					//Test if sigwd is an ancestor of the glass window, and there are lite widgets
					//between sigwd and glass window.
					core_window_t * ancestor = x->parent->parent;
					while (ancestor && (ancestor != sigwd) && (cat_flags::lite_widget == ancestor->other.category))
						ancestor = ancestor->parent;

					if ((ancestor == sigwd) && (cat_flags::lite_widget != ancestor->other.category))
						_m_paint_glass_window(x, true, false, true);
				}
				else
				{
					//test if sigwnd is a parent of glass window x, or a slibing of the glass window, or a child of the slibing of the glass window.
					core_window_t *p = x->parent, *signode = sigwd;
					while(signode->parent && (signode->parent != p))
						signode = signode->parent;

					if(signode->parent && (signode->index < x->index))// || (signode->other.category != category::widget_tag::value))
						_m_paint_glass_window(x, true, false, true);
				}
			}
		}
	private:
		struct data_section
		{
			std::vector<core_window_t*> 	effects_bground_windows;
		};
		static data_section	data_sect;
	};//end class window_layout

	template<typename CoreWindow>
	typename window_layout<CoreWindow>::data_section window_layout<CoreWindow>::data_sect;
}//end namespace detail
}//end namespace gui
}//end namespace nana

#endif //NANA_GUI_DETAIL_WINDOW_LAYOUT_HPP

