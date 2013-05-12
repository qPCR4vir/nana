#include <nana/gui/widgets/menubar.hpp>
#include <stdexcept>

namespace nana
{
namespace gui
{
	namespace drawerbase
	{
		namespace menubar
		{
			struct item_type
			{
				item_type(const nana::string& text, unsigned long shortkey)
					: text(text), shortkey(shortkey)
				{}

				nana::string	text;
				unsigned long	shortkey;
				nana::gui::menu	menu_obj;
				nana::point		pos;
				nana::size		size;
			};

			class trigger::itembase
			{
			public:
				typedef std::vector<item_type*> container;

				~itembase()
				{
					for(auto i : cont_)
						delete i;
				}

				void append(const nana::string& text, unsigned long shortkey)
				{
					if(shortkey && shortkey < 0x61) shortkey += (0x61 - 0x41);
					cont_.push_back(new item_type(text, shortkey));
				}

				nana::gui::menu* get_menu(std::size_t index) const
				{
					return (index < cont_.size() ? &(cont_[index]->menu_obj) : nullptr);
				}

				const item_type& at(std::size_t index) const
				{
					return *cont_.at(index);
				}

				std::size_t find(unsigned long shortkey) const
				{
					if(shortkey)
					{
						if(shortkey < 0x61) shortkey += (0x61 - 0x41);

						std::size_t index = 0;
						for(auto i : cont_)
						{
							if(i->shortkey == shortkey)
								return index;
							++index;
						}
					}
					return npos;
				}

				const container& cont() const
				{
					return cont_;
				}
			private:
				container cont_;
			};

			//class item_renderer
				item_renderer::item_renderer(graph_reference graph)
					:graph_(graph)
				{}

				void item_renderer::background(const nana::point& pos, const nana::size& size, state_t state)
				{
					nana::color_t border, body, corner;

					switch(state)
					{
					case item_renderer::state_highlight:
						border = nana::gui::color::highlight;
						corner = 0xC0DDFC;
						body = 0xC0DDFC;
						break;
					case item_renderer::state_selected:
						border = nana::gui::color::dark_border;
						corner = nana::gui::color::button_face;
						body = 0xFFFFFF;
						break;
					default:	//Don't process other states.
						return;
					}

					nana::rectangle r(pos, size);
					graph_.rectangle(r, border, false);

					graph_.set_pixel(pos.x, pos.y, corner);
					graph_.set_pixel(pos.x + size.width - 1, pos.y, corner);
					graph_.set_pixel(pos.x, pos.y + size.height - 1, corner);
					graph_.set_pixel(pos.x + size.width - 1, pos.y + size.height - 1, corner);

					r.pare_off(1);
					graph_.rectangle(r, body, true);
				}

				void item_renderer::caption(int x, int y, const nana::string& text)
				{
					graph_.string(x, y, 0x0, text);
				}
			//end class item_renderer

			//class trigger
				trigger::trigger()
					: items_(new itembase)
				{}

				trigger::~trigger()
				{
					delete items_;
				}

				nana::gui::menu* trigger::push_back(const nana::string& text)
				{
					nana::string::value_type shkey;
					API::transform_shortkey_text(text, shkey, 0);

					if(shkey)
						API::register_shortkey(widget_->handle(), shkey);

					std::size_t i = items_->cont().size();
					items_->append(text, shkey);
					_m_draw();
					return items_->get_menu(i);
				}

				nana::gui::menu* trigger::at(std::size_t index) const
				{
					return items_->get_menu(index);
				}

				std::size_t trigger::size() const
				{
					return items_->cont().size();
				}

				void trigger::bind_window(widget_reference widget)
				{
					widget_ = &widget;
				}

				void trigger::attached(graph_reference graph)
				{
					graph_ = &graph;
					window wd = widget_->handle();
					using namespace API::dev;
					make_drawer_event<events::mouse_move>(wd);
					make_drawer_event<events::mouse_down>(wd);
					make_drawer_event<events::mouse_up>(wd);
					make_drawer_event<events::mouse_leave>(wd);
					make_drawer_event<events::focus>(wd);
					make_drawer_event<events::shortkey>(wd);
					make_drawer_event<events::key_down>(wd);
					make_drawer_event<events::key_up>(wd);
				}

				void trigger::detached()
				{
					API::dev::umake_drawer_event(widget_->handle());
				}

				void trigger::refresh(graph_reference)
				{
					_m_draw();
					API::lazy_refresh();
				}

				void trigger::mouse_move(graph_reference, const eventinfo& ei)
				{
					if(ei.mouse.x != state_.mouse_pos.x || ei.mouse.y != state_.mouse_pos.y)
						state_.nullify_mouse = false;

					bool popup = false;
					if(state_.behavior == state_type::behavior_focus)
					{
						std::size_t index = _m_item_by_pos(ei.mouse.x, ei.mouse.y);
						if(index != npos && state_.active != index)
						{
							state_.active = index;
							popup = true;
						}
					}
					else if(_m_track_mouse(ei.mouse.x, ei.mouse.y))
						popup = true;

					if(popup)
					{
						_m_popup_menu();
						_m_draw();
						API::lazy_refresh();
					}

					state_.mouse_pos.x = ei.mouse.x;
					state_.mouse_pos.y = ei.mouse.y;
				}

				void trigger::mouse_leave(graph_reference graph, const eventinfo& ei)
				{
					state_.nullify_mouse = false;
					mouse_move(graph, ei);
				}

				void trigger::mouse_down(graph_reference graph, const eventinfo& ei)
				{
					state_.nullify_mouse = false;

					state_.active = _m_item_by_pos(ei.mouse.x, ei.mouse.y);
					if(state_.menu_active == false)
					{
						if(state_.active != npos)
						{
							state_.menu_active = true;
							_m_popup_menu();
						}
						else
							_m_total_close();
					}
					else if(npos == state_.active)
						_m_total_close();
					else
						_m_popup_menu();

					_m_draw();
					API::lazy_refresh();
				}

				void trigger::mouse_up(graph_reference graph, const eventinfo& ei)
				{
					state_.nullify_mouse = false;

					if(state_.behavior != state_.behavior_menu)
					{
						if(state_.menu_active)
							state_.behavior = state_.behavior_menu;
					}
					else
					{
						state_.behavior = state_.behavior_none;
						_m_total_close();
						_m_draw();
						API::lazy_refresh();
					}

				}

				void trigger::focus(graph_reference, const eventinfo& ei)
				{
					if((ei.focus.getting == false) && (state_.active != npos))
					{
						state_.behavior = state_type::behavior_none;
						_m_close_menu();
						state_.active = npos;
						_m_draw();
						API::lazy_refresh();
					}
				}

				void trigger::key_down(graph_reference, const eventinfo& ei)
				{
					state_.nullify_mouse = true;
					if(state_.menu)
					{
						switch(ei.keyboard.key)
						{
						case keyboard::os_arrow_down:
							state_.menu->goto_next(true);  break;
						case keyboard::backspace:
						case keyboard::os_arrow_up:
							state_.menu->goto_next(false); break;
						case keyboard::os_arrow_right:
							if(state_.menu->goto_submen() == false)
								_m_move(false);
							break;
						case keyboard::os_arrow_left:
							if(state_.menu->exit_submenu() == false)
								_m_move(true);
							break;
						case keyboard::escape:
							if(state_.menu->exit_submenu() == false)
							{
								_m_close_menu();
								state_.behavior = state_.behavior_focus;
								state_.menu_active = false;
							}
							break;
						default:
							{
								if(2 != state_.menu->send_shortkey(ei.keyboard.key))
								{
									if(state_.active != npos)
									{
										_m_total_close();
										if(ei.keyboard.key == 18) //ALT
											state_.behavior = state_.behavior_focus;
									}
								}
								else
									state_.menu->goto_submen();
							}
						}
					}
					else
					{
						switch(ei.keyboard.key)
						{
						case keyboard::os_arrow_right:
							_m_move(false);
							break;
						case keyboard::backspace:
						case keyboard::os_arrow_left:
							_m_move(true);
							break;
						case keyboard::escape:
							if(state_.behavior == state_.behavior_focus)
							{
								state_.active= npos;
								state_.behavior = state_.behavior_none;
								API::restore_menubar_taken_window();
							}
						}
					}

					_m_draw();
					API::lazy_refresh();
				}

				void trigger::key_up(graph_reference, const eventinfo& ei)
				{
					if(ei.keyboard.key == 18)
					{
						if(state_.behavior == state_type::behavior_none)
						{
							state_.behavior = state_type::behavior_focus;
							state_.active = 0;
						}
						else
						{
							state_.behavior = state_type::behavior_none;
							nana::point pos = API::cursor_position();
							API::calc_window_point(widget_->handle(), pos);
							state_.active = _m_item_by_pos(pos.x, pos.y);
						}

						state_.menu_active = false;
						_m_draw();
						API::lazy_refresh();
					}
				}

				void trigger::shortkey(graph_reference graph, const eventinfo& ei)
				{
					API::focus_window(widget_->handle());

					std::size_t index = items_->find(ei.keyboard.key);
					if(index != npos && (index != state_.active || nullptr == state_.menu))
					{
						_m_close_menu();
						state_.menu_active = true;
						state_.nullify_mouse = true;
						state_.active = index;

						if(_m_popup_menu())
							state_.menu->goto_next(true);

						_m_draw();
						API::lazy_refresh();
						state_.behavior = state_.behavior_menu;
					}
				}

				void trigger::_m_move(bool to_left)
				{
					if(items_->cont().size() == 0) return;

					std::size_t index = state_.active;
					if(to_left)
					{
						if(index > 0)
							--index;
						else
							index = items_->cont().size() - 1;
					}
					else
					{
						if(index == items_->cont().size() - 1)
							index = 0;
						else
							++index;
					}

					if(index != state_.active)
					{
						state_.active = index;
						_m_draw();
						API::lazy_refresh();

						if(_m_popup_menu())
							state_.menu->goto_next(true);
					}
				}

				bool trigger::_m_popup_menu()
				{
					if(state_.menu_active && (state_.menu != items_->get_menu(state_.active)))
					{
						std::size_t index = state_.active;
						_m_close_menu();
						state_.active = index;

						state_.menu = items_->get_menu(state_.active);
						if(state_.menu)
						{
							const item_type &m = items_->at(state_.active);
							state_.menu->destroy_answer(std::bind(&trigger::_m_unload_menu_window, this));
							state_.menu->popup(widget_->handle(), m.pos.x, m.pos.y + m.size.height, true);
							return true;
						}
					}
					return false;
				}

				void trigger::_m_total_close()
				{
					_m_close_menu();
					state_.menu_active = false;
					state_.behavior = state_.behavior_none;

					API::restore_menubar_taken_window();

					nana::point pos = API::cursor_position();
					API::calc_window_point(widget_->handle(), pos);
					state_.active = _m_item_by_pos(pos.x, pos.y);
				}

				bool trigger::_m_close_menu()
				{
					if(state_.menu)
					{
						state_.passive_close = false;
						state_.menu->close();
						state_.passive_close = true;
						state_.menu = nullptr;
						return true;
					}
					return false;
				}

				void trigger::_m_unload_menu_window()
				{
					state_.menu = nullptr;
					if(state_.passive_close)
					{
						_m_total_close();
						_m_draw();
						API::update_window(widget_->handle());
					}
				}

				std::size_t trigger::_m_item_by_pos(int x, int y)
				{
					if((2 <= x) && (2 <= y) && (y < 25))
					{
						int item_x = 2;
						std::size_t index = 0;
						for(auto i : items_->cont())
						{
							if(item_x <= x && x < item_x + static_cast<int>(i->size.width))
								return index;

							item_x += i->size.width;
							++index;
						}
					}

					return npos;
				}

				bool trigger::_m_track_mouse(int x, int y)
				{
					if(state_.nullify_mouse == false)
					{
						std::size_t which = _m_item_by_pos(x, y);
						if((which != state_.active) && (which != npos || (false == state_.menu_active)))
						{
							state_.active = which;
							return true;
						}
					}
					return false;
				}

				void trigger::_m_draw()
				{
					graph_->rectangle(color::button_face, true);

					item_renderer ird(*graph_);

					nana::point item_pos(2, 2);
					nana::size item_s(0, 23);

					unsigned long index = 0;
					for(auto i : items_->cont())
					{
						//Transform the text if it contains the hotkey character
						nana::string::value_type hotkey;
						nana::string::size_type hotkey_pos;
						nana::string text = API::transform_shortkey_text(i->text, hotkey, &hotkey_pos);

						nana::size text_s = graph_->text_extent_size(text);

						item_s.width = text_s.width + 16;

						i->pos = item_pos;
						i->size = item_s;

						item_renderer::state_t state = (index != state_.active ? ird.state_normal : (state_.menu_active ? ird.state_selected : ird.state_highlight));
						ird.background(item_pos, item_s, state);

						if(state == ird.state_selected)
						{
							int x = item_pos.x + item_s.width;
							int y1 = item_pos.y + 2, y2 = item_pos.y + item_s.height - 1;
							graph_->line(x, y1, x, y2, color::gray_border);
							graph_->line(x + 1, y1, x + 1, y2, color::button_face_shadow_end);
						}

						//Draw text, the text is transformed from orignal for hotkey character
						int text_top_off = (item_s.height - text_s.height) / 2;
						ird.caption(item_pos.x + 8, item_pos.y + text_top_off, text);

						if(hotkey)
						{
							unsigned off_w = (hotkey_pos ? graph_->text_extent_size(text, static_cast<unsigned>(hotkey_pos)).width : 0);
							nana::size hotkey_size = graph_->text_extent_size(text.c_str() + hotkey_pos, 1);
							int x = item_pos.x + 8 + off_w;
							int y = item_pos.y + text_top_off + hotkey_size.height;
							graph_->line(x, y, x + hotkey_size.width - 1, y, 0x0);
						}

						item_pos.x += i->size.width;
						++index;
					}
				}

				//struct state_type
					trigger::state_type::state_type()
						:active(npos), behavior(behavior_none), menu_active(false), passive_close(true), nullify_mouse(false), menu(nullptr)
					{}
				//end struct state_type
			//end class trigger
		}//end namespace menubar
	}//end namespace drawerbase


	//class menubar
		menubar::menubar(){}
		menubar::menubar(window wd)
		{
			create(wd);
		}

		void menubar::create(window wd)
		{
			widget_object<category::widget_tag, drawerbase::menubar::trigger>
				::create(wd, rectangle(nana::size(API::window_size(wd).width, 28)));
			API::attach_menubar(handle());
		}

		menu& menubar::push_back(const nana::string& text)
		{
			return *(get_drawer_trigger().push_back(text));
		}

		menu& menubar::at(std::size_t index) const
		{
			menu* p = get_drawer_trigger().at(index);
			if(nullptr == p)
				throw std::out_of_range("menubar::at, out of range");
			return *p;
		}

		std::size_t menubar::length() const
		{
			return get_drawer_trigger().size();
		}
	//end class menubar
}//end namespace gui
}//end namespace nana
