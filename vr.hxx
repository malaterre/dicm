#pragma once

#include "dicm_export.h"
#include <cstdint>
#include <iosfwd>

namespace dicm {
namespace details {

/**
 * Implementation of DICOM Value Representation:
 * http://dicom.nema.org/medical/dicom/current/output/chtml/part05/sect_6.2.html#table_6.2-1
 *
 * Design choice:
 * - We need to handle all possible combination of VRs. Which means basically
 *   `char[2]` (16 bytes)
 * - We would like also to support OB_OW as defined in the dictionary.
 * This translate into:
 * - We cannot use a finite set of enum(s) since we don't know which VR will be
 *   added to the standard
 * - While char[2] seems like the most obvious solution we prefer a slightly
 * more complex implementation to handle dual VR (eg. OB/OW)
 */
struct DICM_EXPORT value_representation {
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

  type underlying() const;

  /// Convert to an array:
  typedef char str[2];
  const str& to_array(str& in) const;

  /// Construct from a int
  static value_representation make_from_type(int val);

  /// Construct from a pointer to array:
  static value_representation make_from_string(const char* in);
  friend bool operator==(const value_representation& lhs,
                         const value_representation& rhs) {
    return lhs.val == rhs.val;
  }

 private:
  friend std::ostream& operator<<(std::ostream& os,
                                  const value_representation& vr);

  std::uint16_t val;
};

DICM_EXPORT std::ostream & operator<<(std::ostream &os, const value_representation& vr);

}  // end namespace details
typedef details::value_representation vr_t;
}  // end namespace dicm
