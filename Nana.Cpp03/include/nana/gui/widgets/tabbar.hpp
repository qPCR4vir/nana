/*
 *	A Tabbar implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/tabbar.hpp
 *	@brief: A tabbar contains tab items and toolbox for scrolling, closing, selecting items.
 *
 */
#ifndef NANA_GUI_WIDGET_TABBAR_HPP
#define NANA_GUI_WIDGET_TABBAR_HPP
#include "widget.hpp"
#include "../../paint/gadget.hpp"
#include <nana/pat/cloneable.hpp>
#include <nana/any.hpp>

namespace nana{ namespace gui{
	namespace drawerbase
	{
		namespace tabbar
		{
			template<typename Tabbar>
			struct extra_events
			{
				typedef Tabbar tabbar;
				typedef typename tabbar::value_type value_type;

				nana::fn_group<void(tabbar&, value_type&)> add_tab;
				nana::fn_group<void(tabbar&, value_type&)> active;
				nana::fn_group<bool(tabbar&, value_type&)> remove;
			};

			class internal_event_trigger
			{
			public:
				virtual ~internal_event_trigger() = 0;
				virtual void add_tab(std::size_t i) = 0;
				virtual void active(std::size_t i) = 0;
				virtual bool remove(std::size_t i) = 0;
			};

			class item_renderer
			{
			public:
				typedef item_renderer item_renderer_type;
				typedef nana::paint::graphics & graph_reference;
				enum state_t{disable, normal, highlight, press};

				struct item_t
				{
					nana::rectangle r;
					nana::color_t	bgcolor;
					nana::color_t	fgcolor;
				};

				virtual ~item_renderer() = 0;
				virtual void background(graph_reference, const nana::rectangle& r, nana::color_t bgcolor) = 0;
				virtual void item(graph_reference, const item_t&, bool active, state_t) = 0;
				virtual void close_fly(graph_reference, const nana::rectangle&, bool active, state_t) = 0;

				virtual void add(graph_reference, const nana::rectangle&, state_t) = 0;
				virtual void close(graph_reference, const nana::rectangle&, state_t) = 0;
				virtual void back(graph_reference, const nana::rectangle&, state_t) = 0;
				virtual void next(graph_reference, const nana::rectangle&, state_t) = 0;
				virtual void list(graph_reference, const nana::rectangle&, state_t) = 0;
			};

			template<typename Tabbar, typename DrawerTrigger>
			class event_adapter
				: public internal_event_trigger
			{
			public:
				typedef Tabbar tabbar;
				typedef DrawerTrigger drawer_trigger;
				typedef typename tabbar::value_type value_type;

				mutable extra_events<tabbar> ext_event;

				event_adapter(tabbar& tb, drawer_trigger & dtr)
					: tabbar_(tb), drawer_trigger_(dtr)
				{}

				void add_tab(std::size_t pos)
				{
					if((ext_event.add_tab.empty() == false) && (pos != npos))
					{
						drawer_trigger_.at_no_bound_check(pos) = value_type();
						ext_event.add_tab(tabbar_, tabbar_[pos]);
					}
				}

				void active(std::size_t pos)
				{
					if(pos != npos)
						ext_event.active(tabbar_, tabbar_[pos]);
				}

				bool remove(std::size_t pos)
				{
					return ((ext_event.remove.empty() == false) && (pos != npos) ? ext_event.remove(tabbar_, tabbar_[pos]) : true);
				}
			private:
				tabbar & tabbar_;
				drawer_trigger& drawer_trigger_;
			};

			class layouter;

			class trigger
				: public nana::gui::drawer_trigger
			{
			public:
				enum toolbox_button_t{ButtonAdd, ButtonScroll, ButtonList, ButtonClose};
				trigger();
				~trigger();
				void active(std::size_t i);
				std::size_t active() const;
				nana::any& at(std::size_t i) const;
				nana::any& at_no_bound_check(std::size_t i) const;
				pat::cloneable_interface<item_renderer> & ext_renderer() const;
				void ext_renderer(const pat::cloneable_interface<item_renderer>&);
				void event_adapter(internal_event_trigger*);
				void push_back(const nana::string&, const nana::any&);
				layouter& layouter_object();
				std::size_t length() const;
				bool close_fly(bool);
				void relate(size_t, nana::gui::window);
				void tab_color(std::size_t i, bool is_bgcolor, nana::color_t);
				void tab_image(size_t, const nana::paint::image&);
				void text(std::size_t i, const nana::string&);
				nana::string text(std::size_t i) const;
				bool toolbox_button(toolbox_button_t, bool);
			private:
				void bind_window(widget_reference);
				void attached(graph_reference);
				void detached();
				void refresh(graph_reference);
				void mouse_down(graph_reference, const eventinfo&);
				void mouse_up(graph_reference, const eventinfo&);
				void mouse_move(graph_reference, const eventinfo&);
				void mouse_leave(graph_reference, const eventinfo&);
			private:
				layouter * layouter_;
			};
		}
	}//end namespace drawerbase

	template<typename Type>
	class tabbar
		: public widget_object<category::widget_tag, drawerbase::tabbar::trigger>
	{
	public:
		typedef Type value_type;
		typedef drawerbase::tabbar::item_renderer item_renderer;
		typedef drawerbase::tabbar::extra_events<tabbar> ext_event_type;

		struct button_add{};
		struct button_scroll{};
		struct button_list{};
		struct button_close{};

		template<typename ItemRenderer>
		class renderer_cloneable
			: public nana::pat::cloneable<ItemRenderer, item_renderer>
		{
		public:
			renderer_cloneable(){}
			renderer_cloneable(const ItemRenderer& ir)
				: pat::cloneable<ItemRenderer, item_renderer>(ir)
			{}

			template<typename U>
			renderer_cloneable(U& u)
				: pat::cloneable<ItemRenderer, item_renderer>(u)
			{}

			template<typename U>
			renderer_cloneable(const U& u)
				: pat::cloneable<ItemRenderer, item_renderer>(u)
			{}
		};

		template<typename ButtonAdd = nana::null_type, typename ButtonScroll = nana::null_type, typename ButtonList = nana::null_type, typename ButtonClose = nana::null_type>
		struct button_container
		{
			typedef metacomp::fixed_type_set<ButtonAdd, ButtonScroll, ButtonList, ButtonClose> type_set;
		};

		tabbar()
		{
			_m_init();
		}

		tabbar(window wd, bool visible)
		{
			_m_init();
			this->create(wd, rectangle(), visible);
		}

		tabbar(window wd, const rectangle& r = rectangle(), bool visible = true)
		{
			_m_init();
			this->create(wd, r, visible);
		}

		~tabbar()
		{
			get_drawer_trigger().event_adapter(0);
			delete event_adapter_;
		}

		value_type & operator[](std::size_t i) const
		{
			nana::any & value = get_drawer_trigger().at_no_bound_check(i);
			return static_cast<value_type&>(value);
		}

		void active(std::size_t i)
		{
			get_drawer_trigger().active(i);
		}

		std::size_t active() const
		{
			return get_drawer_trigger().active();
		}

		value_type & at(std::size_t i) const
		{
			nana::any & value = get_drawer_trigger().at(i);
			return static_cast<value_type&>(value);
		}

		void close_fly(bool fly)
		{
			if(get_drawer_trigger().close_fly(fly))
				API::refresh_window(this->handle());
		}

		pat::cloneable_interface<item_renderer>& ext_renderer() const
		{
			return get_drawer_trigger().ext_renderer();
		}

		void ext_renderer(const pat::cloneable_interface<item_renderer>& ir)
		{
			get_drawer_trigger().ext_renderer(ir);
		}

		ext_event_type& ext_event() const
		{
			return event_adapter_->ext_event;
		}

		std::size_t length() const
		{
			return get_drawer_trigger().length();
		}

		void push_back(const nana::string& text)
		{
			drawer_trigger_t & t = get_drawer_trigger();
			t.push_back(text, value_type());
			nana::gui::API::update_window(*this);
		}

		void relate(std::size_t pos, nana::gui::window wd)
		{
			get_drawer_trigger().relate(pos, wd);
		}

		void tab_bgcolor(std::size_t i, nana::color_t color)
		{
			get_drawer_trigger().tab_color(i, true, color);
		}

		void tab_fgcolor(std::size_t i, nana::color_t color)
		{
			get_drawer_trigger().tab_color(i, false, color);
		}

		void tab_image(std::size_t i, const nana::paint::image& img)
		{
			get_drawer_trigger().tab_image(i, img);
		}

		template<typename Add, typename Scroll, typename List, typename Close>
		void toolbox(const button_container<Add, Scroll, List, Close>&, bool enable)
		{
			typedef typename button_container<Add, Scroll, List, Close>::type_set type_set;
			drawer_trigger_t & tg = get_drawer_trigger();
			bool redraw = false;

			if(type_set::template count<button_add>::value)
				redraw |= tg.toolbox_button(tg.ButtonAdd, enable);

			if(type_set::template count<button_scroll>::value)
				redraw |= tg.toolbox_button(tg.ButtonScroll, enable);

			if(type_set::template count<button_list>::value)
				redraw |= tg.toolbox_button(tg.ButtonList, enable);

			if(type_set::template count<button_close>::value)
				redraw |= tg.toolbox_button(tg.ButtonClose, enable);

			if(redraw)
				nana::gui::API::refresh_window(this->handle());
		}

		void text(std::size_t pos, const nana::string& str)
		{
			get_drawer_trigger().text(pos, str);
		}

		nana::string text(std::size_t pos) const
		{
			return get_drawer_trigger().text(pos);
		}
	private:
		void _m_init()
		{
			event_adapter_ = new drawerbase::tabbar::event_adapter<tabbar, drawer_trigger_t>(*this, get_drawer_trigger());
			get_drawer_trigger().event_adapter(event_adapter_);
		}
	private:
		drawerbase::tabbar::event_adapter<tabbar, drawer_trigger_t> * event_adapter_;
	};
}//end namespace gui
}
#endif
