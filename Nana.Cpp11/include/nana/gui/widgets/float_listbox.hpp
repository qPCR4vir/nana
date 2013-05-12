/*
 *	A float_listbox Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/float_listbox.hpp
 */

#ifndef NANA_GUI_WIDGETS_FLOAT_LISTBOX_HPP
#define NANA_GUI_WIDGETS_FLOAT_LISTBOX_HPP

#include "widget.hpp"
#include <vector>

namespace nana{ namespace gui{
	namespace drawerbase{
		namespace float_listbox
		{
			//struct module_def
			//@brief: This defines a data structure used for float_listbox
			struct module_def
			{
				struct item_type
				{
					nana::paint::image img;
					nana::string text;

					item_type(const nana::string&);
				};

				std::vector<item_type> items;
				std::size_t max_items;			/// the number of items display.
				mutable std::size_t index;		/// the result of the selection.

				module_def();
			};

			class item_renderer
			{
			public:
				typedef widget&	widget_reference;
				typedef paint::graphics& graph_reference;
				enum state_t{StateNone, StateHighlighted};

				virtual ~item_renderer() = 0;
				virtual void image(bool enabled, unsigned pixels) = 0;
				virtual void render(widget_reference, graph_reference, const nana::rectangle&, const module_def::item_type&, state_t) = 0;
				virtual unsigned item_pixels(graph_reference) const = 0;
			};

			class drawer_impl;

			class trigger
				: public drawer_trigger
			{
			public:
				trigger();
				~trigger();
				drawer_impl& get_drawer_impl();
				const drawer_impl& get_drawer_impl() const;
			private:
				void bind_window(widget_reference);
				void attached(graph_reference graph);
				void detached();
				void refresh(graph_reference);
				void mouse_move(graph_reference, const eventinfo&);
				void mouse_up(graph_reference, const eventinfo&);
			private:
				class drawer_impl *drawer_;
			};
		}
	}//end namespace drawerbase

	class float_listbox
		: public widget_object<category::root_tag, drawerbase::float_listbox::trigger>
	{
		typedef widget_object<category::root_tag, drawerbase::float_listbox::trigger> base_type;
	public:
		typedef drawerbase::float_listbox::item_renderer item_renderer;
		typedef drawerbase::float_listbox::module_def module_type;

		float_listbox();
		float_listbox(window, const rectangle&);

		void set_module(const module_type&, unsigned image_pixels);
		void scroll_items(bool upwards);
		void move_items(bool upwards, bool circle);
		void renderer(item_renderer*);
		std::size_t index() const;
	};
}//end namespace gui
}

#endif
