#include "vr.hxx"
#include <cstdlib>
#include <iostream>

namespace dd = dicm::details;
typedef dd::value_representation vr_t;

#if 0
AE // Application Entity
AS // Age String
AT // Attribute Tag
CS // Code String
DA // Date
DS // Decimal String
DT // Date Time
FL // Floating Point Single
FD // Floating Point Double
IS // Integer String
LO // Long String
LT // Long Text
OB // Other Byte String
OD // Other Double String
OF // Other Float String
OW // Other Word String
PN // Person Name
SH // Short String
SL // Signed Long
SQ // Sequence of Items
SS // Signed Short
ST // Short Text
TM // Time
UC // Unlimited Characters
UI // Unique Identifier (UID)
UL // Unsigned Long
UN // Unknown
UR // Universal Resource Identifier or Universal Resource Locator (URI/URL)
US // Unsigned Short
UT // Unlimited Text
#endif

static const char vrs[][3] = {
"AE", 
"AS", 
"AT", 
"CS", 
"DA", 
"DS", 
"DT", 
"FL", 
"FD", 
"IS", 
"LO", 
"LT", 
"OB", 
"OD", 
"OF", 
"OW", 
"PN", 
"SH", 
"SL", 
"SQ", 
"SS", 
"ST", 
"TM", 
"UC", 
"UI", 
"UL", 
"UN", 
"UR", 
"US", 
"UT", 
"ZZ", 




#if 0












//	"AA",
	"AE",
	"AS",
	"AT",
	"CS",
	"DA",
	"DS",
	"DT",
	"FD",
	"FL",
	"IS",
	"LO",
	"LT",
	"OB",
	"OD",
	"OF",
	"OW",
	"PN",
	"SH",
	"SL",
	"SQ",
	"SS",
	"ST",
	"TM",
	"UI",
	"UL",
	"UN",
	"US",
	"UT",
//	"ZZ"
#endif
};

int main()
{
	vr_t vr = vr_t::type::UT;
std::cout << sizeof vr << std::endl;
	vr_t::str s;
	for( auto&& i: vrs )
	{
                //std::copy(i, i + 2, vr.bytes );
		//std::cout << i << "->" << std::hex << vr.val << std::endl;
                vr.set( i );
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
vr_t a; vr_t b;
 if( a == b )
std::cout << "equal" << std::endl;
	return EXIT_SUCCESS;
}
