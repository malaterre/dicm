#pragma once

#include "dicm_export.h"
#include <cstdint>
#include <iosfwd>

namespace dicm {
namespace details {

/**
 */
struct DICM_EXPORT uid_string {
  /// Convert to an array:
  typedef char str[64];
  const str& to_array(str& in) const;

  /// Construct from a pointer to array:
  static uid_string make_from_string(const char* in);
  friend bool operator==(const uid_string& lhs,
                         const uid_string& rhs) {
    return memcmp(lhs.buf, rhs.buf, 64);
  }

private:
  friend std::ostream& operator<<(std::ostream& os,
                                  const uid_string& uid);

  str buf;
};

DICM_EXPORT std::ostream & operator<<(std::ostream &os, const uid_string& uid);

}  // end namespace details
typedef details::uid_string uid_t;
}  // end namespace dicm
