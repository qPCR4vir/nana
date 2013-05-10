#include <../temp/EditLayot_Form.hpp>


int main()
{
	nana::gui::form  form(nana::rectangle( nana::point(500,410), nana::size(400,150) ));
	form.caption (STR("Configurable Window"));
	nana::gui::menubar	menub(form);
	nana::gui::button	but1(form), but2(form), but3(form), but4(form);
	OpenSaveBox osb(form,STR("Layot:") ), osb2(form,STR("Layot2:") );
				fb.add_filter(STR("Text File"), STR("*.txt;*.doc"));

	osb.ly ( "<<weight=2>gap=2 <Layot gap=2> weight=19 <weight=2>>" );
	but1.caption (STR("But1"));
	but2.caption (STR("But2"));
	but3.caption (STR("But3"));
	but4.caption (STR("But4"));
	nana::gui::place	place(form);
	std::stringstream lay;
	lay << "vertical\n\t<weight=25>\n\t" << osb.ly () << "\n\t<b1 <b2><b3>>\n\t<b4> ";
	place.div(lay.str().c_str ());     // try std::runtime_error msgbox
	place.field("b1")<<but1;
	place.field("b2")<<but2;
	place.field("b3")<<but3;
	place.field("b4")<<but4;
	place<<osb;
	place.collocate ();


	std::unique_ptr <EditLayot_Form> LayForm;
	menub.push_back(STR("&Programm")).append(STR("&Edit this windows Layot"),[&](nana::gui::menu::item_proxy& ip)
	{
		if (!LayForm) 
			LayForm.reset (new EditLayot_Form (place,lay.str(), STR("") , &form ));
		LayForm->show ();
	});
	form.show();
	nana::gui::exec();
	return 0;
}


