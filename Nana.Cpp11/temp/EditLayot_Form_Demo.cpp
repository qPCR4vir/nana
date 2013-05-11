#include <../temp/EditLayot_Form.hpp>


int main()
{
	nana::gui::form  form(nana::rectangle( nana::point(500,410), nana::size(400,150) ));
	                 form.caption (STR("Configurable Window"));
	nana::gui::menubar	menub(form);
	nana::gui::button	but1 (form), but2(form), but3(form), but4(form);
	but1.caption (STR("But1"));
	but2.caption (STR("But2"));
	but3.caption (STR("But3"));
	but4.caption (STR("But4"));
	OpenSaveBox         osb  (form, STR("Project:") ), 
                        osb2 (form, STR("Project2:") );
	osb.add_filter (STR("Text File"), STR("*.txt;*.doc"));
	osb.add_filter (STR("All File"), STR("*.*"));
	osb2.add_filter(STR("Text File"), STR("*.txt"));
	osb2.add_filter(STR("Todos File"), STR("*.*"));

	nana::gui::place	place(form);
	std::stringstream lay;
	lay << "vertical\n\t<weight=25> gap=2\n\t" << osb.ly () << "\n\t<gap=2 <b1> <b2><b3>>\n\t<b4>\n\t"
                                          << osb2.ly() ;
	place.div(lay.str().c_str ());     // try std::runtime_error msgbox

	place.field("b1")<<but1;
	place.field("b2")<<but2;
	place.field("b3")<<but3;
	place.field("b4")<<but4;
	place<<osb<<osb2;
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


