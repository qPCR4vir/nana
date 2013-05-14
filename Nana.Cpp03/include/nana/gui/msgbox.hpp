#ifndef NANA_GUI_MSGBOX_HPP
#define NANA_GUI_MSGBOX_HPP

#include <sstream>
#include <nana/gui/basis.hpp>

namespace nana{	namespace gui{

	class msgbox
	{
	public:
		enum icon_t{icon_none, icon_information, icon_warning, icon_error, icon_question};
		enum button_t{ok, yes_no, yes_no_cancel};
		enum pick_t{pick_ok, pick_yes, pick_no, pick_cancel};

		msgbox();
		msgbox(const msgbox&);
		msgbox& operator=(const msgbox&);

		msgbox(const nana::string&);
		msgbox(window, const nana::string&);
		msgbox(window, const nana::string&, button_t);

		msgbox& icon(icon_t);
		void clear();
		msgbox & operator<<(const nana::string&);
		msgbox & operator<<(const nana::char_t*);
		msgbox & operator<<(std::ostream& (*)(std::ostream&));

		template<typename T>
		msgbox & operator<<(const T& t)
		{
			sstream_<<t;
			return *this;
		}

		pick_t show() const;

		//A function object method alternative to show();
		pick_t operator()() const
		{
			return show();
		}
	private:
		std::stringstream sstream_;
		window wd_;
		nana::string title_;
		button_t button_;
		icon_t icon_;
	};

}//end namespace gui
}//end namespace nana

#endif
