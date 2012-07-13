/*
 *	This is a demo of Nana C++ Library
 *	Author: Jinhao
 *	The demo requires Nana 0.2.5 and C++11 compiler
 *	Screenshot at http://sourceforge.net/projects/stdex
 */
#include <nana/gui/wvl.hpp>
#include <nana/gui/layout.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/progress.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/treebox.hpp>
#include <nana/gui/widgets/checkbox.hpp>
#include <nana/filesystem/file_iterator.hpp>
#include <nana/gui/widgets/date_chooser.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/categorize.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui/tooltip.hpp>
#include <memory>
#include <vector>

namespace demo
{
	using namespace nana::gui;

	class tab_page_listbox
		: public panel<false>
	{
	public:
		tab_page_listbox(window wd)
			: panel<false>(wd)
		{
			gird_.bind(*this);
			listbox_.create(*this);
			listbox_.append_header(STR("Supported compilers"), 200);
			listbox_.append_categ(STR("Nana.C++03"));
			listbox_.append_item(1, STR("GCC 3.4 and later"));
			listbox_.append_item(1, STR("Visual C++ 2003 and later"));

			listbox_.append_categ(STR("Nana.C++11"));
			listbox_.append_item(2, STR("GCC 4.6 and later"));
			listbox_.append_item(2, STR("Visual C++ 2012 and later"));

			checkbox_.create(*this);
			checkbox_.caption(STR("Checkable Listbox"));
			checkbox_.make_event<events::click>([this]()
				{
					this->listbox_.checkable(this->checkbox_.checked());
				});

			gird_.add(listbox_, 0, 0);
			gird * vgird = gird_.add(10, 140);
			vgird->push(checkbox_, 0, 40);
		}
	private:
		gird gird_;
		listbox		listbox_;
		checkbox	checkbox_;
	};

	class tab_page_treebox
		: public panel<false>
	{
	public:
		typedef treebox<int>::node_type node_type;

		tab_page_treebox(window wd)
			: panel<false>(wd)
		{
			gird_.bind(*this);
			treebox_.create(*this);
			gird_.push(treebox_, 0, 0);

#if defined(NANA_WINDOWS)
			node_type node = treebox_.insert(STR("C:"), STR("Local Drive(C:)"), 0);
			nana::filesystem::file_iterator i(STR("C:\\")), end;
#elif defined(NANA_LINUX)
			//Use a whitespace for the root key string under Linux
			node_type node = treebox_.insert(STR(" "), STR("Root"), 0);
			nana::filesystem::file_iterator i(STR("/")), end;
#endif
			for(; i != end; ++i)
			{
				if(false == i->directory) continue;

				treebox_.insert(node, i->name, i->name, 0);
				break;
			}
			treebox_.ext_event().expand = nana::make_fun(*this, &tab_page_treebox::_m_expand);
		}
	private:
		void _m_expand(nana::gui::window, node_type node, bool exp)
		{
			if(!exp) return; //If this is contracted.

			nana::string path = treebox_.make_key_path(node, STR("/")) + STR("/");
			//Trim the head of whitespace, more portable, because the root
			//under Linux is a whitespace
			nana::string::size_type path_start_pos = path.find_first_not_of(STR(" "));
			if(path_start_pos != nana::string::npos)
				path.erase(0, path_start_pos);

			//Walk in the path directory for sub directories.
			nana::filesystem::file_iterator i(path), end;
			for(; i != end; ++i)
			{
				if(false == i->directory) continue; //If it is not a directory.

				node_type child = treebox_.insert(node, i->name, i->name, 0);
				if(0 == child) continue;
            
				//Find a directory in child directory, if there is a directory,
				//insert it into the child, just insert one node to indicate the
				//node has a child and an arrow symbol will be displayed in the
				//front of the node.
				nana::filesystem::file_iterator u(path + i->name);
				for(; u != end; ++u)
				{
					if(false == u->directory) continue; //If it is not a directory.
					treebox_.insert(child, u->name, u->name, 0);
					break;
				}
			}
		}
	private:
		gird gird_;
		treebox<int> treebox_;
	};

	class tab_page_datechooser
		: public panel<false>
	{
	public:
		tab_page_datechooser(window wd)
			: panel<false>(wd)
		{
			date_.create(*this, nana::rectangle(10, 10, 260, 200));
			textbox_.create(*this, nana::rectangle(280, 10, 170, 23));
			textbox_.tip_string(STR("Input a date:"));
		}
	private:
		date_chooser date_;
		textbox	textbox_;
	};
	
	class tab_page_radiogroup
		: public panel<false>
	{
	public:
		tab_page_radiogroup(window wd)
			:	panel<false>(wd)
		{
			gird_.bind(*this);
			gird * gdcontext = gird_.add(10, 0);
			gird * gdbox = gdcontext->push(0, 0);
			const nana::string str[6] = {
					STR("Airbus"), STR("���ߧ��ߧ��"),
					STR("Boeing"), STR("Bombardier"),
					STR("Cessna"), STR("EMBRAER")};
					
			for(int i = 0; i < 6; ++i)
			{
				auto p = std::make_shared<checkbox>(*this);
				box_.push_back(p);

				//Add the checkbox to the radio group. The radio group does not
				//manage the life of checkboxs.
				group_.add(*p);
				gdbox->push(*p, 5, 20);
				p->caption(str[i]);
				p->make_event<events::click>([this]()
					{
						std::size_t index = this->group_.checked();
						nana::string str = this->box_[index]->caption();

						this->label_.caption(STR("You have selected ") + str);
						this->categorize_.caption(STR("Manufacturer\\" + str));
					});	
			}
			gird_.add(0, 10);
			
			label_.create(*this);
			label_.caption(STR("Select an airplane manufacturer"));
			gdcontext->push(label_, 10, 20);

			categorize_.create(*this);
			gdcontext->push(categorize_, 10, 22);
			gdcontext->push(0, 10);

			std::map<nana::string, std::vector<nana::string>> map;
			auto p = &(map[str[0]]);
			p->push_back(STR("320"));
			p->push_back(STR("330"));
			p = &(map[str[1]]);
			p->push_back(STR("An-124"));
			p->push_back(STR("An-225"));
			p = &(map[str[2]]);
			p->push_back(STR("737"));
			p->push_back(STR("747"));
			p->push_back(STR("757"));
			p->push_back(STR("767"));
			p->push_back(STR("777"));
			p->push_back(STR("787"));
			p = &(map[str[3]]);
			p->push_back(STR("CRJ"));
			p->push_back(STR("Dash 8"));
			p = &(map[str[4]]);
			p->push_back(STR("C-170"));
			p->push_back(STR("C-172"));

			p = &(map[str[5]]);
			p->push_back(STR("ERJ-145"));
			p->push_back(STR("E-195"));

			for(auto & mstr: str)
			{
				categorize_.caption(STR("Manufacturer"));
				categorize_.insert(mstr, 0);
				for(auto & astr : map[mstr])
					categorize_.childset(astr, 0);
			}
		}
	private:
		gird gird_;
		radio_group group_;
		std::vector<std::shared_ptr<checkbox>> box_;
		label label_;
		categorize<int> categorize_;
	};

	class widget_show
		: public form
	{
	public:
		widget_show()
			: form(API::make_center(500, 400), appear::decorate<appear::sizable>())
		{
			this->caption(STR("This is a demo of Nana C++ Library"));
			gird_.bind(*this);

			_m_init_buttons();
			_m_init_comboxs();
			_m_init_labels();
			_m_init_progresses();
			_m_init_tabbar();

			this->make_event<events::unload>([this](const eventinfo& ei)
				{
					msgbox mb(this->handle(), STR("Question"), msgbox::yes_no);
					mb.icon(mb.icon_question);
					mb<<STR("Are you sure you want to exit the demo?");
					ei.unload.cancel = (mb.pick_no == mb());
				});
		};
	private:
		void _m_init_buttons()
		{
			gird * gdbutton = gird_.push(10, 22);

			msgbox mb(*this, STR("Msgbox"));
			mb.icon(mb.icon_information);
			mb<<STR("Button Clicked");
			for(int i = 0; i < 3; ++i)
			{
				auto p = std::make_shared<button>(*this);
				buttons_.push_back(p);
				gdbutton->add(p->handle(), 10, 0);
				p->make_event<events::click>(mb);
			}
			gdbutton->add(0, 10);
			
			auto ptr = buttons_[0];
			ptr->caption(STR("Normal Button"));

			ptr = buttons_[1];
			//Nana does not support ICON under Linux now
#if defined(NANA_WINDOWS)
			ptr->icon(STR("image.ico"));
#else
			ptr->icom(STR("image.bmp"));
#endif
			ptr->caption(STR("Button with An Image"));

			ptr = buttons_[2];
			ptr->caption(STR("Pushed Button"));
			ptr->enable_pushed(true);
		}

		void _m_init_comboxs()
		{
			gird * gd = gird_.push(10, 24);
			for(int i = 0; i < 2; ++i)
			{
				auto p = std::make_shared<combox>(*this);
				comboxs_.push_back(p);
				gd->add(p->handle(), 10, 0);
				p->push_back(STR("Item 0"));
				p->push_back(STR("Item 1"));
			}
			gd->add(0, 10);

			msgbox mb(*this, STR("Item Selected"));
			mb.icon(mb.icon_information);

			auto ptr = comboxs_[0];
			ptr->editable(true);
			ptr->caption(STR("This is an editable combox"));
			ptr->ext_event().selected = [this, mb](combox& cmb) mutable
			{
				mb<<STR("The item ")<<cmb.option()<<STR(" is selected in editable combox");
				mb();
				//Clear the buffer, otherwise the mb shows the text generated in
				//the last selected event.
				mb.clear();
			};

			ptr = comboxs_[1];
			ptr->caption(STR("This is an uneditable combox"));
			ptr->ext_event().selected = [this, mb](combox& cmb) mutable
			{
				mb<<STR("The item ")<<cmb.option()<<STR(" is selected in uneditable combox");
				mb();
				//Clear the buffer, otherwise the mb shows the text generated in
				//the last selected event.
				mb.clear();
			};
		}

		void _m_init_labels()
		{
			gird * gd = gird_.push(10, 20);
			for(int i = 0; i < 2; ++i)
			{
				auto p = std::make_shared<label>(*this);
				labels_.push_back(p);
				gd->add(p->handle(), 10, 0);
			}

			auto wd = labels_[0];
			wd->caption(STR("This is a normal label"));

			wd = labels_[1];
			wd->format(true);
			wd->caption(STR("This is a <bold, color=0xFF0000, font=\"Consolas\">formatted label</>"));
		}

		void _m_init_progresses()
		{
			gird * gd = gird_.push(10, 20);
			const nana::string tipstr[] = {STR("Unknwon in progress"), STR("Known in progress")};
			for(int i = 0; i < 2; ++i)
			{
				auto p = std::make_shared<progress>(*this);
				progresses_.push_back(p);
				gd->add(*p, 10, 0);
				p->unknown(i == 0);	//The first progress is unknown mode, the second is known mode.
				tooltip_.set(*p, tipstr[i]);
			}
			gd->add(0, 10);

			timer_.make_tick([this]()
			{
				for(auto & p : this->progresses_)
				{
					//Resets the known mode progress if its value reaches the amount value.
					if(false == p->unknown())
					{
						if(p->value() == p->amount())
							p->value(0);
					}
					p->inc();
				}
			});

			timer_.interval(80);
		}

		void _m_init_tabbar()
		{
			tabbar_.create(*this);
			gird_.push(tabbar_, 10, 24);
			tabbar_.push_back(STR("listbox"));
			tabpages_.push_back(std::make_shared<tab_page_listbox>(*this));
			tabbar_.push_back(STR("treebox"));
			tabpages_.push_back(std::make_shared<tab_page_treebox>(*this));
			tabbar_.push_back(STR("date_chooser"));
			tabpages_.push_back(std::make_shared<tab_page_datechooser>(*this));
			tabbar_.push_back(STR("radio_group"));
			tabpages_.push_back(std::make_shared<tab_page_radiogroup>(*this));

			gird * gd_tabpage = gird_.push(0, 0);
			std::size_t index = 0;
			for(auto & i : tabpages_)
			{
				tabbar_.relate(index++, *i);
				gd_tabpage->fasten(*i);	//Fasten the tab pages
			}
		}
	private:
		//A gird layout management
		gird gird_;
		nana::gui::timer timer_;
		nana::gui::tooltip tooltip_;
		std::vector<std::shared_ptr<button>> buttons_;
		std::vector<std::shared_ptr<combox>> comboxs_;
		std::vector<std::shared_ptr<label>>	labels_;
		std::vector<std::shared_ptr<progress>> progresses_;
		tabbar<nana::string> tabbar_;
		std::vector<std::shared_ptr<panel<false>>> tabpages_;
		
	};//end class widget_show
	
	void go()
	{
		widget_show wdshow;
		wdshow.show();
		exec();
	}
}

int main()
{
	demo::go();	
}
