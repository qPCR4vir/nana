#ifndef NANA_GUI_EditLayot_Form_HPP
#define NANA_GUI_EditLayot_Form_HPP

#include <../temp/CompoWidget.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/menubar.hpp>


class EditLayot_Form : public nana::gui::form
{public:
	EditLayot_Form (	nana::gui::place     &pl, 
						const std::string    &lay="", 
						const nana::string   &DefLayotFileName=STR(""),
						nana::gui::form      *fm=nullptr  );
 private:
	nana::string		_Titel/*, _DefLayFile*/;
	nana::gui::place	_place, &_OriPlace;
	nana::gui::button	_butOpenProject, _butSaveProject, _pickProjectName, _ReCollcate, _ReLayot;
	nana::gui::combox	_cbProject;
	nana::gui::label	_lProject;
	nana::gui::textbox	_textBox;
	nana::gui::menubar	_menu;
	nana::gui::menu	    &_menuFile,&_menuProgram/*,&_menuLayot*/;
	std::unique_ptr <EditLayot_Form> _myEdLayForm;
	bool				_save;

	void InitCaptions();
    void MakeResponsive();
	void InitMyLayot();
	void ReLayot();
	void OpenFile();
	void OpenFileN(const nana::string   &file=STR(""));
	void SaveFile();
	void EditMyLayot();
    static const char* readLayot(const nana::string& FileName, std::string& layot);

}	;
#endif 