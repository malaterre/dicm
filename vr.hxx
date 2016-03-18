#pragma once

#include <cstdint>
#include <algorithm>
//#include <iostream>
#include <iosfwd>
#include <cassert>

namespace dicm {
namespace details {

/**
 * Design choice:
 * - We need to handle all possible combination of VRs. Which means basically
 *   `char[2]` (16 bytes)
 * - We would like also to support OB_OW as defined in the dictionary.
 * This translate into:
 * - We cannot use a finite set of enum(s) since we don't know which VR will be
 *   added to the standard
 * - While char[2] seems like the most obvious solution we prefer a slightly
 * more complex
 *   implementation to handle dual VR (eg. OB/OW)
 */
struct value_representation {
  // actual types for VR (PS 3.5)
  // do not use scoped enum since I need implicit conversion to int for strict
  // weak ordering:
  enum /*class*/ type : std::uint16_t {
    AE = 4,     // Application Entity
    AS = 18,    // Age String
    AT = 19,    // Attribute Tag
    CS = 530,   // Code String
    DA = 768,   // Date
    DS = 786,   // Decimal String
    DT = 787,   // Date Time
    FL = 1291,  // Floating Point Single
    FD = 1283,  // Floating Point Double
    IS = 2066,  // Integer String
    LO = 2830,  // Long String
    LT = 2835,  // Long Text
    OB = 3585,  // Other Byte String
    OD = 3587,  // Other Double String
    OF = 3589,  // Other Float String
    OW = 3606,  // Other Word String
    PN = 3853,  // Person Name
    SH = 4615,  // Short String
    SL = 4619,  // Signed Long
    SQ = 4624,  // Sequence of Items
    SS = 4626,  // Signed Short
    ST = 4627,  // Short Text
    TM = 4876,  // Time
    UC = 5122,  // Unlimited Characters
    UI = 5128,  // Unique Identifier (UID)
    UL = 5131,  // Unsigned Long
    UN = 5133,  // Unknown
    UR = 5137,  // Universal Resource Identifier or Universal Resource Locator
                // (URI/URL)
    US = 5138,  // Unsigned Short
    UT = 5139,  // Unlimited Text
  };
  // only used for dictionary (PS 3.6)
  enum class dual_type : std::uint16_t {
    OB_OW,
    US_SS,
    US_SS_OW,
  };

  value_representation() {}
  value_representation(type t) : val(t) {}
  type underlying() const;
  typedef char str[2];
  const str& to_array(str& in) const;
  // value_representation(dual_type t) : val(static_cast<std::uint16_t>(t)) {}
  void set(const char* in) {
    union {
      char bytes[2];
      std::uint16_t val;
    } u;
    u.bytes[0] = in[1] - 'A';
    u.bytes[1] = in[0] - 'A';
    val = u.val;
  }
#if 0
	std::ostream & print( std::ostream & os )
	{
		union {
			char bytes[2];
			std::uint16_t val;
		} u;
		u.val = val;

		char str[3];
		str[0] = u.bytes[1] + 'A';
		str[1] = u.bytes[0] + 'A';
		str[2] = 0;
		os << str;
return os;
	}
#endif
  friend bool operator==(const value_representation& lhs,
                         const value_representation& rhs) {
    return lhs.val == rhs.val;
  }
#if 0
private:
struct vr_impl {
  bool b : 1; // 0 => regular VR, 1 dict based
  std::uint16_t val : 15; // technically I only need 13 bits, [0x0, 0x1919 = 6425] < 2^13
};
	vr_impl impl;
#else
  std::uint16_t val;
#endif
};

}  // end namespace detail
}  // end namespace dicm
