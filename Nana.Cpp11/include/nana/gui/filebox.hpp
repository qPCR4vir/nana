#ifndef NANA_GUI_FILEBOX_HPP
#define NANA_GUI_FILEBOX_HPP
#include <nana/gui/wvl.hpp>

namespace nana{	namespace gui
{
	class filebox
		: nana::noncopyable
	{
		struct implement;
	public:
		filebox(window owner, bool is_open_mode);
		~filebox();

		void title(const nana::string& s);
		void init_path(const nana::string& s);
		void add_filter(const nana::string& description, const nana::string& filetype);

		nana::string file() const;

		//Show the filebox
		bool show() const;
		
		//A function object method alternative to show()
		bool operator()() const
		{
			return show();
		}
	private:
		implement * impl_;
	};
}//end namespace gui
}//end namespace nana
#endif
