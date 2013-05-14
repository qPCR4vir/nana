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
        {   _textBox.append(nana::string(nana::charset(lay)),false );
            _textBox.select(true);
            _textBox.show();

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
		    if(! _OSbx.UserSelected()) return;

            nana::string   fileN=_OSbx.FileName();  // The newly selected name
            std::wcout<<std::endl<<STR("Selected: ")<<fileN<<std::endl;
            SaveFileN(_Loaded);  // save only is edited?, changed ??? but how to know??	;
			OpenFileN(fileN );
		};

	}
void EditLayot_Form::InitMyLayot()
	{
		std::string layot;
        readLayot( STR("Layot_Form.lay.txt"), layot ="");
        if (layot.empty() )
            layot="vertical\n\t	<weight=25>\n\t<weight=2>\n\t<Layot gap=2 weight=19>\n\t<weight=2>\n\t<textBox>\n\t< weight=25 <weight=5> <re weight=50 gap=1>>\n\t<weight=5>";
        _place.div( layot.c_str () );
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
      if(_OSbx.Canceled () ) return;
      std::wcout<<std::endl<<STR("OpenFile: ")<<std::endl;
	  OpenFileN(_OSbx.FileName());
	}
void EditLayot_Form::OpenFileN(const nana::string   &file)
	{	  
		if( file.empty() ) return;
        std::wcout<<std::endl<<STR("OpenFileN: ")<<file<<std::endl;
		caption	(_Titel+STR(" <")+ file+STR(">"));
		_textBox.load(file.c_str() );
        _textBox.select(true);
            _textBox.show();
        _Loaded=file;
       std::wcout<<std::endl<<STR("OpenedFileN: ")<<file<<std::endl;
	}
void EditLayot_Form::SaveFileN(const nana::string   &fileTip)
	{	
        std::wcout<<std::endl<<STR("Seaving tip: ")<<fileTip<<std::endl;
        _OSbx.fb_s.init_path(fileTip);
        _OSbx.save();
       if(_OSbx.Canceled () ) return;
       SaveFile();
	}
void EditLayot_Form::SaveFile()
	{	
      if(_OSbx.Canceled () ) return;
        std::wcout<<std::endl<<STR("Seaving file: ")<<_OSbx.FileName()<<std::endl;
		_textBox.store(_OSbx.FileName().c_str() );
        _Loaded=_OSbx.FileName();
		caption	(_Titel+STR(" <" )+ _Loaded + STR( ">" ));
		std::wcout<<std::endl<<STR("SavedFIle: ")<<_OSbx.FileName()<<std::endl;
	}
void EditLayot_Form::EditMyLayot()
		{
			if (!_myEdLayForm) 
				_myEdLayForm.reset (new EditLayot_Form (_place,"vertical\n\t	<weight=25>\n\t<weight=2>\n\t<Layot gap=2 weight=19>\n\t<weight=2>\n\t<textBox>\n\t< weight=25 <weight=5> <re weight=50 gap=1>>\n\t<weight=5>",
                STR("Layot_Form.lay.txt") , this ));
			_myEdLayForm->show ();
		}
const char* EditLayot_Form::readLayot(const nana::string& FileName, std::string& layot)
    {
		std::ifstream loy(FileName);
		std::string temp;

		while (std::getline(loy,temp)) layot+=temp + "\n";
		std::cout<< layot;
		return layot.c_str();
    }	


