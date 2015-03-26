/*
*	Color Schemes
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/widget_colors.hpp
*	@description:
*/
#ifndef NANA_DETAIL_WIDGET_COLORS_HPP
#define NANA_DETAIL_WIDGET_COLORS_HPP

#include <nana/gui/basis.hpp>
#include <memory>
namespace nana
{
	class color_proxy
	{
	public:
		color_proxy(const color_proxy&);
		color_proxy(color_rgb);
		color_proxy(colors);
		color_proxy& operator=(const color_proxy&);
		color_proxy& operator=(const ::nana::color&);
		color_proxy& operator=(color_rgb);
		color_proxy& operator=(colors);
		color get_color() const;
		operator color() const;
	private:
		std::shared_ptr<color> color_;
	};//end namespace color_proxy

	struct widget_colors
	{
		virtual ~widget_colors() = default;
        widget_colors(){}
        widget_colors(color_proxy ac,  color_proxy bg , color_proxy fg):
                 activated(ac), background(bg), foreground(fg){}

        color_proxy activated   {def.activated };
        color_proxy background  {def.background};
        color_proxy foreground  {def.background};

        static widget_colors def; 
	};


}

#endif