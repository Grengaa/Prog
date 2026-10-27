#include "../src/pressurecalibration.hpp"
#include <iostream>
#include <limits>
int main() {
    PressureCalibration c;
    int failures=0;
    auto check=[&](bool ok) { if(!ok) ++failures; };
    check(!c.convert(100));
    // Synthetic references for testing only; NOT calibration for a real HX710B.
    check(c.configure(1000,0,5000,200));
    check(c.convert(1000)==0 && c.convert(5000)==200 && c.convert(3000)==100);
    check(c.convert(0)==-50); // Preserve out-of-range readings; do not clamp.
    check(c.configure(5000,0,1000,200) && c.convert(3000)==100);
    check(!c.configure(10,0,10,100) && !c.convert(10));
    check(!c.configure(10,0,20,0));
    check(!c.configure(0,0,std::numeric_limits<double>::infinity(),100));
    c.configure(0,0,100,100); c.invalidate(); check(!c.convert(10));
    std::cout << "PressureCalibration: " << failures << " falhas\n";
    return failures ? 1 : 0;
}
