/*
 *	A Categorize Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/categorize.hpp
 */

#ifndef NANA_GUI_WIDGET_CATEGORIZE_HPP
#define NANA_GUI_WIDGET_CATEGORIZE_HPP

#include <nana/gui/widgets/widget.hpp>
#include <nana/pat/cloneable.hpp>
#include <nana/any.hpp>

namespace nana{	namespace gui
{
	namespace drawerbase
	{
		namespace categorize
		{
			template<typename CategObj>
			struct extra_events
			{
				typedef CategObj categorize_type;
				typedef typename categorize_type::value_type value_type;
				nana::fn_group<void(categorize_type&, value_type&)> selected;
			};

			class ext_event_adapter_if
			{
			public:
				virtual ~ext_event_adapter_if() = 0;
				virtual void selected(nana::any&) = 0;
			};

			template<typename CategObj>
			class ext_event_adapter
				: public ext_event_adapter_if
			{
			public:
				typedef typename CategObj::value_type value_type;
				ext_event_adapter(CategObj & obj)
					: categ_obj_(obj)
				{}

				extra_events<CategObj>& ext_event() const
				{
					return ext_event_;
				}
			private:
				void selected(nana::any& anyobj)
				{
					value_type * p = anyobj.get<typename CategObj::value_type>();
					if(0 == p)
					{
						value_type nullval;
						ext_event_.selected(categ_obj_, nullval);
					}
					else
						ext_event_.selected(categ_obj_, *p);
				}
			private:
				CategObj & categ_obj_;
				mutable extra_events<CategObj> ext_event_;
			};

			class renderer
			{
			public:
				typedef nana::paint::graphics & graph_reference;

				struct ui_element
				{
					enum t
					{
						none,	//Out of the widget
						somewhere, item_root, item_name, item_arrow
					};

					t what;
					std::size_t index;

					ui_element();
				};

				virtual ~renderer() = 0;
				virtual void background(graph_reference, window wd, const nana::rectangle&, const ui_element&) = 0;
				virtual void root_arrow(graph_reference, const nana::rectangle&, mouse_action) = 0;
				virtual void item(graph_reference, const nana::rectangle&, std::size_t index, const nana::string& name, unsigned textheight, bool has_child, mouse_action) = 0;
				virtual void border(graph_reference) = 0;
			};

			class trigger
				: public drawer_trigger
			{
				class scheme;
			public:
				typedef renderer::ui_element ui_element;

				trigger();
				~trigger();

				void insert(const nana::string&, nana::any);
				bool childset(const nana::string&, nana::any);
				bool childset_erase(const nana::string&);
				bool clear();

				//splitstr
				//@brief: Sets the splitstr. If the parameter will be ingored if it is an empty string.
				void splitstr(const nana::string&);
				const nana::string& splitstr() const;
				
				void path(const nana::string&);
				nana::string path() const;

				template<typename CategObj>
				ext_event_adapter<CategObj>* ref_adapter(CategObj& obj) const
				{
					if(0 == ext_event_adapter_)
					{
						ext_event_adapter_ = new ext_event_adapter<CategObj>(obj);
						_m_attach_adapter_to_drawer();
					}
					//categorize guarantees ext_event_adapter refers to the object of ext_event_adapter<CategObj>
					return static_cast<ext_event_adapter<CategObj>*>(ext_event_adapter_);
				}

				nana::any & value() const;
			private:
				void _m_attach_adapter_to_drawer() const;
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
				mutable ext_event_adapter_if * ext_event_adapter_;
				scheme * scheme_;
			};
		}//end namespace categorize
	}//end namespace drawerbase

	template<typename T>
	class categorize
		: public widget_object<category::widget_tag, drawerbase::categorize::trigger>
	{
	public:
		typedef T value_type;
		typedef drawerbase::categorize::extra_events<categorize> ext_event_type;
		typedef drawerbase::categorize::renderer renderer;

		template<typename ExtRenderer>
		class renderer_cloneable
			: public pat::cloneable<ExtRenderer, renderer>
		{
		public:
			renderer_cloneable(){}
			renderer_cloneable(const ExtRenderer& rnd)
				: pat::cloneable<ExtRenderer, renderer>(rnd)
			{}
		};

		categorize()
		{}

		categorize(window wd, bool visible)
		{
			this->create(wd, rectangle(), visible);
		}

		categorize(window wd, const rectangle& r = rectangle(), bool visible = true)
		{
			this->create(wd, r, visible);
		}

		ext_event_type& ext_event() const
		{
			return get_drawer_trigger().ref_adapter(const_cast<categorize&>(*this))->ext_event();
		}

		categorize& insert(const nana::string& name, const value_type& value)
		{
			get_drawer_trigger().insert(name, value);
			API::update_window(*this);
			return *this;
		}

		categorize& childset(const nana::string& name, const value_type& value)
		{
			if(get_drawer_trigger().childset(name, value))
				API::update_window(*this);
			return *this;
		}

		categorize& childset_erase(const nana::string& name)
		{
			if(get_drawer_trigger().childset_erase(name))
				API::update_window(*this);
			return *this;
		}

		void clear()
		{
			if(get_drawer_trigger().clear())
				API::update_window(*this);
		}

		categorize& splitstr(const nana::string& sstr)
		{
			get_drawer_trigger().splitstr(sstr);
			return *this;
		}

		nana::string splitstr() const
		{
			return get_drawer_trigger().splitstr();
		}

		value_type& value() const
		{
			return get_drawer_trigger().value();
		}
	private:
		void _m_caption(const nana::string& str)
		{
			get_drawer_trigger().path(str);
			API::dev::window_caption(*this, get_drawer_trigger().path());
		}
	};
}//end namespace gui
}//end namespace nana

#endif
