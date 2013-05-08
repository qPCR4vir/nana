#include <../temp/EditLayot_Form.hpp>
#include <iostream> 
#include <fstream> 

	EditLayot_Form::EditLayot_Form  (	nana::gui::place     &pl, 
										const std::string    &lay, 
										const nana::string   &DefLayotFileName,
										nana::gui::form      *fm )
		:nana::gui::form ( nana::rectangle( nana::point(500,200), nana::size(400,170) )),
					_OriPlace		(pl),
					//_DefLayFile		(DefLayotFileName),
				    _butOpenProject	(*this),      
					_butSaveProject	(*this),
					_pickProjectName(*this),
					_ReCollcate		(*this),
					_textBox		(*this),
					_ReLayot		(*this),
					_cbProject		(*this),
					_lProject		(*this),
				    _place			(*this),
					_save			(true),
					_menu			(*this),
					_menuFile		(_menu.push_back(STR("&File"))),
					_menuProgram	(_menu.push_back(STR("&Programm"))),
					_Titel          (STR("Editing layot of: "))
	{	
		if (fm) _Titel +=fm->caption(); 
		InitCaptions();
		InitMyLayot();

		_cbProject.editable(true);
		_textBox.editable(true);

		if (lay!="") 
			_textBox.append(nana::string(nana::charset(lay)),false );
		MakeResponsive();
		if(DefLayotFileName!=STR(""))
		{	if (lay=="") 
				OpenFileN(DefLayotFileName);
		    else
			{	_cbProject.push_back	(DefLayotFileName);
				_cbProject.option(0);
			}
		}

	}

void EditLayot_Form::InitCaptions()
	{
		 caption				(_Titel);
		_lProject.caption		(STR("Project:"	));
		_butOpenProject.caption	(STR("Open"		));
		_butSaveProject.caption	(STR("Save"		));
		_pickProjectName.caption(STR("..."		));
		_ReCollcate.caption		(STR("@"		));
		_ReLayot.caption		(STR("lay"		));
		_textBox.tip_string		(STR("type or load a layot to be applied to the calling window..."		));
	}
void EditLayot_Form::MakeResponsive()
	{
		_menuFile.append  (STR("&Open..."),[&](nana::gui::menu::item_proxy& ip){OpenFile();});
		_menuFile.append  (STR("&Save..."),[&](nana::gui::menu::item_proxy& ip){SaveFile();});
		_menuProgram.append (STR("&Apply Layot to calling windows"),[&](nana::gui::menu::item_proxy& ip){ReLayot();});
		_menuProgram.append (STR("&Edit this windows Layot"),[&](nana::gui::menu::item_proxy& ip){EditMyLayot();});

		_butOpenProject.make_event	<nana::gui::events::click> (*this , &EditLayot_Form::OpenFile		);
		_butSaveProject.make_event	<nana::gui::events::click> (*this , &EditLayot_Form::SaveFile		);
		_ReCollcate.make_event		<nana::gui::events::click> (_place, &nana::gui::place::collocate	);
		_ReLayot.make_event			<nana::gui::events::click> (*this , &EditLayot_Form::ReLayot		);
		_cbProject.ext_event().selected = [&](nana::gui::combox&cb)
		{
			SaveFile();  // save only is edited, changed ??? but how to know??	;
			_save =false;
			if(_cbProject.the_number_of_options()>0)
				_textBox.load(_cbProject.text(_cbProject.option()).c_str() );
			_save=true;
		};

	}
void EditLayot_Form::InitMyLayot()
	{
		std::string layot;
        _place.div( readLayot(STR("layot.txt"), layot ="") );
		_place.field("proj_buttons")<< _butOpenProject<< _butSaveProject;
		_place.field("label"       )<< _lProject ;
		_place.field("proj_txt"    )<< _cbProject;
		_place.field("pick"		   )<< _pickProjectName;
		_place.field("textBox"	   )<< _textBox;
		_place.field("re"		   )<<_ReLayot<< _ReCollcate ;
		_place.collocate ();
	}
void EditLayot_Form::ReLayot()
	{   nana::string lay,line;
		for (size_t linum=0; _textBox.getline(linum , line) ; ++linum )
			lay+=line;
        _OriPlace.div( std::string(nana::charset (lay)).c_str() );  // try std::runtime_error msgbox
		_OriPlace.collocate ();
	}
void EditLayot_Form::OpenFile()
	{	  if(!_save) return; 
		 nana::gui::filebox fb(0, true);
		 fb.add_filter(STR("Text File"), STR("*.txt;*.doc"));
		 fb.add_filter(STR("All Files"), STR("*.*"));
		 nana::string file;
		 if(fb())  
		 { 
			 file = fb.file();
			 OpenFileN(file);

		 }
	}
void EditLayot_Form::OpenFileN(const nana::string   &file)
	{	  if(!_save) return; 
			std::wcout<<std::endl<<STR("caption: ")<<_cbProject.caption()<<std::endl;
			std::wcout<<std::endl<<STR("Option: ")<<_cbProject.option()<<std::endl;
			//std::wcout<<std::endl<<STR("Option text: ")<<_cbProject.text(_cbProject.option())<<std::endl;
			std::wcout<<std::endl<<STR("Option Last: ")<<_cbProject.the_number_of_options()<<std::endl;
			//std::wcout<<std::endl<<STR("text Last-1: ")<<_cbProject.text(_cbProject.the_number_of_options()-1)<<std::endl;
			 _save=false;
			_cbProject.push_back(file).option(_cbProject.the_number_of_options());
			caption	(_Titel+STR(" <")+ file+STR(">"));
			_textBox.load(file.c_str() );
			 _save=true;

			std::wcout<<std::endl<<STR("OpenFIle: ")<<file<<std::endl;
			std::wcout<<std::endl<<STR("text 0 pos: ")<<_cbProject.text(0)<<std::endl;
			std::wcout<<std::endl<<STR("Option: ")<<_cbProject.option()<<std::endl;
			std::wcout<<std::endl<<STR("Option text: ")<<_cbProject.text(_cbProject.option())<<std::endl;
			std::wcout<<std::endl<<STR("Option Last: ")<<_cbProject.the_number_of_options()<<std::endl;
			std::wcout<<std::endl<<STR("text Last-1: ")<<_cbProject.text(_cbProject.the_number_of_options()-1)<<std::endl;
	}
void EditLayot_Form::SaveFile()
	{	 if(!_save) return;
	   
		 nana::gui::filebox fb(0, false);
		 fb.add_filter(STR("Text File"), STR("*.txt;*.doc"));
		 fb.add_filter(STR("All Files"), STR("*.*"));
		 nana::string file;
		 if(fb())  
		 { 
			 file = fb.file();
			_textBox.store(file.c_str() );
			caption	(_Titel+STR(" <")+ file+STR(">"));
			size_t op=_cbProject.the_number_of_options();
			 _save=false;
			_cbProject.push_back(file);
			_cbProject.option(op);
			 _save=true;

			std::wcout<<std::endl<<STR("OpenFIle: ")<<file<<std::endl;
		 }
	}
void EditLayot_Form::EditMyLayot()
		{
			if (!_myEdLayForm) 
				_myEdLayForm.reset (new EditLayot_Form (_place,"", STR("layot.txt") , this ));
			_myEdLayForm->show ();
		}
const char* EditLayot_Form::readLayot(const nana::string& FileName, std::string& layot)
    {
		std::ifstream loy(FileName);
		std::string temp;

		while (std::getline(loy,temp)) layot+=temp;
		std::cout<< layot;
		return layot.c_str();
    }	


