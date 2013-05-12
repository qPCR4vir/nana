/*
 *	A Treebox Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/treebox.cpp
 */
#include <nana/gui/widgets/treebox.hpp>
#include <nana/system/platform.hpp>
#include <stdexcept>

namespace nana
{
namespace gui
{
	namespace drawerbase
	{
		namespace treebox
		{
			class tlwnd_drawer
				: public drawer_trigger
			{
			public:
				void text(const nana::string& text, int text_off, bool selected, const nana::paint::image* img)
				{
					text_off_ = text_off;
					selected_ = selected;
					text_ = text;
					draw(img);
				}

				void draw(const nana::paint::image* img)
				{
					nana::size sz = graph_->size();

					int left = 0;
					int right = left + sz.width - 1;
					int top = 0;
					int bottom = top + sz.height - 1;

					graph_->rectangle((selected_ ? 0xC4E8FA : 0xE8F5FD), true);

					const unsigned colorx = (selected_ ? 0xB6E6FB : 0xD8F0FA);
					graph_->line(left + 1, top, right - 1, top, colorx);
					graph_->line(left + 1, bottom, right - 1, bottom, colorx);

					graph_->line(left, top + 1, left, bottom - 1, colorx);
					graph_->line(right, top + 1, right, bottom - 1, colorx);

					graph_->string(text_off_, 3, 0x0, text_);

					if(img) img->paste(*graph_, 2, 2);
				}
			private:
				void bind_window(widget_reference wd)
				{
					widget_ = &wd;
				}

				void attached(graph_reference graph)
				{
					graph_ = &graph;
				}
			private:
				nana::paint::graphics * graph_;
				widget	*widget_;
				int text_off_;
				bool selected_;
				nana::string	text_;
			};//end class tlwnd_drawer

			class tooltip_window
				: public widget_object<category::root_tag, tlwnd_drawer>
			{
			public:
				tooltip_window(window wd, const nana::point& pos, const nana::size& size)
					: widget_object<category::root_tag, tlwnd_drawer>(wd, false, rectangle(pos, size), appear::bald<appear::floating>())
				{
					API::take_active(handle(), false, nullptr);
				}

				void show_text(const nana::string& text, int text_off, bool selected, nana::paint::image* img)
				{
					get_drawer_trigger().text(text, text_off, selected, img);
					show();
				}
			};//end class tooltip_window

			//class trigger
				//struct treebox_node_type
					trigger::treebox_node_type::treebox_node_type()
						:expanded(false)
					{}

					trigger::treebox_node_type::treebox_node_type(const nana::any& v)
						:value(v), expanded(false)
					{}

					trigger::treebox_node_type::treebox_node_type(const nana::string& text, const nana::any& v)
						:text(text), value(v), expanded(false)
					{}

					trigger::treebox_node_type& trigger::treebox_node_type::operator=(const trigger::treebox_node_type& rhs)
					{
						if(this != &rhs)
						{
							text = rhs.text;
							value = rhs.value;
							img_idstr = rhs.img_idstr;
						}
						return *this;
					}
				//end struct treebox_node_type

				trigger::trigger()
					:graph_(nullptr), widget_(nullptr)
				{
					adjust_.timer.enable(false);
					adjust_.timer.make_tick(std::bind(&trigger::_m_deal_adjust, this));
					adjust_.timer.interval(10);
				}

				void trigger::auto_draw(bool ad)
				{
					if(attr_.auto_draw != ad)
					{
						attr_.auto_draw = ad;
						if(ad)
							API::update_window(widget_->handle());
					}
				}

				bool trigger::check(const node_type* node) const
				{
					return attr_.tree_cont.check(node);
				}

				nana::any & trigger::value(node_type* node) const
				{
					if(attr_.tree_cont.check(node) == false)
						throw std::invalid_argument("Nana.GUI.treebox.value() invalid node");

					return node->value.second.value;
				}

				trigger::node_type* trigger::find(const nana::string& key_path)
				{
					return attr_.tree_cont.find(key_path);
				}

				trigger::node_type* trigger::get_owner(const node_type* node) const
				{
					return attr_.tree_cont.get_owner(node);
				}

				trigger::node_type* trigger::get_root() const
				{
					return attr_.tree_cont.get_root();
				}

				trigger::node_type* trigger::insert(node_type* node, const nana::string& key, const nana::string& title, const nana::any& v)
				{
					node_type * p = attr_.tree_cont.node(node, key);
					if(p)
					{
						p->value.second.text = title;
						p->value.second.value = v;
					}
					else
						p = attr_.tree_cont.insert(node, key, treebox_node_type(title, v));

					if(p && attr_.auto_draw && _m_draw(true))
						API::update_window(widget_->handle());
					return p;
				}

				trigger::node_type* trigger::insert(const nana::string& path, const nana::string& title, const nana::any& v)
				{
					auto x = attr_.tree_cont.insert(path, treebox_node_type(title, v));
					if(x && attr_.auto_draw && _m_draw(true))
						API::update_window(widget_->handle());
					return x;
				}

				bool trigger::check(node_type* parent, node_type* child) const
				{
					if(nullptr == parent || nullptr == child) return false;

					while(child && (child != parent))
						child = child->owner;

					return (nullptr != child);
				}

				void trigger::remove(node_type* node)
				{
					if(check(node, node_state_.event_node))
						node_state_.event_node = nullptr;

					if(check(node, node_desc_.first))
						node_desc_.first = nullptr;

					if(check(node, node_state_.selected))
						node_state_.selected = nullptr;

					attr_.tree_cont.remove(node);
				}

				trigger::node_type* trigger::selected() const
				{
					return node_state_.selected;
				}

				void trigger::selected(node_type* node)
				{
					if(attr_.tree_cont.check(node) && _m_set_selected(node))
					{
						_m_draw(true);
						API::update_window(*widget_);
					}
				}

				void trigger::set_expand(node_type* node, bool exp)
				{
					if(widget_ && _m_set_expanded(node, exp))
					{
						_m_draw(true);
						API::update_window(widget_->handle());
					}
				}

				void trigger::set_expand(const nana::string& path, bool exp)
				{
					if(_m_set_expanded(attr_.tree_cont.find(path), exp))
					{
						_m_draw(true);
						API::update_window(widget_->handle());
					}
				}

				unsigned trigger::visual_item_size() const
				{
					return attr_.tree_cont.child_size_if(STR(""), pred_allow_child());
				}

				void trigger::image(const nana::string& id, const node_image_tag& img)
				{
					shape_.image_table[id] = img;
				}

				node_image_tag& trigger::image(const nana::string& id) const
				{
					auto i = shape_.image_table.find(id);
					if(i != shape_.image_table.end())
						return i->second;
					throw std::invalid_argument("Nana.GUI.treebox.image() invalid image identifier");
				}

				void trigger::image_erase(const nana::string& id)
				{
					shape_.image_table.erase(id);
				}

				void trigger::node_image(node_type* node, const nana::string& id)
				{
					if(check(node))
					{
						node->value.second.img_idstr = id;
						auto i = shape_.image_table.find(id);
						if((i != shape_.image_table.end()) && _m_draw(true))
								API::update_window(widget_->handle());
					}
				}

				unsigned long trigger::node_width(const node_type *node) const
				{
					return (static_cast<int>(graph_->text_extent_size(node->value.second.text).width) + node_desc_.text_offset * 2 + node_desc_.image_width);
				}

				bool trigger::rename(node_type *node, const nana::char_t* key, const nana::char_t* name)
				{
					if((key || name ) && this->check(node))
					{
						if(key && (key != node->value.first))
						{
							node_type * element = node->owner->child;
							for(; element; element = element->next)
							{
								if((element->value.first == key) && (node != element))
									return false;
							}
							node->value.first = key;
						}

						if(name)
							node->value.second.text = name;

						return (key || name);
					}
					return false;

				}

				trigger::ext_event_type& trigger::ext_event() const
				{
					return attr_.ext_event;
				}

				void trigger::bind_window(widget_reference widget)
				{
					widget.background(0xFFFFFF);
					widget_ = &widget;
					widget.caption(STR("Nana Treebox"));
				}

				void trigger::attached(graph_reference graph)
				{
					graph_ = &graph;
					window wd = widget_->handle();
					using namespace API::dev;
					make_drawer_event<events::dbl_click>(wd);
					make_drawer_event<events::mouse_move>(wd);
					make_drawer_event<events::mouse_down>(wd);
					make_drawer_event<events::mouse_up>(wd);
					make_drawer_event<events::mouse_wheel>(wd);
					make_drawer_event<events::size>(wd);
					make_drawer_event<events::key_down>(wd);
					make_drawer_event<events::key_char>(wd);
				}

				void trigger::detached()
				{
					API::dev::umake_drawer_event(widget_->handle());
				}

				void trigger::refresh(graph_reference)
				{
					_m_draw(false);
				}

				void trigger::dbl_click(graph_reference, const eventinfo& ei)
				{
					int xpos = attr_.tree_cont.indent_size(node_desc_.first) * node_desc_.indent_size - node_desc_.offset_x;
					item_locator nl(*this, xpos, ei.mouse.x, ei.mouse.y);
					attr_.tree_cont.for_each<item_locator&>(node_desc_.first, nl);

					if(nl.node() && (nl.what() == item_locator::object::item))
					{
						node_state_.event_node = nl.node();
						_m_set_expanded(node_state_.event_node, !node_state_.event_node->value.second.expanded);
						_m_draw(true);
						API::lazy_refresh();
					}
				}

				void trigger::mouse_down(graph_reference graph, const eventinfo& ei)
				{
					int xpos = attr_.tree_cont.indent_size(node_desc_.first) * node_desc_.indent_size - node_desc_.offset_x;
					item_locator nl(*this, xpos, ei.mouse.x, ei.mouse.y);
					attr_.tree_cont.for_each<item_locator&>(node_desc_.first, nl);

					bool has_redraw = false;

					node_state_.event_node = 0;

					if(nl.node())
					{
						node_state_.event_node = nl.node();
						if(nl.what() != item_locator::object::none)
						{
							switch(nl.what())
							{
							case item_locator::object::arrow:
								if(_m_set_expanded(node_state_.event_node, !node_state_.event_node->value.second.expanded))
									_m_adjust(node_state_.event_node, 0);

								has_redraw = true;
								break;
							case item_locator::object::item:
								if(node_state_.selected != node_state_.event_node)
								{
									_m_set_selected(node_state_.event_node);
									has_redraw = true;
								}
								break;
							}
						}
						else if(node_state_.selected != node_state_.event_node)
						{
							_m_set_selected(node_state_.event_node);
							has_redraw = true;
						}
					}

					if(has_redraw)
					{
						_m_draw(true);
						API::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference, const eventinfo& ei)
				{
					int xpos = attr_.tree_cont.indent_size(node_desc_.first) * node_desc_.indent_size - node_desc_.offset_x;
					item_locator nl(*this, xpos, ei.mouse.x, ei.mouse.y);
					attr_.tree_cont.for_each<item_locator&>(node_desc_.first, nl);

					if(nl.node() && (node_state_.selected != nl.node()) && (nl.what() == item_locator::object::item))
					{
						_m_set_selected(nl.node());
						if(_m_adjust(node_state_.selected, 1))
							adjust_.scroll_timestamp = 1;

						_m_draw(true);
						API::lazy_refresh();
					}
				}

				void trigger::mouse_move(graph_reference graph, const eventinfo& ei)
				{
					if(_m_track_mouse(ei.mouse.x, ei.mouse.y))
					{
						_m_draw(false);
						API::lazy_refresh();
					}
				}

				void trigger::mouse_wheel(graph_reference, const eventinfo& ei)
				{
					unsigned prev = shape_.prev_first_value;

					shape_.scrollbar.make_step(!ei.wheel.upwards);

					_m_event_scrollbar(ei);

					if(prev != shape_.prev_first_value)
					{
						_m_track_mouse(ei.wheel.x, ei.wheel.y);

						_m_draw(false);
						API::lazy_refresh();
					}
				}

				void trigger::resize(graph_reference, const eventinfo&)
				{
					_m_draw(false);
					API::lazy_refresh();
					_m_show_scrollbar();
					if(shape_.scrollbar.empty() == false)
					{
						shape_.scrollbar.move(graph_->width() - 16, 0);
						shape_.scrollbar.size(16, graph_->height());
					}
				}

				void trigger::key_down(graph_reference, const eventinfo& ei)
				{
					bool redraw = false;
					bool scroll = false; //Adjust the scrollbar

					switch(ei.keyboard.key)
					{
					case keyboard::os_arrow_up:
						if(node_state_.selected && node_state_.selected != attr_.tree_cont.get_root()->child)
						{
							node_type * prev = node_state_.selected->owner;
							if(prev->child != node_state_.selected)
							{
								prev = prev->child;
								while(prev->next != node_state_.selected)
									prev = prev->next;

								while(prev->child && prev->value.second.expanded)
								{
									prev = prev->child;
									while(prev->next)
										prev = prev->next;
								}
							}

							_m_set_selected(prev);

							if(_m_adjust(prev, 4))
								scroll = true;

							redraw = true;
						}
						break;
					case keyboard::os_arrow_down:
						if(node_state_.selected)
						{
							node_type * node = node_state_.selected;
							if(node->value.second.expanded)
							{
								node = node->child;
							}
							else if(node->next)
							{
								node = node->next;
							}
							else
							{
								node = node->owner;
								while(node && (nullptr == node->next))
									node = node->owner;

								if(node)
									node = node->next;
							}

							if(node)
							{
								_m_set_selected(node);
								redraw = true;
								if(_m_adjust(node_state_.selected, 4))
									scroll = true;
							}
						}
						break;
					case keyboard::os_arrow_left:
						if(node_state_.selected)
						{
							if(node_state_.selected->value.second.expanded == false)
							{
								if(node_state_.selected->owner != attr_.tree_cont.get_root())
								{
									_m_set_selected(node_state_.selected->owner);
									_m_adjust(node_state_.selected, 4);
								}
							}
							else
								_m_set_expanded(node_state_.selected, false);

							redraw = true;
							scroll = true;
						}
						break;
					case keyboard::os_arrow_right:
						if(node_state_.selected)
						{
							if(node_state_.selected->value.second.expanded == false)
							{
								_m_set_expanded(node_state_.selected, true);
								redraw = true;
								scroll = true;
							}
							else if(node_state_.selected->child)
							{
								_m_set_selected(node_state_.selected->child);
								_m_adjust(node_state_.selected, 4);
								redraw = true;
								scroll = true;
							}
						}
						break;
					}

					if(redraw)
					{
						_m_draw(scroll);
						API::lazy_refresh();
					}
				}

				void trigger::key_char(graph_reference, const eventinfo& ei)
				{
					auto node = const_cast<node_type*>(_m_find_track_node(ei.keyboard.key));

					if(node && (node != node_state_.selected))
					{
						_m_set_selected(node);
						_m_adjust(node, 4);
						_m_draw(false);
						API::lazy_refresh();
					}
				}

				void trigger::_m_find_first(unsigned long offset)
				{
					node_desc_.first = attr_.tree_cont.advance_if(nullptr, offset, pred_allow_child());
				}

				unsigned trigger::_m_node_height() const
				{
					return graph_->text_extent_size(STR("jh{")).height + 8;
				}

				unsigned trigger::_m_max_allow() const
				{
					return graph_->height() / _m_node_height();
				}

				bool no_sensitive_compare(const nana::string& text, const nana::char_t *pattern, std::size_t len)
				{
					if(len <= text.length())
					{
						const nana::char_t * s = text.c_str();
						for(std::size_t i = 0; i < len; ++i)
						{
							if('a' <= s[i] && s[i] <= 'z')
							{
								if(pattern[i] != s[i] - ('a' - 'A'))
									return false;
							}
							else
								if(pattern[i] != s[i]) return false;
						}
						return true;
					}
					return false;
				}

				const trigger::node_type* find_track_child_node(const trigger::node_type* node, const trigger::node_type * end, const nana::char_t* pattern, std::size_t len, bool &finish)
				{
					if(node->value.second.expanded)
					{
						node = node->child;

						while(node)
						{
							if(no_sensitive_compare(node->value.second.text, pattern, len)) return node;

							if(node == end) break;

							if(node->value.second.expanded)
							{
								auto t = find_track_child_node(node, end, pattern, len, finish);
								if(t || finish)
									return t;
							}
							node = node->next;
						}
					}

					finish = (node && (node == end));
					return nullptr;
				}

				const trigger::node_type* trigger::_m_find_track_node(nana::char_t key)
				{
					nana::string pattern;

					if('a' <= key && key <= 'z') key -= 'a' - 'A';

					unsigned long now = nana::system::timestamp();

					if(now - track_node_.key_time > 1000)
						track_node_.key_buf.clear();

					if(track_node_.key_buf.length())
					{
						if(track_node_.key_buf[track_node_.key_buf.length() - 1] != key)
							pattern = track_node_.key_buf;
					}

					track_node_.key_time = now;
					pattern += key;
					track_node_.key_buf += key;

					const node_type *begin = node_state_.selected ? node_state_.selected : attr_.tree_cont.get_root()->child;

					if(begin)
					{
						const node_type *node = begin;
						const node_type *end = nullptr;
						if(pattern.length() == 1)
						{
							if(node->value.second.expanded && node->child)
							{
								node = node->child;
							}
							else
							{
								if(nullptr == node->next)
								{
									if(nullptr == node->owner->next)
									{
										end = begin;
										node = attr_.tree_cont.get_root()->child;
									}
									else
										node = node->owner->next;
								}
								else
									node = node->next;
							}
						}

						while(node)
						{
							if(no_sensitive_compare(node->value.second.text, pattern.c_str(), pattern.length())) return node;

							bool finish;
							const node_type *child = find_track_child_node(node, end, pattern.c_str(), pattern.length(), finish);
							if(child)
								return child;

							if(finish || (node == end))
								return nullptr;

							if(nullptr == node->next)
							{
								node = (node->owner ? node->owner->next : nullptr);
								if(nullptr == node)
								{
									node = attr_.tree_cont.get_root()->child;
									end = begin;
								}
							}
							else
								node = node->next;
						}
					}
					return nullptr;
				}

				nana::paint::image* trigger::_m_image(const node_type* node)
				{
					const nana::string& idstr = node->value.second.img_idstr;
					if(idstr.size())
					{
						auto i = shape_.image_table.find(idstr);
						if(i == shape_.image_table.end())
							return nullptr;

						unsigned long state = 0xFFFFFFFF;
						if(node_state_.highlight == node && node_state_.highlight_object == item_locator::object::item)
							state = (node_state_.highlight != node_state_.selected ? 0: 1);
						else if(node_state_.selected == node)
							state = 2;

						node_image_tag & nodeimg = i->second;
						if(node->value.second.expanded || (state == 1 || state == 2))
							if(nodeimg.expanded.empty() == false)	return &nodeimg.expanded;

						if(node->value.second.expanded == false && state == 0)
							if(nodeimg.highlighted.empty() == false)	return &nodeimg.highlighted;

						return &nodeimg.normal;
					}
					return nullptr;
				}

				bool trigger::_m_track_mouse(int x, int y)
				{
					int xpos = attr_.tree_cont.indent_size(node_desc_.first) * node_desc_.indent_size - node_desc_.offset_x;
					item_locator nl(*this, xpos, x, y);
					attr_.tree_cont.for_each<item_locator&>(node_desc_.first, nl);

					bool redraw = false;
					node_state_.event_node = nl.node();

					if(nl.node() && (nl.what() != item_locator::object::none))
					{
						if((nl.what() != node_state_.highlight_object || nl.node() != node_state_.highlight))
						{
							node_state_.highlight_object = nl.what();
							node_state_.highlight = nl.node();
							redraw = (node_state_.highlight_object != item_locator::object::none);
						}
					}
					else if(node_state_.highlight)
					{
						redraw = true;
						node_state_.highlight_object = 0;
						node_state_.highlight = nullptr;
						_m_close_tooltip_window();
					}

					if(redraw)
					{
						if(node_state_.highlight_object == item_locator::object::item)
						{
							_m_adjust(node_state_.highlight, 2);
							adjust_.scroll_timestamp = 1;

							_m_tooltip_window(node_state_.highlight, nl.pos(), nl.size());
						}
					}

					return redraw;
				}

				void trigger::_m_tooltip_window(node_type* node, const nana::point& pos, const nana::size& size)
				{
					_m_close_tooltip_window();

					if((nullptr == node_state_.tooltip) && (pos.x + size.width > _m_visible_width()))
					{
						node_state_.tooltip = new tooltip_window(widget_->handle(), pos, size);
						node_state_.tooltip->show_text(node->value.second.text, node_desc_.text_offset + node_desc_.image_width, (node == node_state_.selected), this->_m_image(node));

						node_state_.tooltip->make_event<events::mouse_leave>(*this, &trigger::_m_close_tooltip_window);
						node_state_.tooltip->make_event<events::mouse_move>(*this, &trigger::_m_mouse_move_tooltip_window);

						auto click_fn = nana::make_fun(*this, &trigger::_m_click_tooltip_window);
						node_state_.tooltip->make_event<events::mouse_down>(click_fn);
						node_state_.tooltip->make_event<events::mouse_up>(click_fn);
						node_state_.tooltip->make_event<events::dbl_click>(click_fn);
					}
				}

				void trigger::_m_close_tooltip_window()
				{
					if(node_state_.tooltip)
					{
						tooltip_window * x = node_state_.tooltip;
						node_state_.tooltip = nullptr;
						delete x;
					}
				}

				void trigger::_m_mouse_move_tooltip_window()
				{
					nana::point pos = API::cursor_position();
					API::calc_window_point(widget_->handle(), pos);

					if(pos.x >= static_cast<int>(_m_visible_width()))
						_m_close_tooltip_window();
				}

				void trigger::_m_click_tooltip_window(const eventinfo& ei)
				{
					bool redraw = false;
					switch(ei.identifier)
					{
					case events::mouse_down::identifier:
						if(_m_adjust(node_state_.highlight, 1))
							adjust_.scroll_timestamp = 1;
						break;
					case events::mouse_up::identifier:
						if(node_state_.selected != node_state_.highlight)
						{
							_m_set_selected(node_state_.highlight);
							redraw = true;
						}
						break;
					default:
						_m_set_expanded(node_state_.selected, !node_state_.selected->value.second.expanded);
						redraw = true;
					}

					if(redraw)
					{
						_m_draw(false);
						API::update_window(widget_->handle());
					}
				}

				bool trigger::_m_draw(bool scrollbar_react)
				{
					if(graph_ && (false == dwflags_.pause))
					{
						if(scrollbar_react)
							_m_show_scrollbar();

						//draw background
						graph_->rectangle(widget_->background(), true);

						_m_draw_tree();
						return true;
					}
					return false;
				}

				void trigger::_m_draw_tree()
				{
					attr_.tree_cont.for_each(node_desc_.first, item_renderer(*this, nana::point(static_cast<int>(attr_.tree_cont.indent_size(node_desc_.first) * node_desc_.indent_size) - node_desc_.offset_x, 1)));
				}

				unsigned trigger::_m_visible_width() const
				{
					if(graph_)
					{
						return graph_->width() - (shape_.scrollbar.empty() ? 0 : shape_.scrollbar.size().width);
					}
					return 0;
				}

				void trigger::_m_show_scrollbar()
				{
					if(nullptr == graph_) return;

					unsigned max_allow = _m_max_allow();
					unsigned visual_items = visual_item_size();

					if(visual_items <= max_allow)
					{
						if(false == shape_.scrollbar.empty())
						{
							shape_.scrollbar.close();
							node_desc_.first = nullptr;
						}
					}
					else
					{
						if(shape_.scrollbar.empty())
						{
							using namespace nana::gui;
							shape_.prev_first_value = 0;
							shape_.scrollbar.create(*widget_, nana::rectangle(graph_->width() - 16, 0, 16, graph_->height()));
							auto scroll_fn = nana::make_fun(*this, &trigger::_m_event_scrollbar);
							shape_.scrollbar.make_event<events::mouse_down>(scroll_fn);
							shape_.scrollbar.make_event<events::mouse_move>(scroll_fn);
							shape_.scrollbar.make_event<events::mouse_wheel>(scroll_fn);
						}

						shape_.scrollbar.amount(visual_items);
						shape_.scrollbar.range(max_allow);
					}

					shape_.scrollbar.value(attr_.tree_cont.distance_if(node_desc_.first, pred_allow_child()));
				}

				void trigger::_m_event_scrollbar(const eventinfo& ei)
				{
					if(ei.identifier == events::mouse_wheel::identifier || ei.mouse.left_button)
					{
						if(shape_.prev_first_value != shape_.scrollbar.value())
						{
							shape_.prev_first_value = static_cast<unsigned long>(shape_.scrollbar.value());
							adjust_.scroll_timestamp = nana::system::timestamp();
							adjust_.timer.enable(true);

							node_desc_.first = attr_.tree_cont.advance_if(0, shape_.prev_first_value, pred_allow_child());

							if(ei.window == shape_.scrollbar.handle())
							{
								_m_draw(false);
								API::update_window(widget_->handle());
							}
						}
					}
				}

				bool trigger::_m_adjust(node_type * node, int reason)
				{
					if(nullptr == node) return false;

					switch(reason)
					{
					case 0:
						//adjust if the node expanded and the number of its children are over the max number allowed
						if(node_desc_.first != node)
						{
							unsigned child_size = attr_.tree_cont.child_size_if(*node, pred_allow_child());
							const unsigned max_allow = _m_max_allow();

							if(child_size < max_allow)
							{
								unsigned off1 = attr_.tree_cont.distance_if(node_desc_.first, pred_allow_child());
								unsigned off2 = attr_.tree_cont.distance_if(node, pred_allow_child());
								const unsigned size = off2 - off1 + child_size + 1;
								if(size > max_allow)
									node_desc_.first = attr_.tree_cont.advance_if(node_desc_.first, size - max_allow, pred_allow_child());
							}
							else
								node_desc_.first = node;
						}
						break;
					case 1:
					case 2:
					case 3:
						//param is the begin pos of an item in absolute.
						{
							int beg = static_cast<int>(attr_.tree_cont.indent_size(node) * node_desc_.indent_size) - node_desc_.offset_x;
							int end = beg + static_cast<int>(this->node_width(node)) + node_desc_.item_offset;

							bool adjust = false;
							if(reason == 1)
								adjust = (beg < 0 || (beg > 0 && end > static_cast<int>(_m_visible_width())));
							else if(reason == 2)
								adjust = (beg < 0);
							else if(reason == 3)
								return (beg > 0 && end > static_cast<int>(_m_visible_width()));

							if(adjust)
							{
								adjust_.offset_x_adjust = node_desc_.offset_x + beg;
								adjust_.node = node;
								adjust_.timer.enable(true);
								return true;
							}
						}
						break;
					case 4:
						if(node_desc_.first != node)
						{
							unsigned off_first = attr_.tree_cont.distance_if(node_desc_.first, pred_allow_child());
							unsigned off_node = attr_.tree_cont.distance_if(node, pred_allow_child());
							if(off_node < off_first)
							{
								node_desc_.first = node;
								return true;
							}
							else if(off_node - off_first > _m_max_allow())
							{
								node_desc_.first = attr_.tree_cont.advance_if(0, off_node - _m_max_allow() + 1, pred_allow_child());
								return true;
							}
						}
						break;
					}
					return false;
				}

				bool trigger::_m_set_selected(trigger::node_type * node)
				{
					if(node_state_.selected != node)
					{
						dwflags_.pause = true;
						if(node_state_.selected)
							attr_.ext_event.selected(widget_->handle(), reinterpret_cast<ext_event_type::node_type>(node_state_.selected), false);

						node_state_.selected = node;
						if(node)
							attr_.ext_event.selected(widget_->handle(), reinterpret_cast<ext_event_type::node_type>(node), true);
						dwflags_.pause = false;
						return true;
					}
					return false;
				}

				bool trigger::_m_set_expanded(node_type* node, bool value)
				{
					if(node && node->value.second.expanded != value)
					{
						if(value == false)
						{
							//if contracting a parent of the selected node, select the contracted node.
							if(check(node, node_state_.selected))
								_m_set_selected(node);
						}

						node->value.second.expanded = value;
						if(node->child)
						{
							dwflags_.pause = true;
							attr_.ext_event.expand(widget_->handle(), reinterpret_cast<ext_event_type::node_type>(node), value);
							dwflags_.pause = false;
						}
						return true;
					}
					return false;
				}

				void trigger::_m_deal_adjust()
				{
					if(adjust_.scroll_timestamp && (nana::system::timestamp() - adjust_.scroll_timestamp >= 500))
					{
						if(adjust_.offset_x_adjust == 0)
						{
							if(false == _m_adjust(adjust_.node ? adjust_.node : node_desc_.first, 1))
							{
								adjust_.offset_x_adjust = 0;
								adjust_.node = nullptr;
								adjust_.scroll_timestamp = 0;
								adjust_.timer.enable(false);
								return;
							}
						}

						const int delta = 5;
						int old = node_desc_.offset_x;
						if(node_desc_.offset_x < adjust_.offset_x_adjust)
						{
							node_desc_.offset_x += delta;
							if(node_desc_.offset_x > adjust_.offset_x_adjust)
								node_desc_.offset_x = adjust_.offset_x_adjust;
						}
						else if(node_desc_.offset_x > adjust_.offset_x_adjust)
						{
							node_desc_.offset_x -= delta;
							if(node_desc_.offset_x < adjust_.offset_x_adjust)
								node_desc_.offset_x = adjust_.offset_x_adjust;
						}

						_m_draw(false);
						API::update_window(widget_->handle());

						if(node_state_.tooltip)
						{
							nana::point pos = node_state_.tooltip->pos();
							pos.x -= (node_desc_.offset_x - old);
							node_state_.tooltip->move(pos.x, pos.y);
						}

						if(node_desc_.offset_x == adjust_.offset_x_adjust)
						{
							adjust_.offset_x_adjust = 0;
							adjust_.node = nullptr;
							adjust_.scroll_timestamp = 0;
							adjust_.timer.enable(false);
						}
					}
				}

				//Functor
				//class item_renderer
					trigger::item_renderer::item_renderer(trigger& drawer, const nana::point& pos)
						:drawer_(drawer), pos_(pos)
					{
					}

					//affect
					//0 = Sibling, the last is a sibling of node
					//1 = Owner, the last is the owner of node
					//>=2 = Children, the last is a child of a node that before this node.
					int trigger::item_renderer::operator()(const trigger::item_renderer::node_type& node, int affect)
					{
						switch(affect)
						{
						case 1:
							pos_.x += drawer_.node_desc_.indent_size;
							break;
						default:
							if(affect >= 2)
								pos_.x -= drawer_.node_desc_.indent_size * (affect - 1);

						}

						_m_background(node, (node.child != 0), node.value.second.expanded);

						drawer_.graph_->string(	pos_.x + drawer_.node_desc_.item_offset + drawer_.node_desc_.image_width + drawer_.node_desc_.text_offset,
												pos_.y + 3, 0x0, node.value.second.text);

						pos_.y += drawer_._m_node_height();

						if(pos_.y > static_cast<int>(drawer_.graph_->height()))
							return 0;

						return (node.child && node.value.second.expanded ? 1 : 2);
					}

					unsigned trigger::item_renderer::width(const node_type &node) const
					{
						return drawer_.node_width(&node);
					}

					void trigger::item_renderer::_m_draw_arrow(const node_type& node, unsigned item_height, bool expand)
					{
						using namespace nana::paint;
						const unsigned color = (&node == drawer_.node_state_.highlight && drawer_.node_state_.highlight_object == item_locator::object::arrow ? 0x1CC4F7 : 0x0);

						uint32_t style = 1;
						gadget::directions::t dir = gadget::directions::to_southeast;
						if(!expand)
						{
							style = 0;
							dir = gadget::directions::to_east;
						}

						gadget::arrow_16_pixels(*drawer_.graph_, pos_.x, pos_.y + (item_height - 16) / 2,
													color, style, dir);
					}

					void trigger::item_renderer::_m_background(const node_type& node, bool has_child, bool expand)
					{
						using namespace nana::paint;

						unsigned height = drawer_._m_node_height();

						if(has_child)
							_m_draw_arrow(node, height, expand);

						unsigned state = 0xFFFFFFFF;

						if(drawer_.node_state_.highlight == &node && drawer_.node_state_.highlight_object == item_locator::object::item)
							state = (drawer_.node_state_.highlight != drawer_.node_state_.selected ? 0: 1);
						else if(drawer_.node_state_.selected == &node)
							state = 2;

						if(state < 3)
						{
							const unsigned color[][2] = {	{0xE8F5FD, 0xD8F0FA}, //highlighted
															{0xC4E8FA, 0xB6E6FB}, //Selected and highlighted
															{0xD5EFFC, 0x99DEFD}  //Selected but not highlighted
														};

							graphics &graph = *drawer_.graph_;
							unsigned width = this->width(node);

							int left = pos_.x + drawer_.node_desc_.item_offset;
							int right = left + width - 1;
							int top = pos_.y;
							int bottom = top + height - 1;

							graph.rectangle(left, top, width, height, color[state][0], true);

							const unsigned colorx = color[state][1];
							graph.line(left + 1, top, right - 1, top, colorx);
							graph.line(left + 1, bottom, right - 1, bottom, colorx);

							graph.line(left, top + 1, left, bottom - 1, colorx);
							graph.line(right, top + 1, right, bottom - 1, colorx);
						}

						nana::paint::image * img = drawer_._m_image(&node);
						if(img)
							img->paste(*drawer_.graph_, pos_.x + drawer_.node_desc_.item_offset + 2, pos_.y + 2);
					}
				//end class item_renderer

				//class item_locator
					trigger::item_locator::item_locator(trigger& drawer, int item_pos, int x, int y)
						:drawer_(drawer), item_pos_(item_pos), item_ypos_(1), pos_(x, y), object_(object::none), node_(nullptr)
					{}

					int trigger::item_locator::operator()(tree_cont_type::node_type &node, int affect)
					{
						switch(affect)
						{
						case 0: break;
						case 1: item_pos_ += drawer_.node_desc_.indent_size; break;
						default:
							if(affect >= 2)
								item_pos_ -= drawer_.node_desc_.indent_size * (affect - 1);
						}

						if((pos_.y - item_ypos_) < static_cast<int>(drawer_._m_node_height()))
						{
							if(item_pos_ < pos_.x && pos_.x < item_pos_ + drawer_.node_desc_.item_offset)
								object_ = (node.child ? object::arrow : object::none);
							else if(item_pos_ + drawer_.node_desc_.item_offset <= pos_.x && pos_.x < item_pos_ + drawer_.node_desc_.item_offset + static_cast<int>(drawer_.node_width(&node)))
								object_ = object::item;
							else
								object_ = object::none;

							node_ = &node;
							return 0;
						}

						item_ypos_ += drawer_._m_node_height();

						if(node.value.second.expanded && node.child)
							return 1;

						return 2;
					}

					trigger::tree_cont_type::node_type * trigger::item_locator::node() const
					{
						return node_;
					}

					unsigned trigger::item_locator::what() const
					{
						return object_;
					}

					nana::point trigger::item_locator::pos() const
					{
						return nana::point(item_pos_ + drawer_.node_desc_.item_offset, item_ypos_);
					}

					nana::size trigger::item_locator::size() const
					{
						return nana::size(static_cast<int>(drawer_.node_width(node_)), drawer_._m_node_height());
					}
				//end class item_locator

				//struct pred_allow_child
					bool trigger::pred_allow_child::operator()(const tree_cont_type::node_type& node)
					{
						return node.value.second.expanded;
					}
				//end struct pred_all_child

				//struct drawing_flags
					trigger::drawing_flags::drawing_flags()
						:pause(false)
					{}
				//end struct drawing_flags;

				//struct shape_data_type
					trigger::shape_data_type::shape_data_type()
						:prev_first_value(0)
					{}
				//end struct shape_data_type

				//struct attribute_type
					trigger::attribute_type::attribute_type()
						: auto_draw(true), visual_item_size(0), button_width(16)
					{}
				//end struct attribute_type

				//struct node_desc_type
					trigger::node_desc_type::node_desc_type()
						:first(nullptr), indent_size(10),
						 offset_x(0),
						 item_offset(16), text_offset(3),
						 image_width(0)
					{}
				//end struct node_desc_type

				//struct node_state
					trigger::node_state::node_state()
						:tooltip(nullptr), highlight(nullptr), highlight_object(0), selected(nullptr), event_node(nullptr)
					{}
				//end struct node_state

				//struct track_node_tag
					trigger::track_node_tag::track_node_tag()
						:key_time(0)
					{}
				//end struct track_node_tag

				//struct adjust_desc_type
					trigger::adjust_desc_type::adjust_desc_type()
						: offset_x_adjust(0), node(nullptr), scroll_timestamp(0)
					{}
				//end struct adjust_desc_type
			//end class trigger
		}//end namespace treebox
	}//end namespace drawerbase
}//end namespace gui
}//end namespace nana
