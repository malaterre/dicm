#pragma once

#include "dicm_export.h"
#include <iosfwd>
#include <cstring>

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
    return std::strncmp(lhs.buf, rhs.buf, 64);
  }

private:
  friend std::ostream& operator<<(std::ostream& os,
                                  const uid_string& uid);

  str buf;
};

DICM_EXPORT std::ostream & operator<<(std::ostream &os, const uid_string& uid);

struct DICM_EXPORT uid_bcd { // packed BCD
  /// Convert to an array:
  typedef char str[64];
  const str& to_array(str& in) const;

  /// Construct from a pointer to array:
  static uid_bcd make_from_string(const char* in);
  friend bool operator==(const uid_bcd& lhs,
                         const uid_bcd& rhs) {
    return memcmp(lhs.buf, rhs.buf, 32);
  }

private:
  friend std::ostream& operator<<(std::ostream& os,
                                  const uid_bcd& uid);

  unsigned char buf[32];
};

DICM_EXPORT std::ostream & operator<<(std::ostream &os, const uid_bcd& uid);

struct DICM_EXPORT uid_binary { // uint256_t
  /// Convert to an array:
  typedef char str[64];
  const str& to_array(str& in) const;

  /// Construct from a pointer to array:
  static uid_bcd make_from_string(const char* in);
  friend bool operator==(const uid_bcd& lhs,
                         const uid_bcd& rhs) {
    return memcmp(lhs.buf, rhs.buf, 32);
  }

private:
  friend std::ostream& operator<<(std::ostream& os,
                                  const uid_bcd& uid);

  unsigned char buf[32];
};

DICM_EXPORT std::ostream & operator<<(std::ostream &os, const uid_bcd& uid);


}  // end namespace details
typedef details::uid_string uid_t;
typedef details::uid_bcd uid_bcd_t;
}  // end namespace dicm
