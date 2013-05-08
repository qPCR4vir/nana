#include <../temp/CompoWidget.hpp>


CompoWidget::CompoWidget (	const nana::string &caption_, 
							const std::string& fieldName, 
							const std::string& layot,
							const nana::string& fileName)
				: _caption	(caption_ ), 
				  _lay		(layot),
				  _fieldname (fieldName!=""? fieldName : std::string(nana::charset(caption_)))
{
	size_t len=0;
	for(char c: _fieldname) 
		if (c=='_' || iswalnum(c))
			_fieldname[len++]=c;
	_fieldname.resize(len); 
}
nana::gui::place::field_reference operator<<(nana::gui::place::field_reference f, CompoWidget& cw)
{
	return cw.put(f); 
}
nana::gui::place& operator<<(nana::gui::place&pl, CompoWidget& cw)
{
	return cw.put(pl); 
}	

OpenSaveBox::OpenSaveBox     (	nana::gui::form &fm, 
								const nana::string   &label,
								const nana::string   &DefFileName )
							:	CompoWidget(	label  ),
								Open(fm), Save(fm), Pick(fm),_fileName(fm),_label(fm)
{
	//caption(label);  // or def as FileName  ??
	_label.caption (caption()  ); 
	Open.caption	(STR("Open"		));
	Save.caption	(STR("Save"		));
	Pick.caption	(STR("..."		));
	std::string lay;
	lay+=;

//		ly( "        <weight=2>\n"
//"        < weight=25 <weight=5><   vertical weight=45 <><label weight=15><>     >\n"
//"		                             <proj_buttons weight=74 gap=1> \n"
//"									 <proj_txt> \n"
//"									 <pick weight=30>\n"
//"									 <weight=4> 	>" );  // an interesante idea, but allow only one instantition of a CompoWidwet of this type 

	Open.make_event	<nana::gui::events::click> (*this , &OpenSaveBox::open	);
	Save.make_event	<nana::gui::events::click> (*this , &OpenSaveBox::save	);
	//_fileName.ext_event().selected = [&](nana::gui::combox&cb)
	//{
	//	SaveFile();  // save only is edited, changed ??? but how to know??	;
	//	_save =false;
	//	if(_cbProject.the_number_of_options()>0)
	//		_textBox.load(_cbProject.text(_cbProject.option()).c_str() );
	//	_save=true;
	//};

	_fileName.editable(true);
	if(DefFileName!=STR(""))
	{	_fileName.push_back	(DefFileName);
		_fileName.option(0);
	}
			fb.add_filter(STR("Text File"), STR("*.txt;*.doc"));

}
	
void OpenSaveBox::open()
{
		nana::string file;
		if(fb_o())  
		{ 
			file = fb.file();
		_fileName.push_back(file).option(_fileName.the_number_of_options());
		}
}
void OpenSaveBox::save()
{
		nana::string file;
		if(fb_o())  
		{ 
			file = fb.file();
		_fileName.push_back(file).option(_fileName.the_number_of_options());
		}
}
OpenSaveBox::p::field_reference	OpenSaveBox::put(p::field_reference f)
{
	return f << p::fixed(_label,49) << p::fixed(Open,40) << p::fixed(Save,35) <<_fileName << p::fixed(Pick,25); 
}
	//virtual p&	put(p&   pl) override
	//{	
	//	pl.field (fn ())<<*this;
	//	return pl;			
	//}

