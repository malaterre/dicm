#include "vr.hxx"
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace dicm {
namespace details {

// http://stackoverflow.com/questions/4165439/generic-way-to-cast-int-to-enum-in-c
template <typename T>
struct enum_traits {};

template <typename T, size_t N>
T *endof(T(&ra)[N]) {
  return ra + N;
}

template <typename T, typename ValType>
T check(ValType v) {
  typedef enum_traits<T> traits;
  const T *first = traits::enumerators;
  const T *last = endof(traits::enumerators);
  if (traits::sorted) {  // probably premature optimization
    if (std::binary_search(first, last, v)) return T(v);
  } else if (std::find(first, last, v) != last) {
    return T(v);
  }
  // corrupted DICOM or simply unhandled VR ?
  throw std::range_error("invalid vr");
}

template <>
struct enum_traits<value_representation::type> {
  static const value_representation::type enumerators[];
  static const bool sorted = true;
};
// must appear in only one TU,
// so if the above is in a header then it will need the array size
// const e enum_traits<e>::enumerators[] = {x, y, z};
const value_representation::type
    enum_traits<value_representation::type>::enumerators[] = {
        value_representation::type::AE, value_representation::type::AS,
        value_representation::type::AT, value_representation::type::CS,
        value_representation::type::DA, value_representation::type::DS,
        value_representation::type::DT, value_representation::type::FD,
        value_representation::type::FL, value_representation::type::IS,
        value_representation::type::LO, value_representation::type::LT,
        value_representation::type::OB, value_representation::type::OD,
        value_representation::type::OF, value_representation::type::OW,
        value_representation::type::PN, value_representation::type::SH,
        value_representation::type::SL, value_representation::type::SQ,
        value_representation::type::SS, value_representation::type::ST,
        value_representation::type::TM, value_representation::type::UC,
        value_representation::type::UI, value_representation::type::UL,
        value_representation::type::UN, value_representation::type::UR,
        value_representation::type::US, value_representation::type::UT,
};

value_representation::type value_representation::underlying() const {
  return check<type>(val);
}

const value_representation::str &value_representation::to_array(str &in) const {
  union {
    char bytes[2];
    std::uint16_t val;
  } u;
  u.val = val;

  in[0] = u.bytes[1] + 'A';
  in[1] = u.bytes[0] + 'A';
  return in;
}

value_representation value_representation::make_from_type(int val) {
  value_representation ret;
  ret.val = val;
  return ret;
}

value_representation value_representation::make_from_string(const char *in) {
  union {
    char bytes[2];
    std::uint16_t val;
  } u;
  u.bytes[0] = in[1] - 'A';
  u.bytes[1] = in[0] - 'A';
  return make_from_type(u.val);
}

std::ostream &operator<<(std::ostream &os, const value_representation &vr) {
  return os << vr.val;
}

}  // end namespace detail
}  // end namespace dicm
