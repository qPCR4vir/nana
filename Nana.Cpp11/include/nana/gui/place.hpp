/*
 *	An Implementation of Place for Layout
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/place.cpp
 */

#ifndef NANA_GUI_PLACE_HPP
#define NANA_GUI_PLACE_HPP
#include <utility>
#include <nana/gui/basis.hpp>
#include <limits>
#include <nana\gui\programming_interface.hpp>


namespace nana {namespace gui
{
	namespace place_impl
    {
        struct IField ;
        struct implement;
    }
    using  place_impl::IField ;
    using  place_impl::implement ;

    class place		: nana::noncopyable
	{
	  public:
        struct minmax
        {
            minmax(unsigned Min=MIN, unsigned Max=MAX);
            unsigned min,max /*, Min(), Max()*/;
            void     MinMax      (minmax Size_Range) 
                {
                    min=Size_Range.min; 
                    max=Size_Range.max;
                };
            void     MinMax      (unsigned min_,unsigned max_=MAX) {min=min_; max=max_;} 
            minmax   MinMax      (                 )  { return *this;};
            static const unsigned MIN=0,MAX=1000000;
        };
		struct field_t
		{
			virtual field_t& operator<<(minmax              Size_range)	= 0;
            virtual field_t& operator<<(IField *            fld)		= 0;
            virtual field_t& operator<<(const std::wstring& txt)		= 0;
            virtual field_t& operator<<(const std::string&  txt)		= 0;
			virtual field_t& operator<<(window              wd)		= 0;    
			virtual field_t& operator<<(unsigned            gap)  	= 0;
			virtual field_t& fasten(window wd)	    = 0;
			virtual ~field_t()                      = 0;
		};

		typedef field_t &             field_reference;

		 place(window parent_widget);
		 place();
		void              bind     (window parent_widget);
		~place();

		void              div      (const char* layout);
		void              collocate();
		field_reference    field   (const char* name);    /// TODO: Add min and max

		static IField*     fixed   (window wd, unsigned size         );
		static IField*     percent (window wd, double   percent_ , minmax MinMax=minmax()    );
		static IField*     room    (window wd, unsigned w, unsigned h);/// TODO: Add min and max
		/** @brief Bind to a window
		 *	@param handle	A handle to a window which the place wants to attach.
		 *	@remark	It will throw an exception if the place has already binded to a window.
		 */

	  private:
		implement * impl_;
	};
}//end namespace gui
}//end namespace nana

#endif //#ifndef NANA_GUI_PLACE_HPP
