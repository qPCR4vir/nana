
#include <nana/gui/wvl.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/button.hpp>

using namespace nana::gui;

void readFiled (textbox& owner)
{
    std::cout<<"\nclick: ";

    nana::string text;
    owner.getline (0, text);
    std::wcout<< text;
}
void readFiled2 (textbox& owner, const eventinfo& ei)
{
    std::cout<<"\nfocus " << (ei.focus.getting ? "entering: " : " leaving: ");
 
    nana::string text;
    owner.getline (0, text);
    std::wcout<< text;
}

int main(int argc, char *argv[])
{
    form fm (nana::rectangle(10, 10, 200, 200));

    textbox txtbox (fm, nana::rectangle(20, 20, 160, 30), true);
    txtbox.multi_lines (false);
	    
    txtbox.make_event<events::click>([&](){readFiled(txtbox);});
    txtbox.make_event<events::focus>([&](const eventinfo& ei){readFiled2(txtbox, ei);});
	 
    textbox txtbox2 (fm, nana::rectangle(20, 60, 160, 30), true);
    txtbox2.multi_lines (false);
	    
    txtbox2.make_event<events::click>([&](){readFiled(txtbox2);});
    txtbox2.make_event<events::focus>([&](const eventinfo& ei){readFiled2(txtbox2, ei);});

    button b (fm, nana::rectangle(20, 130, 160, 30), true);
    b.caption (STR("Just leave the text boxes"));

    fm.show();

    exec();

    return 0;
}

