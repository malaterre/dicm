#include "uid.hxx"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cassert>

#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)
#define LO_NIBBLE(b) ((b) & 0x0F)

namespace dicm {
namespace details {

const uid_string::str &uid_string::to_array(str &in) const {
  strncpy(in, ret.buf, in);
  return in;
}

uid_string uid_string::make_from_string(const char *in) {
  uid_string ret = {};
  if(in)
  {
    const size_t len = strlen(in);
    if( len <= 64 ) {
      strncpy(ret.buf, in, len );
    }
  }
  return ret;
}

std::ostream &operator<<(std::ostream &os, const uid_string &uid) {
  return os << std::string(uid.buf,64);
}

static inline char base11(char in) {
  // assert in == '.' || 0 <= in <= 9
  if(in == '.') return 0x0a;
  return in - '0';
}

static inline char invbase11(char in) {
  if(in == 0xf) return 0x0;
  else if(in == 0xa) return '.';
  assert( in >= 0x0 && in <= 0x9 );
  return '0' + in;
}


const uid_bcd::str &uid_bcd::to_array(str &in) const {
  for( int i = 0; i < 32; ++i )
  {
    in[2*i+0] = invbase11(HI_NIBBLE(buf[i]));
    in[2*i+1] = invbase11(LO_NIBBLE(buf[i]));
  }
  return in;
}

uid_bcd uid_bcd::make_from_string(const char *in) {
  uid_bcd ret = {};
  const char term = 0xf;
  if(in)
  {
    const size_t len = strlen(in);
    if( len <= 64 ) {
      for( int i = 0; i < len/2; ++i ) {
        char hi = base11(in[2*i+0]);
        char lo = base11(in[2*i+1]);
	assert( hi >= 0x0 && hi <= 0xa );
	assert( lo >= 0x0 && lo <= 0xa );
	assert( invbase11(hi) == in[2*i+0] );
	assert( invbase11(lo) == in[2*i+1] );
        ret.buf[i] = hi << 4 | lo;
        assert( hi == HI_NIBBLE(ret.buf[i]));
        assert( lo == LO_NIBBLE(ret.buf[i]));
      }
      if( len % 2 ) {
        char hi = base11(in[len-1]);
        char lo = term;
	assert( hi >= 0x0 && hi <= 0xa );
        ret.buf[len/2+0] = hi << 4 | lo;
        assert( hi == HI_NIBBLE(ret.buf[len/2+0]));
        assert( lo == LO_NIBBLE(ret.buf[len/2+0]));
      }
      for( int i = (len + 1) / 2; i < 32; ++i ) {
        ret.buf[i] = 0xff;
      }
    }
  }
  return ret;
}

std::ostream &operator<<(std::ostream &os, const uid_bcd &uid) {
  uid_bcd::str s;
  uid.to_array(s);
  return os << std::string(s,64);
}

}  // end namespace detail
}  // end namespace dicm
