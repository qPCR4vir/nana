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
				    _OSbx			(*this, STR("Layot:")),      
					_ReCollocate	(*this),
					_textBox		(*this),
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

		_textBox.editable(true);
		_OSbx.add_filter(STR("Layot File"), STR("*.lay.txt"));

	    if   (lay.empty())    
        {    if (! DefLayotFileName.empty())
             {   _OSbx.FileName	(   DefLayotFileName  );
                 OpenFile();
             }
        } else   		          
        {    _textBox.append(nana::string(nana::charset(lay)),false );
             if (! DefLayotFileName.empty())
                _OSbx.FileName	(   DefLayotFileName  );
        }

		MakeResponsive();
	}

void EditLayot_Form::InitCaptions()
	{
		 caption				(_Titel);
		_ReCollocate.caption	(STR("Apply"		));
		_textBox.tip_string		(STR("type or load a layot to be applied to the calling window..."		));
	}
void EditLayot_Form::MakeResponsive()
	{
		_menuFile.append  (STR("&Open..."),[&](nana::gui::menu::item_proxy& ip){OpenFile();});
		_menuFile.append  (STR("&Save..."),[&](nana::gui::menu::item_proxy& ip){SaveFile();});
		_menuProgram.append (STR("&Apply Layot to calling windows"),[&](nana::gui::menu::item_proxy& ip){ReLayot();});
		_menuProgram.append (STR("&Edit this windows Layot"       ),[&](nana::gui::menu::item_proxy& ip){EditMyLayot();});

		_OSbx.Open.make_event	<nana::gui::events::click> (*this , &EditLayot_Form::OpenFile		);
		_OSbx.Save.make_event	<nana::gui::events::click> (*this , &EditLayot_Form::SaveFile		);
		_OSbx.Pick.make_event	<nana::gui::events::click> (*this , &EditLayot_Form::OpenFile		);
		_ReCollocate.make_event		<nana::gui::events::click> ([&](){ReLayot();});
		_OSbx._fileName.ext_event().selected = [&](nana::gui::combox&cb)
		{
			SaveFile();  // save only is edited, changed ??? but how to know??	;
			_save =false;
			if( ! _OSbx.FileName().empty() )
				_textBox.load(_OSbx.FileName().c_str() );
			_save=true;
		};

	}
void EditLayot_Form::InitMyLayot()
	{
		std::string layot;
        _place.div( readLayot( STR("Layot_Form.lay.txt"), layot ="") );
		_place						<< _OSbx;
		_place.field("textBox"	   )<< _textBox;
		_place.field("re"		   )<< _ReCollocate ;
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
	{	 
       if(_save) 
	        OpenFileN(_OSbx.FileName());
	}
void EditLayot_Form::OpenFileN(const nana::string   &file)
	{	  if(!_save) return; 
            std::wcout<<std::endl<<STR("Opening file: ")<<_OSbx.FileName()<<std::endl;
			 _save=false;
			caption	(_Titel+STR(" <")+ file+STR(">"));
			_textBox.load(file.c_str() );
			_save=true;

			//std::wcout<<std::endl<<STR("OpenFIle: ")<<file<<std::endl;
			//std::wcout<<std::endl<<STR("text 0 pos: ")<<_cbProject.text(0)<<std::endl;
			//std::wcout<<std::endl<<STR("Option: ")<<_cbProject.option()<<std::endl;
			//std::wcout<<std::endl<<STR("Option text: ")<<_cbProject.text(_cbProject.option())<<std::endl;
			//std::wcout<<std::endl<<STR("Option Last: ")<<_cbProject.the_number_of_options()<<std::endl;
			//std::wcout<<std::endl<<STR("text Last-1: ")<<_cbProject.text(_cbProject.the_number_of_options()-1)<<std::endl;
	}
void EditLayot_Form::SaveFile()
	{	if(!_save) return;

        std::wcout<<std::endl<<STR("Seaving file: ")<<_OSbx.FileName()<<std::endl;

		_textBox.store(_OSbx.FileName().c_str() );
		caption	(_Titel+STR(" <")+ _OSbx.FileName()+STR(">"));
		_save=true;

		std::wcout<<std::endl<<STR("SavedFIle: ")<<_OSbx.FileName()<<std::endl;
	}
void EditLayot_Form::EditMyLayot()
		{
			if (!_myEdLayForm) 
				_myEdLayForm.reset (new EditLayot_Form (_place,"",  STR("Layot_Form.lay.txt") , this ));
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


