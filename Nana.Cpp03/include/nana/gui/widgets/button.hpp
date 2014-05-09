/*
 *	A Button Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/button.hpp
 */

#ifndef NANA_GUI_WIDGET_BUTTON_HPP
#define NANA_GUI_WIDGET_BUTTON_HPP
#include "widget.hpp"
#include "../element.hpp"


namespace nana{namespace gui{
	namespace drawerbase
	{
		namespace button
		{
			//button_drawer
			//@brief: Draw the button
			class trigger: public nana::gui::drawer_trigger
			{
			public:
				trigger();
				~trigger();
				void icon(const nana::paint::image&);
				bool enable_pushed(bool);
				bool pushed(bool);
				bool pushed() const;
				void omitted(bool);
				bool focus_color(bool);

				element::cite_bground& cite();
			private:
				void attached(widget_reference, graph_reference);
				void refresh(graph_reference);
				void mouse_enter(graph_reference, const eventinfo&);
				void mouse_leave(graph_reference, const eventinfo&);
				void mouse_down(graph_reference, const eventinfo&);
				void mouse_up(graph_reference, const eventinfo&);
				void key_char(graph_reference, const eventinfo&);
				void key_down(graph_reference, const eventinfo&);
				void focus(graph_reference, const eventinfo&);
			private:
				void _m_draw(graph_reference);
				void _m_draw_title(graph_reference, bool enabled);
				void _m_draw_background(graph_reference);
				void _m_draw_border(graph_reference);
			private:
				nana::gui::widget* widget_;
				nana::paint::graphics* graph_;

				element::cite_bground cite_;
				struct attr_tag
				{
					element_state::t e_state;
					bool omitted;
					bool focused;
					bool pushed;
					bool keep_pressed;
					bool enable_pushed;
					bool focus_color;
					paint::image * icon;
					color_t bgcolor;
					color_t fgcolor;
				}attr_;
			};
		}//end namespace button
	}//end namespace drawerbase

		/// Defaine a button widget and it provides the interfaces to be operational
		class button
			: public widget_object<category::widget_tag, drawerbase::button::trigger>
		{
			typedef button self_type;
			typedef widget_object<category::widget_tag, drawerbase::button::trigger> base_type;
		public:
			button();
			button(window, bool visible);
			button(window, const nana::string& caption, bool visible = true);
			button(window, const nana::char_t* caption, bool visible = true);
			button(window, const rectangle& = rectangle(), bool visible = true);

			button& icon(const nana::paint::image&);
			button& enable_pushed(bool);
			bool pushed() const;
			button& pushed(bool);
			button& omitted(bool);
			button& enable_focus_color(bool);

			button& set_bground(const pat::cloneable<element::element_interface>&);
			button& set_bground(const std::string&);

			button& transparent(bool enable);
			bool transparent() const;

			button& edge_effects(bool enable);
		private:
			void _m_shortkey();
			void _m_complete_creation();
			void _m_caption(const nana::string&);
		};
}//end namespace gui
}//end namespace nana
#endif
