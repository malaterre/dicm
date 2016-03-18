#include "vr.hxx"
#include <cstdlib>
#include <iostream>

namespace dd = dicm::details;
typedef dd::value_representation vr_t;

static const char vrs[][3] = {
    "AE", "AS", "AT", "CS", "DA", "DS", "DT", "FL", "FD", "IS", "LO",
    "LT", "OB", "OD", "OF", "OW", "PN", "SH", "SL", "SQ", "SS", "ST",
    "TM", "UC", "UI", "UL", "UN", "UR", "US", "UT", "ZZ",
};

int main() {
  vr_t vr = vr_t::type::UT;
  std::cout << sizeof vr << std::endl;
  vr_t::str s;
  for (auto&& i : vrs) {
    vr.set(i);
    std::cout << i << " =" << /*vr.b <<*/ " " << vr.val << std::endl;
    //                vr.print( std::cout );
    vr.to_array(s);
    std::cout << '(' << s[0] << s[1] << ')' << std::endl;
    //   vr_t::type t = vr.underlying();
  }
  std::cout << std::boolalpha;
  std::cout << std::is_pod<vr_t>::value << '\n';

  //   vr_t::type t = vr.underlying();
  //   std::cout << (int)t << std::endl;
  vr_t a;
  vr_t b;
  if (a == b) std::cout << "equal" << std::endl;
  return EXIT_SUCCESS;
}
