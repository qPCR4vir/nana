
#include <nana/gui/widgets/menu.hpp>
#include <nana/system/platform.hpp>
#include <nana/paint/gadget.hpp>
#include <nana/gui/wvl.hpp>
#include <nana/paint/text_renderer.hpp>

namespace nana{ namespace gui{
	namespace drawerbase
	{
		namespace menu
		{
			//struct menu_item_type
				//class item_proxy
				//@brief: this class is used as parameter of menu event function.
					menu_item_type::item_proxy::item_proxy(std::size_t index, menu_item_type &item)
						:index_(index), item_(item)
					{}

					void menu_item_type::item_proxy::enabled(bool v)
					{
						item_.flags.enabled = v;
					}

					bool menu_item_type::item_proxy::enabled() const
					{
						return item_.flags.enabled;
					}

					std::size_t menu_item_type::item_proxy::index() const
					{
						return index_;
					}
				//end class item_proxy

				//Default constructor initializes the item as a splitter
				menu_item_type::menu_item_type()
					:sub_menu(0), style(gui::menu::check_none), hotkey(0)
				{
					flags.enabled = true;
					flags.splitter = true;
					flags.checked = false;
				}

				menu_item_type::menu_item_type(const nana::string& text, const event_fn_t& f)
					:sub_menu(0), text(text), functor(f), style(gui::menu::check_none), hotkey(0)
				{
					flags.enabled = true;
					flags.splitter = false;
					flags.checked = false;
				}
			//end class menu_item_type

			class internal_renderer
				: public renderer_interface
			{
				void background(graph_reference graph, window)
				{
					graph.rectangle(nana::gui::color::gray_border, false);
					graph.rectangle(1, 1, 28, graph.height() - 2, 0xF6F6F6, true);
					graph.rectangle(29, 1, graph.width() - 30, graph.height() - 2, 0xFFFFFF, true);
				}

				void item(graph_reference graph, const nana::rectangle& r, const attr& at)
				{
					if(at.item_state == state::active)
					{
						graph.rectangle(r, 0xA8D8EB, false);
						nana::point points[4] = {
							nana::point(r.x, r.y),
							nana::point(r.x + r.width - 1, r.y),
							nana::point(r.x, r.y + r.height - 1),
							nana::point(r.x + r.width - 1, r.y + r.height - 1)
						};
						for(int i = 0; i < 4; ++i)
							graph.set_pixel(points[i].x, points[i].y, 0xC0DDFC);

						if(at.enabled)
							graph.shadow_rectangle(nana::rectangle(r).pare_off(1), 0xE8F0F4, 0xDBECF4, true);
					}

					if(at.checked && (at.check_style != gui::menu::check_none))
					{
						graph.rectangle(r, 0xCDD3E6, false);
						graph.rectangle(nana::rectangle(r).pare_off(1), 0xE6EFF4, true);
						nana::paint::gadget::checker(graph, r.x, r.y, r.height, (at.check_style == gui::menu::check_option ? nana::paint::gadget::checkers::radio : nana::paint::gadget::checkers::clasp), 0x0);
					}
				}

				void item_image(graph_reference graph, const nana::point& pos, const paint::image& img)
				{
					img.paste(graph, pos.x, pos.y);
				}

				void item_text(graph_reference graph, const nana::point& pos, const nana::string& text, unsigned text_pixels, const attr& at)
				{
					nana::paint::text_renderer tr(graph);
					tr.render(pos.x, pos.y, (at.enabled ? 0x0 : nana::gui::color::gray_border), text.c_str(), text.length(), text_pixels, true);
				}

				void sub_arrow(graph_reference graph, const nana::point& pos, unsigned pixels, const attr&)
				{
					nana::paint::gadget::arrow_16_pixels(graph, pos.x, pos.y + static_cast<int>(pixels - 16) / 2, 0x0, 0, nana::paint::gadget::directions::to_east);
				}
			};

			//class renderer_interface
				renderer_interface::~renderer_interface()
				{}
			//end class renderer_interface

			class menu_builder
				: noncopyable
			{
			public:
				typedef menu_item_type item_type;

				typedef menu_type::item_container::value_type::event_fn_t event_fn_t;
				typedef menu_type::item_container::iterator iterator;
				typedef menu_type::item_container::const_iterator const_iterator;

				menu_builder()
				{
					root_.max_pixels = API::screen_size().width * 2 / 3;
					root_.item_pixels = 24;

					this->renderer_ = pat::cloneable<internal_renderer, renderer_interface>().clone();
				}

				~menu_builder()
				{
					this->destroy();
					this->renderer_->self_delete();
				}

				void check_style(std::size_t index, int style)
				{
					if(gui::menu::check_none <= style && style <= gui::menu::check_highlight)
					{
						if(root_.items.size() > index)
							root_.items[index].style = style;
					}
				}

				void checked(std::size_t index, bool check)
				{
					if(root_.items.size() > index)
					{
						item_type & m = root_.items[index];
						if(check && (m.style == gui::menu::check_option))
						{
							if(index)
							{
								std::size_t i = index;
								do
								{
									item_type& el = root_.items[--i];
									if(el.flags.splitter) break;

									if(el.style == gui::menu::check_option)
										el.flags.checked = false;
								}while(i);
							}

							for(std::size_t i = index + 1; i < root_.items.size(); ++i)
							{
								item_type & el = root_.items[i];
								if(el.flags.splitter) break;

								if(el.style == gui::menu::check_option)
									el.flags.checked = false;
							}
						}
						m.flags.checked = check;
					}
				}

				menu_type& data()
				{
					return root_;
				}

				const menu_type& data() const
				{
					return root_;
				}

				void insert(std::size_t pos, const nana::string& text, const event_fn_t& f)
				{
					menu_item_type m(text, f);
					if(pos < root_.items.size())
						root_.items.insert(root_.items.begin() + pos, m);
					else
						root_.items.push_back(m);
				}

				bool set_sub_menu(std::size_t pos, menu_type &sub)
				{
					if(root_.items.size() > pos)
					{
						menu_item_type & item = root_.items[pos];
						if(item.sub_menu == 0)
						{
							item.sub_menu = &sub;
							sub.owner.push_back(&root_);
							return true;
						}
					}
					return false;
				}

				void destroy()
				{
					for(std::vector<menu_type*>::iterator i = root_.owner.begin(); i != root_.owner.end(); ++i)
					{
						for(std::vector<menu_item_type>::iterator m = (*i)->items.begin(); m != (*i)->items.end(); ++m)
						{
							if(m->sub_menu == &root_)
								m->sub_menu = 0;
						}
					}

					for(std::vector<menu_item_type>::iterator i = root_.items.begin(); i != root_.items.end(); ++i)
					{
						if(i->sub_menu)
						{
							for(std::vector<menu_type*>::iterator m = i->sub_menu->owner.begin(); m != i->sub_menu->owner.end();)
							{
								if((*m) == &root_)
									m = i->sub_menu->owner.erase(m);
								else
									++m;
							}
						}
					}
				}

				pat::cloneable_interface<renderer_interface> * renderer() const
				{
					return renderer_;
				}

				void renderer(pat::cloneable_interface<renderer_interface>* rdptr)
				{
					renderer_ = rdptr;
				}
			private:
				menu_type root_;
				pat::cloneable_interface<renderer_interface> * renderer_;
			};//end class menu_builder

			class menu_drawer
				: public drawer_trigger
			{
			public:
				typedef menu_item_type::item_proxy item_proxy;

				renderer_interface * renderer;

				menu_drawer()
					:widget_(0), graph_(0), menu_(0)
				{
					state_.active = npos;
					state_.sub_window = false;
					state_.nullify_mouse = false;

					detail_.border.x = detail_.border.y = 2;
				}

				void bind_window(widget_reference widget)
				{
					widget_ = &widget;
				}

				void attached(graph_reference graph)
				{
					graph_ = &graph;
					window wd = widget_->handle();
					using namespace API::dev;
					make_drawer_event<events::mouse_move>(wd);
					make_drawer_event<events::mouse_down>(wd);
					make_drawer_event<events::mouse_leave>(wd);

					//Get the current cursor pos to determinate the monitor
					detail_.monitor_pos = API::cursor_position();
				}

				void detached()
				{
					API::dev::umake_drawer_event(widget_->handle());
				}

				void mouse_move(graph_reference, const eventinfo& ei)
				{
					state_.nullify_mouse = false;
					if(track_mouse(ei.mouse.x, ei.mouse.y))
					{
						draw();
						API::lazy_refresh();
					}
				}

				void mouse_leave(graph_reference, const eventinfo& ei)
				{
					state_.nullify_mouse = false;
					if(track_mouse(ei.mouse.x, ei.mouse.y))
					{
						draw();
						API::lazy_refresh();
					}
				}

				void mouse_down(graph_reference graph, const eventinfo& ei)
				{
					state_.nullify_mouse = false;
				}

				void refresh(graph_reference)
				{
					draw();
				}

				std::size_t active() const
				{
					return state_.active;
				}

				bool goto_next(bool forword)
				{
					state_.nullify_mouse = true;
					if(menu_->items.size())
					{
						std::size_t index = state_.active;

						bool end = false;
						while(true)
						{
							if(forword)
							{
								if(index == menu_->items.size() - 1)
								{
									if(end == false)
									{
										end = true;
										index = 0;
									}
									else
									{
										index = npos;
										break;
									}
								}
								else
									++index;
							}
							else
							{
								if(index == 0 || index == npos)
								{
									if(end == false)
									{
										end = true;
										index = menu_->items.size() - 1;
									}
									else
										break;
								}
								else
									--index;
							}
							if(false == menu_->items.at(index).flags.splitter)
								break;
						}

						if(index != npos && index != state_.active)
						{
							state_.active = index;
							state_.sub_window = false;

							draw();
							return true;
						}
					}
					return false;
				}

				bool track_mouse(int x, int y)
				{
					if(state_.nullify_mouse == false)
					{
						unsigned long index = _m_get_index_by_pos(x, y);
						if(index != state_.active)
						{
							if(index == npos && menu_->items.at(state_.active).sub_menu && state_.sub_window)
								return false;

							state_.active = (index != npos && menu_->items.at(index).flags.splitter) ? npos : index;

							state_.active_timestamp = nana::system::timestamp();
							return true;
						}
					}
					return false;
				}

				void data(menu_type & menu)
				{
					menu_ = & menu;
				}

				menu_type* data() const
				{
					return menu_;
				}

				void set_sub_window(bool subw)
				{
					state_.sub_window = subw;
				}

				menu_type* retrive_sub_menu(nana::point& pos, unsigned long interval) const
				{
					if(state_.active != npos && (nana::system::timestamp() - state_.active_timestamp >= interval))
					{
						pos.x = graph_->width() - 2;
						pos.y = 2;

						unsigned long index = 0;
						for(menu_builder::const_iterator it = menu_->items.begin(); it != menu_->items.end(); ++it, ++index)
						{
							if(false == it->flags.splitter)
							{
								if(index == state_.active)
									break;

								pos.y += _m_item_height() + 1;
							}
							else
								pos.y += 2;
						}
						return (menu_->items.at(state_.active).sub_menu);
					}
					return 0;
				}

				//send_shortkey has 3 states, 0 = UNKNOWN KEY, 1 = ITEM, 2 = GOTO SUBMENU
				int send_shortkey(nana::char_t key)
				{
					unsigned long index = 0;
					for(menu_builder::iterator it = menu_->items.begin(); it != menu_->items.end(); ++it, ++index)
					{
						if(it->hotkey == key)
						{
							if(it->flags.splitter == false)
							{
								if(it->sub_menu)
								{
									state_.active = static_cast<unsigned long>(std::distance(it, menu_->items.begin()));
									state_.active_timestamp = nana::system::timestamp();

									this->draw();

									nana::gui::API::update_window(widget_->handle());

									return 2;
								}
								else if(it->flags.enabled)
								{
									item_proxy ip(index, *it);
									it->functor.operator()(ip);
									return 1;
								}
							}
							break;
						}
					}

					return 0;
				}

				void draw() const
				{
					if(menu_ == 0) return;

					_m_adjust_window_size();

					renderer->background(*graph_, *widget_);

					nana::rectangle item_r(2, 2, graph_->width() - 4, _m_item_height());

					unsigned strpixels = item_r.width - 60;

					unsigned item_height = _m_item_height();
					int text_top_off = (_m_item_height() - graph_->text_extent_size(STR("jh({[")).height) / 2;

					unsigned long index = 0;
					for(menu_builder::const_iterator it = menu_->items.begin(); it != menu_->items.end(); ++it, ++index)
					{
						if(false == it->flags.splitter)
						{
							renderer_interface::attr attr = _m_make_renderer_attr(index == state_.active, *it);
							//Draw item background
							renderer->item(*graph_, item_r, attr);

							//Draw text, the text is transformed from orignal for hotkey character
							nana::string::value_type hotkey;
							nana::string::size_type hotkey_pos;
							nana::string text = nana::gui::API::transform_shortkey_text(it->text, hotkey, &hotkey_pos);

							if(it->image.empty() == false)
								renderer->item_image(*graph_, nana::point(item_r.x + 5, item_r.y + (item_height - it->image.size().height) / 2), it->image);

							renderer->item_text(*graph_, nana::point(item_r.x + 40, item_r.y + text_top_off), text, strpixels, attr);

							if(hotkey)
							{
								it->hotkey = hotkey;
								if(it->flags.enabled)
								{
									unsigned off_w = (hotkey_pos ? graph_->text_extent_size(text, static_cast<unsigned>(hotkey_pos)).width : 0);
									nana::size hotkey_size = graph_->text_extent_size(text.c_str() + hotkey_pos, 1);
									int x = item_r.x + 40 + off_w;
									int y = item_r.y + text_top_off + hotkey_size.height;
									graph_->line(x, y, x + hotkey_size.width - 1, y, 0x0);
								}
							}

							if(it->sub_menu)
								renderer->sub_arrow(*graph_, nana::point(graph_->width() - 20, item_r.y), _m_item_height(), attr);

							item_r.y += item_r.height + 1;
						}
						else
						{
							graph_->line(item_r.x + 40, item_r.y, graph_->width() - 1, item_r.y, nana::gui::color::gray_border);
							item_r.y += 2;
						}
					}
				}
			private:
				static renderer_interface::attr _m_make_renderer_attr(bool active, const menu_item_type & m)
				{
					renderer_interface::attr attr;
					attr.item_state = (active ? renderer_interface::state::active : renderer_interface::state::normal);
					attr.enabled = m.flags.enabled;
					attr.checked = m.flags.checked;
					attr.check_style = m.style;
					return attr;
				}

				unsigned long _m_get_index_by_pos(int x, int y) const
				{
					if(	(x < static_cast<int>(detail_.border.x)) ||
						(x > static_cast<int>(graph_->width() - detail_.border.x)) ||
						(y < static_cast<int>(detail_.border.y)) ||
						(y > static_cast<int>(graph_->height() - detail_.border.y)))
						return npos;

					int pos = detail_.border.y;
					unsigned long index = 0;
					for(menu_type::const_iterator it = menu_->items.begin(); it != menu_->items.end(); ++it)
					{
						unsigned h = (it->flags.splitter ? 1 : _m_item_height());
						if(pos <= y && y < static_cast<int>(pos + h))
							return index;

						else if(y < pos) return npos;

						++index;
						pos += (h + 1);
					}
					return npos;
				}

				unsigned _m_item_height() const
				{
					return menu_->item_pixels;
				}

				nana::size _m_client_size() const
				{
					nana::size size;

					if(menu_->items.size())
					{
						for(menu_type::const_iterator it = menu_->items.begin(); it != menu_->items.end(); ++it)
						{
							if(it->flags.splitter)
								++size.height;
							else
							{
								nana::size item_size = graph_->text_extent_size(it->text);
								if(size.width < item_size.width)
									size.width = item_size.width;
							}
						}

						size.width += (35 + 40);
						size.height = static_cast<unsigned>(menu_->items.size() - size.height) * _m_item_height() + size.height + static_cast<unsigned>(menu_->items.size() - 1);
					}

					if(size.width > menu_->max_pixels)
						size.width = menu_->max_pixels;

					return size;
				}

				void _m_adjust_window_size() const
				{
					nana::size size = _m_client_size();

					size.width += detail_.border.x * 2;
					size.height += detail_.border.y * 2;

					widget_->size(size.width, size.height);

					nana::point pos;
					API::calc_screen_point(*widget_, pos);

					//get the screen coordinates of the widget pos.
					nana::rectangle scr_area = API::screen_area_from_point(detail_.monitor_pos);

					if(pos.x + size.width > scr_area.x + scr_area.width)
						pos.x = static_cast<int>(scr_area.x + scr_area.width - size.width);
					if(pos.x < scr_area.x) pos.x = scr_area.x;

					if(pos.y + size.height > scr_area.y + scr_area.height)
						pos.y = static_cast<int>(scr_area.y + scr_area.height - size.height);
					if(pos.y < scr_area.y) pos.y = scr_area.y;

					window owner = API::get_owner_window(*widget_);
					API::calc_window_point(owner, pos);
					widget_->move(pos.x, pos.y);
				}
			private:
				nana::gui::widget		*widget_;
				nana::paint::graphics	*graph_;
				menu_type	*menu_;

				struct state
				{
					std::size_t		active;
					unsigned long	active_timestamp;
					unsigned long	sub_window: 1;
					unsigned long	nullify_mouse: 1;
				}state_;

				struct widget_detail
				{
					nana::point monitor_pos;	//It used to determinate the monitor.
					nana::upoint border;
				}detail_;
			};//end class menu_drawer

			class menu_window
				:	public widget_object<category::root_tag, menu_drawer>
			{
				typedef menu_drawer drawer_type;
				typedef widget_object<category::root_tag, menu_drawer> base_type;
			public:
				typedef menu_builder::item_type item_type;

				menu_window(window wd, const point& pos, renderer_interface * rdptr)
					:	base_type(wd, false, rectangle(pos, nana::size(2, 2)), appear::bald<appear::floating>())
				{
					get_drawer_trigger().renderer = rdptr;
					state_.owner_menubar = state_.self_submenu = false;
					state_.auto_popup_submenu = true;

					submenu_.child = submenu_.parent = 0;
					submenu_.object = 0;

					_m_make_mouse_event();
				}

				void popup(menu_type& menu, bool owner_menubar)
				{
					get_drawer_trigger().data(menu);
					API::take_active(this->handle(), false, 0);

					if(submenu_.parent == 0)
					{
						state_.owner_menubar = owner_menubar;
						API::register_menu_window(this->handle(), !owner_menubar);
					}

					timer_.interval(100);
					timer_.make_tick(nana::functor<void()>(*this, &menu_window::_m_check_repeatly));

					make_event<events::destroy>(*this, &menu_window::_m_destroy);
					make_event<events::key_down>(*this, &menu_window::_m_key_down);
					make_event<events::mouse_up>(*this, &menu_window::_m_strike);

					show();
				}

				void goto_next(bool forward)
				{
					menu_window * object = this;

					while(object->submenu_.child)
						object = object->submenu_.child;

					state_.auto_popup_submenu = false;

					if(object->get_drawer_trigger().goto_next(forward))
						API::update_window(object->handle());
				}

				bool goto_submenu()
				{
					menu_window * object = this;
					while(object->submenu_.child)
						object = object->submenu_.child;

					state_.auto_popup_submenu = false;

					nana::point pos;
					menu_type * sbm = object->get_drawer_trigger().retrive_sub_menu(pos, 0);
					return object->_m_show_submenu(sbm, pos, true);
				}

				bool exit_submenu()
				{
					menu_window * object =this;
					while(object->submenu_.child)
						object = object->submenu_.child;

					state_.auto_popup_submenu = false;

					menu_window * parent = object->submenu_.parent;

					if(parent)
					{
						parent->submenu_.child = 0;
						parent->submenu_.object = 0;
						object->close();
						return true;
					}
					return false;
				}

				int send_shortkey(nana::char_t key)
				{
					menu_window * object = this;
					while(object->submenu_.child)
						object = object->submenu_.child;

					return object->get_drawer_trigger().send_shortkey(key);
				}
			private:
				//_m_destroy just destroys the children windows.
				//If the all window including parent windows want to be closed call the _m_close_all() instead of close()
				void _m_destroy()
				{
					if(this->submenu_.parent)
					{
						this->submenu_.parent->submenu_.child = 0;
						this->submenu_.parent->submenu_.object = 0;
					}

					if(this->submenu_.child)
					{
						menu_window * tail = this;
						while(tail->submenu_.child) tail = tail->submenu_.child;

						while(tail != this)
						{
							menu_window * junk = tail;
							tail = tail->submenu_.parent;
							junk->close();
						}
					}
				}

				void _m_close_all()
				{
					menu_window * root = this;
					while(root->submenu_.parent)
						root = root->submenu_.parent;

					if(root != this)
					{
						//Disconnect the menu chain at this menu, and delete the menus before this.
						this->submenu_.parent->submenu_.child = 0;
						this->submenu_.parent->submenu_.object = 0;
						this->submenu_.parent = 0;
						root->close();
						//The submenu is treated its parent menu as an owner window,
						//if the root is closed, the all submenus will be closed
					}
					else
					{
						//Then, delete the menus from this.
						this->close();
					}
				}

				void _m_strike()
				{
					menu_window * object = this;
					while(object->submenu_.child)
						object = object->submenu_.child;

					std::size_t active = object->get_drawer_trigger().active();
					if(active != npos)
					{
						menu_type * menu = object->get_drawer_trigger().data();
						if(menu)
						{
							menu_item_type & item = menu->items.at(active);
							if(item.flags.splitter == false && item.sub_menu == 0)
							{
								//There is a situation that menu will not call functor if the item style is check_option
								//and it is checked before clicking.
								bool call_functor = true;

								if(item.style == gui::menu::check_highlight)
								{
									item.flags.checked = !item.flags.checked;
								}
								else if(gui::menu::check_option == item.style)
								{
									if(active > 0)
									{
										//clear the checked state in front of active if it is check_option.
										std::size_t i = active;
										do
										{
											--i;
											menu_item_type & im = menu->items.at(i);
											if(im.flags.splitter) break;

											if(im.style == gui::menu::check_option && im.flags.checked)
												im.flags.checked = false;
										}while(i);
									}

									for(std::size_t i = active + 1; i < menu->items.size(); ++i)
									{
										menu_item_type & im = menu->items.at(i);
										if(im.flags.splitter) break;

										if(im.style == gui::menu::check_option && im.flags.checked)
											im.flags.checked = false;
									}

									item.flags.checked = true;
								}

								this->_m_close_all();	//means deleting this;
								//The deleting operation has moved here, because item.functor.operator()(ip)
								//may create a window, which make a killing focus for menu window, if so the close_all
								//operation preformences after item.functor.operator()(ip), that would be deleting this object twice!

								if(call_functor && item.flags.enabled)
								{
									item_type::item_proxy ip(active, item);
									item.functor.operator()(ip);
								}
							}
						}
					}
				}

				void _m_key_down(const eventinfo& ei)
				{
					switch(ei.keyboard.key)
					{
					case keyboard::os_arrow_up:
						this->goto_next(false);
						break;
					case keyboard::os_arrow_down:
						this->goto_next(true);
						break;
					case keyboard::os_arrow_left:
						this->exit_submenu();
						break;
					case keyboard::os_arrow_right:
						this->goto_submenu();
						break;
					case keyboard::enter:
						this->_m_strike();
						break;
					default:
						if(2 != send_shortkey(ei.keyboard.key))
						{
							if(API::empty_window(*this) == false)
								close();
						}
						else
							goto_submenu();
					}
				}

				void _m_make_mouse_event()
				{
					state_.mouse_pos = API::cursor_position();
					make_event<events::mouse_move>(*this, &menu_window::_m_mouse_event);
				}

				void _m_mouse_event()
				{
					nana::point pos = API::cursor_position();
					if(pos != state_.mouse_pos)
					{
						menu_window * root = this;
						while(root->submenu_.parent)
							root = root->submenu_.parent;
						root->state_.auto_popup_submenu = true;

						state_.mouse_pos = pos;
					}
				}

				bool _m_show_submenu(menu_type* sbm, nana::point pos, bool forced)
				{
					menu_drawer & mdtrigger = get_drawer_trigger();
					if(submenu_.object && (sbm != submenu_.object))
					{
						mdtrigger.set_sub_window(false);
						submenu_.child->close();
						submenu_.child = 0;
						submenu_.object = 0;
					}

					if(sbm)
					{
						menu_window * root = this;
						while(root->submenu_.parent)
							root = root->submenu_.parent;

						if((submenu_.object == 0) && sbm && (forced || root->state_.auto_popup_submenu))
						{
							sbm->item_pixels = mdtrigger.data()->item_pixels;
							sbm->gaps = mdtrigger.data()->gaps;
							pos.x += sbm->gaps.x;
							pos.y += sbm->gaps.y;

							menu_window & mwnd = form_loader<menu_window>()(handle(), pos, get_drawer_trigger().renderer);
							mwnd.state_.self_submenu = true;
							submenu_.child = & mwnd;
							submenu_.child->submenu_.parent = this;
							submenu_.object = sbm;

							API::set_window_z_order(handle(), mwnd.handle(), z_order_action::none);
							mwnd.popup(*sbm, state_.owner_menubar);
							get_drawer_trigger().set_sub_window(true);
							if(forced)
								mwnd.goto_next(true);

							return true;
						}
					}
					return false;
				}

				void _m_check_repeatly()
				{
					if(state_.auto_popup_submenu)
					{
						nana::point pos = API::cursor_position();

						drawer_type& drawer = get_drawer_trigger();
						API::calc_window_point(handle(), pos);
						drawer.track_mouse(pos.x, pos.y);
						menu_type* sbm = drawer.retrive_sub_menu(pos, 500);
						_m_show_submenu(sbm, pos, false);
					}
				}
			private:
				timer timer_;
				struct state_type
				{
					bool self_submenu; //Indicates whether the menu window is used for a submenu
					bool owner_menubar;
					bool auto_popup_submenu;
					nana::point mouse_pos;
				}state_;

				struct submenu_type
				{
					menu_window*	parent;
					menu_window*	child;
					const menu_type *object;
				}submenu_;
			};
			//end class menu_window
		}//end namespace menu
	}//end namespace drawerbase

	//class menu
		struct menu::implement
		{
			struct info
			{
				menu * handle;
				bool kill;
			};

			drawerbase::menu::menu_builder	mbuilder;
			drawerbase::menu::menu_window *	uiobj;
			nana::functor<void()> destroy_answer;
			std::map<std::size_t, info> sub_container;		
		};

		menu::menu()
			:impl_(new implement)
		{
			impl_->uiobj = 0;
		}

		menu::~menu()
		{
			for(std::map<std::size_t, implement::info>::reverse_iterator i = impl_->sub_container.rbegin(); i != impl_->sub_container.rend(); ++i)
			{
				if(i->second.kill)
					delete i->second.handle;
			}
			delete impl_;
		}

		void menu::append(const nana::string& text, const menu::event_fn_t& f)
		{
			impl_->mbuilder.data().items.push_back(drawerbase::menu::menu_item_type(text, f));
		}

		void menu::append_splitter()
		{
			impl_->mbuilder.data().items.push_back(drawerbase::menu::menu_item_type());
		}

		void menu::clear()
		{
			impl_->mbuilder.data().items.clear();
		}

		void menu::enabled(std::size_t index, bool enable)
		{
			impl_->mbuilder.data().items.at(index).flags.enabled = enable;
		}

		bool menu::enabled(std::size_t index) const
		{
			return impl_->mbuilder.data().items.at(index).flags.enabled;
		}

		void menu::erase(std::size_t index)
		{
			std::vector<drawerbase::menu::menu_item_type> & items = impl_->mbuilder.data().items;
			if(index < items.size())
				items.erase(items.begin() + index);
		}

		void menu::image(std::size_t index, const paint::image& img)
		{
			impl_->mbuilder.data().items.at(index).image = img;
		}

		bool menu::link(std::size_t index, menu& menu_obj)
		{
			if(impl_->mbuilder.set_sub_menu(index, menu_obj.impl_->mbuilder.data()))
			{
				implement::info& minfo = impl_->sub_container[index];
				minfo.handle = &menu_obj;
				minfo.kill = false;
				return true;
			}
			return false;
		}

		menu* menu::link(std::size_t index)
		{
			std::map<std::size_t, implement::info>::iterator i = impl_->sub_container.find(index);
			if(i == impl_->sub_container.end())
				return 0;
			return i->second.handle;
		}

		menu *menu::create_sub_menu(std::size_t index)
		{
			menu * sub = new menu;
			if(link(index, *sub))
			{
				implement::info& minfo = impl_->sub_container[index];
				minfo.handle = sub;
				minfo.kill = true;
				return sub;
			}

			delete sub;
			return 0;
		}

		void menu::popup(nana::gui::window wd, int x, int y, bool owner_menubar)
		{
			if(impl_->mbuilder.data().items.size())
			{
				close();

				typedef drawerbase::menu::menu_window menu_window;
				impl_->uiobj = &(nana::gui::form_loader<menu_window>()(wd, point(x, y), &(impl_->mbuilder.renderer()->refer())));
				impl_->uiobj->make_event<nana::gui::events::destroy>(*this, &menu::_m_destroy_menu_window);
				impl_->uiobj->popup(impl_->mbuilder.data(), owner_menubar);
			}
		}

		void menu::close()
		{
			if(impl_->uiobj)
			{
				impl_->uiobj->close();
				impl_->uiobj = 0;
			}
		}

		void menu::check_style(std::size_t index, check_t style)
		{
			impl_->mbuilder.check_style(index, style);
		}

		void menu::checked(std::size_t index, bool check)
		{
			impl_->mbuilder.checked(index, check);
		}

		bool menu::checked(std::size_t index) const
		{
			return impl_->mbuilder.data().items.at(index).flags.checked;
		}

		void menu::answerer(std::size_t index, const menu::event_fn_t& fn)
		{
			impl_->mbuilder.data().items.at(index).functor = fn;
		}

		void menu::destroy_answer(const nana::functor<void()>& f)
		{
			impl_->destroy_answer = f;
		}

		void menu::gaps(const nana::point& pos)
		{
			impl_->mbuilder.data().gaps = pos;
		}

		void menu::goto_next(bool forward)
		{
			if(impl_->uiobj)
				impl_->uiobj->goto_next(forward);
		}

		bool menu::goto_submen()
		{
			return (impl_->uiobj ? impl_->uiobj->goto_submenu() : false);
		}

		bool menu::exit_submenu()
		{
			return (impl_->uiobj ? impl_->uiobj->exit_submenu() : false);
		}

		std::size_t menu::size() const
		{
			return impl_->mbuilder.data().items.size();
		}

		int menu::send_shortkey(nana::char_t key)
		{
			return (impl_->uiobj ? impl_->uiobj->send_shortkey(key) : 0);
		}

		menu& menu::max_pixels(unsigned px)
		{
			impl_->mbuilder.data().max_pixels = (px > 100 ? px : 100);
			return *this;
		}

		unsigned menu::max_pixels() const
		{
			return impl_->mbuilder.data().max_pixels;
		}

		menu& menu::item_pixels(unsigned px)
		{
			impl_->mbuilder.data().item_pixels = px;
			return *this;
		}

		unsigned menu::item_pixels() const
		{
			return impl_->mbuilder.data().item_pixels;
		}

		pat::cloneable_interface<menu::renderer_interface> * menu::renderer() const
		{
			return impl_->mbuilder.renderer();
		}

		void menu::renderer(const pat::cloneable_interface<renderer_interface>* rdptr)
		{
			if(rdptr)
				impl_->mbuilder.renderer(rdptr->clone());
		}

		void menu::_m_destroy_menu_window()
		{
			impl_->uiobj = 0;
			impl_->destroy_answer();
		}
	//end class menu

		detail::popuper menu_popuper(menu& mobj, mouse::t ms)
	{
		return detail::popuper(mobj, ms);
	}

		detail::popuper menu_popuper(menu& mobj, window owner, const point& pos, mouse::t ms)
	{
		return detail::popuper(mobj, owner, pos, ms);
	}

	namespace detail
	{
	//class popuper
		popuper::popuper(menu& mobj, mouse::t ms)
			: mobj_(mobj), owner_(0), take_mouse_pos_(true), mouse_(ms)
		{}

		popuper::popuper(menu& mobj, window owner, const point& pos, mouse::t ms)
			: mobj_(mobj), owner_(owner), take_mouse_pos_(false), pos_(pos), mouse_(ms)
		{}

		void popuper::operator()(const eventinfo& ei)
		{
			if(take_mouse_pos_)
			{
				switch(ei.identifier)
				{
				case events::click::identifier:
				case events::mouse_down::identifier:
				case events::mouse_up::identifier:
					owner_ = ei.window;
					pos_.x = ei.mouse.x;
					pos_.y = ei.mouse.y;
					break;
				default:
					return;
				}
			}
			bool popup = false;
			switch(mouse_)
			{
			case mouse::left_button:
				popup = ei.mouse.left_button;
				break;
			case mouse::middle_button:
				popup = ei.mouse.mid_button;
				break;
			case mouse::right_button:
				popup = ei.mouse.right_button;
				break;
			case mouse::any_button:
				popup = true;
			}
			if(popup)
				mobj_.popup(owner_, pos_.x, pos_.y, false);
		}
	//end class
	}//end namespace detail
}//end namespace gui
}//end namespace nana
