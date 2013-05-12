/*
 *	A Toolbar Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/toolbar.hpp
 */

#ifndef NANA_GUI_WIDGET_TOOLBAR_HPP
#define NANA_GUI_WIDGET_TOOLBAR_HPP

#include "widget.hpp"
#include <vector>

namespace nana{ namespace gui{
	class toolbar;

	namespace drawerbase
	{
		namespace toolbar
		{
			struct item_type;

			struct extra_events
			{
				nana::fn_group<void(nana::gui::toolbar&, size_t)> selected;
				nana::fn_group<void(nana::gui::toolbar&, size_t)> enter;
				nana::fn_group<void(nana::gui::toolbar&, size_t)> leave;
			};

			class drawer
				: public nana::gui::drawer_trigger
			{
				struct drawer_impl_type;

			public:
				typedef std::size_t size_type;

				mutable extra_events ext_event;

				drawer();
				~drawer();

				void append(const nana::string&, const nana::paint::image&);
				void append();
				bool enable(size_type) const;
				bool enable(size_type, bool);
				void scale(unsigned);
			private:
				void bind_window(widget_reference);
				void refresh(graph_reference);
				void attached(graph_reference);
				void detached();
				void mouse_move(graph_reference, const eventinfo&);
				void mouse_leave(graph_reference, const eventinfo&);
				void mouse_down(graph_reference, const eventinfo&);
				void mouse_up(graph_reference, const eventinfo&);
			private:
				size_type _m_which(int x, int y, bool want_if_disabled) const;
				void _m_draw_background(nana::color_t color);
				void _m_draw();
				void _m_owner_sized(const eventinfo& ei);
			private:
				void _m_fill_pixels(item_type*, bool force);
			private:
				widget*	widget_;
				nana::paint::graphics* graph_;
				drawer_impl_type	*impl_;
			};
		
		}//end namespace toolbar
	}//end namespace drawerbase

	class toolbar
		: public widget_object<category::widget_tag, drawerbase::toolbar::drawer>
	{
	public:
		typedef std::size_t size_type;
		typedef drawerbase::toolbar::extra_events ext_event_type;

		toolbar();
		toolbar(window, bool visible);
		toolbar(window, const rectangle& = rectangle(), bool visible = true);

		ext_event_type& ext_event() const;
		void append();
		void append(const nana::string& text, const nana::paint::image& img);
		void append(const nana::string&);
		bool enable(size_type) const;
		void enable(size_type n, bool eb);
		void scale(unsigned s);
	};
}//end namespace gui
}//end namespace nana


#endif
