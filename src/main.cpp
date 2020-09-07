#include "sciter-x.h"
#include "sciter-x-window.hpp"
#include "resources.cpp" // resources packaged into binary blob.

class frame : public sciter::window {
public:
    frame() : window(SW_MAIN) {}

};

int uimain(std::function<int()> run) {

    sciter::archive::instance().open(aux::elements_of(resources)); // bind resources[] (defined in "resources.cpp") with the archive

    sciter::om::hasset<frame> pwin = new frame();

    // note: this:://app URL is dedicated to the sciter::archive content associated with the application
    pwin->load(WSTR("this://app/main.htm"));

    pwin->expand();

    return run();
}