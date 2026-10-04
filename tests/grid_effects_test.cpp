#include "grid_effects.h"
#include <iostream>
int main() {
    int failures=0,width=-1;
    auto check=[&](bool ok,const char* what){if(!ok){std::cerr<<what<<'\n';++failures;}};
    gridui::Effects fx;
    fx.boot(100);
    check(fx.frame(100,width)&&width==0,"boot first frame");
    check(!fx.frame(139,width),"frame rate bound");
    check(fx.frame(340,width)&&width==39,"boot midpoint");
    check(fx.frame(580,width)&&width==78,"boot restore");
    check(!fx.frame(620,width),"no idle animation");
    fx.touch(1000);check(fx.frame(1000,width)&&width==78,"touch pulse");
    check(fx.frame(1120,width)&&width==59,"touch decay");
    check(fx.frame(1160,width)&&width==78,"touch restore");
    fx.boot(2000);fx.enable(false);
    check(!fx.frame(2100,width),"disable cancels");
    fx.touch(2200);check(!fx.frame(2200,width),"disabled touch");
    fx.enable(true);fx.boot(0xfffffff0u);
    check(fx.frame(0x000000e0u,width)&&width==39,"clock wrap");
    fx.cancel();check(!fx.frame(0x100u,width),"screen redraw cancellation");
    std::cout<<"Effects: "<<failures<<" failures\n";
    return failures?1:0;
}
