#include <nana/gui/widgets/login.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/checkbox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/tooltip.hpp>
#include <vector>

namespace nana{ namespace gui{ namespace drawerbase{ namespace login
{
	struct item_t
	{
		int pixels;
		bool have_user;	//indicates whether the item has an user name initialized.

		nana::paint::image img;

		nana::rectangle close;

		nana::string init_user_string;
		nana::string init_pswd_string;

		gui::textbox user;
		gui::textbox password;
		gui::label	forget;
		gui::checkbox remember_user;
		gui::checkbox remember_password;

		item_t(const nana::string& init_user, const nana::string& init_password, const nana::paint::image& img)
			:	have_user(init_user.size() != 0), img(img),
				init_user_string(init_user), init_pswd_string(have_user ? init_password : STR(""))

		{}
	};

	class item_renderer
	{
	public:
		typedef nana::paint::graphics & graph_reference;

		struct state_flags
		{
			bool have_user;
			bool highlight;
			bool select;
		};

		const static int xpos = 5;

		bool place_user(int top, int width, int pixels, nana::rectangle & r, bool & is_show_tipstr)
		{
			if(!flag_.have_user)
			{
				r.x = xpos;
				r.y = top + 20;
				r.width = width - xpos * 2;
				r.height = 24;
				is_show_tipstr = false;
				return true;
			}
			is_show_tipstr = true;
			return false;
		}

		bool place_password(int top, int width, int pixels, nana::rectangle & r, bool & is_show_tipstr)
		{
			if(flag_.select)
			{
				r.x = xpos;
				r.width = width - xpos * 2;
				r.height = 24;
				if(flag_.have_user)
					r.y = top + 33;
				else
					r.y = top + 74;

				is_show_tipstr = flag_.have_user;

				pswd_top_ = r.y;

				return true;
			}
			return false;
		}

		bool place_login(int top, int width, int pixels, nana::rectangle& r)
		{
			if(!flag_.have_user)
			{
				r.x = xpos;
				r.y = pswd_top_ + 118;
				r.width = 50;
				r.height = 22;
				return true;
			}
			else
			{
				if(flag_.select)
				{
					r.x = width - 50;
					r.y = top + pixels - 25;
					r.width = 45;
					r.height = 20;
					return true;
				}
			}
			return false;
		}

		bool place_forget(int top, int width, int pixels, nana::rectangle& r)
		{
			if(flag_.select)
			{
				r.x = xpos;
				r.y = pswd_top_ + 24;
				r.width = 160;
				r.height = 20;
				return true;
			}
			return false;
		}

		bool place_remember_user(int top, int width, int pixels, nana::rectangle& r)
		{
			if(!flag_.have_user)
			{
				r.x = xpos;
				r.y = pswd_top_ + 58;
				r.width = 200;
				r.height = 20;
				return true;
			}
			return false;
		}

		bool place_remember_password(int top, int width, int pixels, nana::rectangle& r)
		{
			if(flag_.select || !flag_.have_user)
			{
				r.x = xpos;
				r.width = 180;
				r.height = 20;
				r.y = pswd_top_ + 58;
				if(!flag_.have_user)
					r.y += 24;
				return true;
			}
			return false;
		}

		bool place_close(int top, int width, int pixels, nana::rectangle& r)
		{
			if(flag_.highlight && flag_.have_user)
			{
				r.x = width - 18;
				r.y = top + 2;
				r.width = 16;
				r.height = 16;
				return true;
			}
			return false;
		}


		int render(graph_reference graph, const gui::login::label_strings_t& lbstr, int top, unsigned width, const nana::string& user, const nana::paint::image& img, const state_flags& sf)
		{
			flag_ = sf;
			if(false == flag_.have_user)
			{
				graph.string(xpos, 0, 0x0, lbstr.user);
				graph.string(xpos, 56, 0x0, lbstr.password);
				return static_cast<int>(graph.height());
			}

			unsigned height = 40;
			if(flag_.select)
			{
				height = 128;
				flag_.highlight = true;
			}

			nana::size ts = graph.text_extent_size(user);
			if(flag_.highlight)
			{
				graph.rectangle(0, top, width, height, 0x7DA2CE, false);
				graph.shadow_rectangle(1, top + 1, width - 2, height - 2, 0xDCEBFD, 0xC2DCFD, true);
			}

			const int scale = 32;
			if(img.empty() == false)
				img.paste(nana::rectangle(0, 0, 32, 32), graph, nana::point(xpos + 10, top + 10 + (static_cast<int>(ts.height) - scale) / 2));

			if(flag_.have_user)
				graph.string(xpos + 60, top + 10, 0x0, user);

			return height;
		}
	private:
		state_flags flag_;
		int pswd_top_;
	};

	//class trigger::drawer
	class trigger::drawer
	{
	public:
		struct component
		{
			enum type{none, item, up, down};
			type which;
			std::size_t item_index;
			bool is_close;

			component()
				: which(none), item_index(npos), is_close(false)
			{}

			bool operator==(const component & rhs) const
			{
				return (which == rhs.which) && ((which != item) || ((item_index == rhs.item_index) && (is_close == rhs.is_close)));
			}

			bool operator!=(const component& rhs) const
			{
				return !this->operator==(rhs);
			}
		};

		mutable extra_events ext_event;

		drawer()
		{
			lbstrings_.user = STR("Account:");
			lbstrings_.password = STR("Password:");
			lbstrings_.login = STR("Login");
			lbstrings_.forget = STR("Forget the password?");
			lbstrings_.remember_user = STR("Remember my information");
			lbstrings_.remember_password = STR("Remember my password");
			lbstrings_.remove = STR("Don't remember this user");
			lbstrings_.require_user = STR("Please enter your username.");
			lbstrings_.require_password = STR("Please type a value for password.");
			lbstrings_.other_user = STR("Login by using other account");

			btn_login_.hide();
			btn_login_.caption(lbstrings_.login);
		}

		bool selection(bool sl)
		{
			if(mode_.valid == sl)
			{
				mode_.valid = !sl;
				return true;
			}
			return false;
		}

		bool selection() const
		{
			return (mode_.valid != true);
		}

		void bind_login_object(class login& obj)
		{
			other_.login_object = &obj;
		}

		void attached(nana::paint::graphics& graph)
		{
			other_.graph = &graph;
		}

		void bind(nana::gui::widget& wd)
		{
			other_.wd  = &wd;
			_m_init_widgets(wd);
		}

		nana::gui::widget* widget() const
		{
			return other_.wd;
		}

		void reset()
		{
			_m_enable(_m_op_item(), true);
			btn_login_.enabled(true);
		}

		void detached()
		{
			other_.graph = 0;
		}

		void insert(const nana::string& user, const nana::string& password, const nana::paint::image& img)
		{
			if(user.size())
			{
				for(std::vector<item_t*>::iterator i = container_.begin(); i < container_.end(); ++i)
				{
					if(user == (*i)->user.caption())
					{
						(*i)->password.caption(password);
						(*i)->img = img;
						return;
					}
				}

				item_t * m = new item_t(user, password, img);
				if(other_.wd)
				{
					_m_init_widget(m);
				}

				container_.push_back(m);
				mode_.valid = false;
			}
		}

		void draw()
		{
			if(false == API::glass_window(other_.wd->handle()))
				other_.graph->rectangle(other_.wd->background(), true);

			nana::size gsize = other_.graph->size();
			item_renderer renderer;
			nana::rectangle login_rectangle;

			int top = 0;
			if(mode_.valid)
			{
				for(std::vector<item_t*>::iterator i = container_.begin(); i != container_.end(); ++i)
					_m_hide(*i);

				_m_draw_item(renderer, mode_.item, 0, login_rectangle, 0, gsize);
				top += mode_.item->pixels;
			}
			else
			{
				_m_hide(mode_.item);
				if(item_state_.index >= container_.size()) item_state_.index = 0;

				std::vector<item_t*>::iterator end = container_.begin() + item_state_.index;
				for(std::vector<item_t*>::iterator i = container_.begin(); i != end; ++i)
				{
					_m_hide(*i);
				}

				for(std::vector<item_t*>::iterator i = container_.begin() + item_state_.index; i != container_.end(); ++i)
				{
					item_t * m = *i;
					if(top < static_cast<int>(gsize.width))
					{
						_m_draw_item(renderer, m, i - container_.begin(), login_rectangle, top, gsize);
						top += m->pixels;
					}
					else
						_m_hide(m);
				}
			}

			if(login_rectangle.width && login_rectangle.height)
			{
				btn_login_.move(login_rectangle.x, login_rectangle.y);
				btn_login_.size(login_rectangle.width, login_rectangle.height);
				btn_login_.show();
			}
			else
				btn_login_.hide();

			if(false == mode_.valid)
			{
				lb_login_other_.move(0, top + 5);
				lb_login_other_.show();
			}
			else
				lb_login_other_.hide();
		}

		bool cancel_highlight()
		{
			if(trace_.which != trace_.none)
			{
				if(trace_.which == trace_.item)
				{
					trace_.item_index = npos;
					trace_.is_close = false;
				}
				trace_.which = trace_.none;
				tooltip_.close();
				return true;
			}
			return false;
		}

		bool trace_by_mouse(int x, int y)
		{
			component comp;
			comp.which = comp.none;
			comp.item_index = npos;

			nana::size sz = _m_item_area();
			if(x < static_cast<int>(sz.width))
			{
				int top = 0;
				for(std::vector<item_t*>::const_iterator i = container_.begin() + item_state_.index; i != container_.end(); ++i)
				{
					if(top < static_cast<int>(sz.height))
					{
						if(top <= y && y <= top + (*i)->pixels)
						{
							comp.which = comp.item;
							comp.item_index = i - container_.begin();
							const nana::rectangle & r = (*i)->close;
							if(r.width && r.height)
								comp.is_close = (r.x <= x && x < static_cast<int>(r.x + r.width)) && (r.y <= y && y <= static_cast<int>(r.y + r.height));
							else
								comp.is_close = false;
							break;
						}
					}
					else
						break;

					top += (*i)->pixels;
				}
			}
			if(comp != trace_)
			{
				trace_ = comp;
				if(comp.which == comp.item && comp.is_close)
					tooltip_.show(other_.wd->handle(), x, y + 16, lbstrings_.remove);
				else
					tooltip_.close();
				return true;
			}
			return false;
		}

		bool active()
		{
			//Test if click on the close
			if(trace_.which == trace_.item && trace_.is_close)
			{
				if(trace_.item_index < container_.size())
				{
					std::vector<item_t*>::iterator i = container_.begin() + trace_.item_index;
					item_t * m = *i;

					if(other_.login_object)
					{
						ext_event.remove(*other_.login_object, m->init_user_string);
					}
					container_.erase(i);
					delete m;

					if(item_state_.select != npos)
					{
						if(item_state_.select == trace_.item_index)
							item_state_.select = npos;
						else if(item_state_.select > trace_.item_index)
							--item_state_.select;
					}
				}

				if(container_.empty())
				{
					btn_login_.hide();
					tooltip_.close();
					mode_.valid = true;
				}
				return true;
			}

			if(item_state_.active != trace_)
			{
				item_state_.active = trace_;
				if(trace_.which == trace_.item)
					item_state_.select = trace_.item_index;
				else
					item_state_.select = npos;
				return true;
			}
			return false;
		}

		void lbstr(const label_strings& lbs)
		{
			if(lbs.user.size())
				lbstrings_.user = lbs.user;
			if(lbs.password.size())
				lbstrings_.password = lbs.password;
			if(lbs.forget.size())
				lbstrings_.forget = lbs.forget;
			if(lbs.remember_user.size())
				lbstrings_.remember_user = lbs.remember_user;
			if(lbs.remember_password.size())
				lbstrings_.remember_password = lbs.remember_password;
			if(lbs.login.size())
				lbstrings_.login = lbs.login;
			if(lbs.remove.size())
				lbstrings_.remove = lbs.remove;
			if(lbs.require_user.size())
				lbstrings_.require_user = lbs.require_user;
			if(lbs.require_password.size())
				lbstrings_.require_password = lbs.require_password;
			if(lbs.other_user.size())
				lbstrings_.other_user = lbs.other_user;

			item_t * m = mode_.item;
			if(lbs.user.size())
			{
				m->user.tip_string(lbs.user);
				m->password.tip_string(lbs.password);
			}
			if(lbs.remember_user.size())
				m->remember_user.caption(lbs.remember_user);
			if(lbs.remember_password.size())
				m->remember_password.caption(lbs.remember_password);

			if(lbs.forget.size())
				m->forget.caption(lbs.forget);

			for(std::vector<item_t*>::iterator i = container_.begin(); i != container_.end(); ++i)
			{
				m = *i;
				if(lbs.user.size())
				{
					m->user.tip_string(lbs.user);
					m->password.tip_string(lbs.password);
				}
				if(lbs.remember_user.size())
					m->remember_user.caption(lbs.remember_user);
				if(lbs.remember_password.size())
					m->remember_password.caption(lbs.remember_password);

				if(lbs.forget.size())
					m->forget.caption(lbs.forget);
			}
			if(lbs.login.size())
				btn_login_.caption(lbs.login);

			if(lbs.other_user.size())
				lb_login_other_.caption(lbs.other_user);

			if(other_.wd)
			{
				this->draw();
				API::update_window(other_.wd->handle());
			}
		}
	private:
		item_t * _m_op_item()
		{
			if(mode_.valid)
				return mode_.item;

			if(item_state_.select >= container_.size())
			{
				item_state_.select = 0;
			}

			return container_[item_state_.select];
		}

		void _m_draw_item(item_renderer& renderer, item_t* m, std::size_t index, nana::rectangle& login_rectangle, int top, const nana::size& gsize)
		{
			nana::rectangle r;
			item_renderer::state_flags flag;
			flag.have_user = m->have_user;
			flag.highlight = (trace_.which == trace_.item && (index == trace_.item_index));

			flag.select = (index == item_state_.select || !m->have_user);

			m->pixels = renderer.render(*other_.graph, lbstrings_, top, gsize.width, m->user.caption(), m->img, flag);
			bool is_show_tipstr;
			if(renderer.place_user(top, gsize.width, m->pixels, r, is_show_tipstr))
			{
				m->user.move(r.x, r.y);
				m->user.size(r.width, r.height);
				m->user.show();
			}
			else
				m->user.hide();
			m->user.tip_string(is_show_tipstr ? lbstrings_.user : nana::string(STR("")));

			if(renderer.place_password(top, gsize.width, m->pixels, r, is_show_tipstr))
			{
				m->password.move(r.x, r.y);
				m->password.size(r.width, r.height);
				m->password.show();
			}
			else
				m->password.hide();
			m->password.tip_string(is_show_tipstr ? lbstrings_.password : nana::string(STR("")));

			if(renderer.place_forget(top, gsize.width, m->pixels, r))
			{
				m->forget.move(r.x, r.y);
				m->forget.size(r.width, r.height);
				m->forget.show();
			}
			else
				m->forget.hide();

			if(renderer.place_remember_user(top, gsize.width, m->pixels, r))
			{
				m->remember_user.move(r.x, r.y);
				m->remember_user.size(r.width, r.height);
				m->remember_user.show();
			}
			else
				m->remember_user.hide();

			if(renderer.place_remember_password(top, gsize.width, m->pixels, r))
			{
				m->remember_password.move(r.x, r.y);
				m->remember_password.size(r.width, r.height);
				m->remember_password.show();
			}
			else
				m->remember_password.hide();

			if(renderer.place_close(top, gsize.width, m->pixels, r))
			{
				m->close = r;
				nana::paint::gadget::close_16_pixels(*other_.graph, r.x, r.y, 0, 0x0);
			}
			else
				m->close.width = m->close.height = 0;

			if(renderer.place_login(top, gsize.width, m->pixels, r))
				login_rectangle = r;
		}

		nana::size _m_item_area() const
		{
			return other_.graph->size();
		}

		void _m_hide(item_t * m)
		{
			m->pixels = 0;
			m->user.hide();
			m->password.hide();
			m->forget.hide();
			m->remember_user.hide();
			m->remember_password.hide();
		}

		void _m_enable(item_t *m, bool enb)
		{
			if(m)
			{
				m->user.enabled(enb);
				m->password.enabled(enb);
				m->remember_user.enabled(enb);
				m->remember_password.enabled(enb);
				m->forget.enabled(enb);
			}
		}

		void _m_do_verify(const gui::eventinfo& ei)
		{
			tooltip_.close();
			if(ei.keyboard.key == nana::gui::keyboard::enter)
			{
				_m_verify();
			}
		}

		void _m_verify()
		{
			item_t * m = _m_op_item();

			nana::string user = m->user.caption();
			nana::string pass = m->password.caption();

			if(user.size() == 0)
			{
				nana::gui::API::focus_window(m->user.handle());
				nana::point pos = m->user.pos();
				tooltip_.show(other_.wd->handle(), pos.x, pos.y + m->user.size().height, lbstrings_.require_user);
			}
			else if(pass.size() == 0)
			{
				nana::gui::API::focus_window(m->password.handle());
				nana::point pos = m->password.pos();
				tooltip_.show(other_.wd->handle(), pos.x, pos.y + m->password.size().height, lbstrings_.require_password);
			}

			if(other_.login_object && (ext_event.verify.empty() == false) && user.size() && pass.size())
			{
				_m_enable(m, false);
				btn_login_.enabled(false);

				extra_events::flags_t fg;
				fg.remember_user = m->remember_user.checked();
				fg.remember_password = m->remember_password.checked();
				ext_event.verify(*other_.login_object, user, pass, fg);

				if(fg.remember_password == false)
					m->password.caption(STR(""));
			}
		}

		void _m_forget()
		{
			ext_event.forget(*other_.login_object, _m_op_item()->user.caption());
		}

		void _m_take_check(const eventinfo& ei)
		{
			item_t * m = _m_op_item();
			if(m)
			{
				if(m->remember_password.handle() == ei.window)
				{
					if(m->remember_password.checked())
						m->remember_user.check(true);
				}
				else if(m->remember_user.handle() == ei.window)
				{
					if(m->remember_user.checked() == false)
						m->remember_password.check(false);
				}
			}
		}

		void _m_login_for_other_user()
		{
			this->selection(false);
			this->draw();
			if(other_.wd)
				API::update_window(other_.wd->handle());
		}

		void _m_init_widgets(nana::gui::widget& wd)
		{
			if(false == wd.empty())
			{
				for(std::vector<item_t*>::iterator i = container_.begin(); i != container_.end(); ++i)
				{
					item_t * m = *i;
					this->_m_init_widget(m);
				}

				this->_m_init_widget(mode_.item);
				mode_.valid = container_.empty();

				btn_login_.create(wd);
				btn_login_.caption(lbstrings_.login);
				btn_login_.make_event<events::click>(*this, &drawer::_m_verify);

				lb_login_other_.create(wd, nana::size(wd.size().width, 20));
				lb_login_other_.transparent(true);
				lb_login_other_.caption(lbstrings_.other_user);
				lb_login_other_.make_event<events::click>(*this, &drawer::_m_login_for_other_user);
				lb_login_other_.foreground(0x66CC);
				//lb_login_other_.hide();
			}
		}

		void _m_init_widget(item_t * m)
		{
			if(other_.wd && other_.wd->empty() && m && m->user.empty())
				return;

			nana::gui::widget& wd = *other_.wd;
			m->pixels = 0;
			m->user.create(wd);
			m->user.multi_lines(false);
			m->user.caption(m->init_user_string);
			m->user.tip_string(lbstrings_.user);
			API::eat_tabstop(m->user.handle(), false);
			m->user.make_event<events::key_char>(*this, &drawer::_m_do_verify);


			m->password.create(wd);
			m->password.multi_lines(false);
			m->password.caption(m->init_pswd_string);
			m->password.tip_string(lbstrings_.password);
			m->password.mask('*');
			API::eat_tabstop(m->password.handle(), false);
			m->password.make_event<events::key_char>(*this, &drawer::_m_do_verify);


			m->forget.create(wd);
			m->forget.caption(lbstrings_.forget);
			m->forget.foreground(0x66CC);
			m->forget.transparent(true);
			m->forget.make_event<events::click>(*this, &drawer::_m_forget);

			m->remember_user.create(wd);
			m->remember_user.caption(lbstrings_.remember_user);
			m->remember_user.transparent(true);
			m->remember_user.check(m->have_user);
			m->remember_user.make_event<events::click>(*this, &drawer::_m_take_check);

			m->remember_password.create(wd);
			m->remember_password.caption(lbstrings_.remember_password);
			m->remember_password.transparent(true);
			m->remember_password.check(m->init_pswd_string.size() != 0);
			m->remember_password.make_event<events::click>(*this, &drawer::_m_take_check);

		}
	private:
		struct other_tag
		{
			class login * login_object;
			nana::gui::widget * wd;
			nana::paint::graphics * graph;

			other_tag()
				: login_object(0), wd(0), graph(0)
			{}
		}other_;

		struct item_state_tag
		{
			std::size_t index;	//determine the first item wound be displayed.
			std::size_t select;
			component active;

			item_state_tag()
				: index(0), select(npos)
			{
			}
		}item_state_;

		struct mode_tag
		{
			item_t * item;
			bool valid;

			mode_tag()
				: item(new item_t(STR(""), STR(""), nana::paint::image())), valid(true)
			{}
		}mode_;

		component trace_;

		label_strings lbstrings_;
		nana::gui::button btn_login_;
		nana::gui::label	lb_login_other_;
		std::vector<item_t*> container_;
		nana::gui::tooltip tooltip_;
	};

	//class trigger
	trigger::trigger()
		:impl_(new drawer)
	{}

	trigger::~trigger()
	{
		delete impl_;
	}
	trigger::drawer* trigger::get_drawer() const
	{
		return impl_;
	}

	void trigger::bind_window(widget_reference wd)
	{
		impl_->bind(wd);
	}

	void trigger::attached(graph_reference graph)
	{
		impl_->attached(graph);
		window wd = impl_->widget()->handle();
		using namespace API::dev;
		make_drawer_event<events::mouse_move>(wd);
		make_drawer_event<events::mouse_up>(wd);
		make_drawer_event<events::mouse_leave>(wd);
	}

	void trigger::detached()
	{
		API::dev::umake_drawer_event(impl_->widget()->handle());
		impl_->detached();
	}

	void trigger::refresh(graph_reference)
	{
		impl_->draw();
	}

	void trigger::mouse_move(graph_reference, const eventinfo& ei)
	{
		if(impl_->trace_by_mouse(ei.mouse.x, ei.mouse.y))
		{
			impl_->draw();
			API::lazy_refresh();
		}
	}

	void trigger::mouse_up(graph_reference, const eventinfo& ei)
	{
		if(impl_->active())
		{
			impl_->draw();
			API::lazy_refresh();
		}
	}

	void trigger::mouse_leave(graph_reference, const eventinfo& ei)
	{
		if(impl_->cancel_highlight())
		{
			impl_->draw();
			API::lazy_refresh();
		}
	}
	//end class trigger

}//end namespace login
}//end namespace drawerbase

	login::login()
	{
		this->get_drawer_trigger().get_drawer()->bind_login_object(*this);
	}

	login::login(window wd, bool visible)
	{
		get_drawer_trigger().get_drawer()->bind_login_object(*this);
		this->create(wd, rectangle(), visible);
	}

	login::login(window wd, const rectangle& r, bool visible)
	{
		this->get_drawer_trigger().get_drawer()->bind_login_object(*this);
		this->create(wd, r, visible);
	}

	void login::selection(bool sl)
	{
		if(this->get_drawer_trigger().get_drawer()->selection(sl))
			API::refresh_window(this->handle());
	}

	bool login::selection() const
	{
		return get_drawer_trigger().get_drawer()->selection();
	}

	bool login::transparent() const
	{
		return API::glass_window(this->handle());
	}

	void login::transparent(bool t)
	{
		if(API::glass_window(this->handle(), t))
			API::refresh_window(this->handle());
	}

	void login::insert(const nana::string& user, const nana::string& password)
	{
		get_drawer_trigger().get_drawer()->insert(user, password, nana::paint::image());
		API::refresh_window(this->handle());
	}

	void login::insert(const nana::string& user, const nana::string& password, const nana::paint::image& img)
	{
		get_drawer_trigger().get_drawer()->insert(user, password, img);
		API::refresh_window(this->handle());
	}

	login::ext_event_type& login::ext_event() const
	{
		return get_drawer_trigger().get_drawer()->ext_event;
	}

	void login::set(const login::label_strings_t& lbstr)
	{
		get_drawer_trigger().get_drawer()->lbstr(lbstr);
	}

	void login::reset()
	{
		get_drawer_trigger().get_drawer()->reset();
	}

}//end namespace gui
}//end namespace nana
