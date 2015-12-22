/*
 *  DICM, a library for reading DICOM instances
 *
 *  Copyright (c) 2020 Mathieu Malaterre
 *  All rights reserved.
 *
 *  DICM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, version 2.1.
 *
 *  DICM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with DICM . If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "dicm-uid.h"

#include <assert.h>
#include <string.h>

typedef unsigned char ustr_32_t[32];
typedef char str_64_t[64];

struct _uid {
  str_64_t buf;
};

struct _fast_uid  // FIXME
{
  str_64_t buf;
};

bool uid_from_string(struct _uid *out, const char *in) {
  if (!in) return false;
  const size_t len = strlen(in);
  if (len <= 64) {
    strncpy(out->buf, in, len);
    return true;
  }
  return false;
}

bool fast_uid_from_string(struct _fast_uid *out, const char *in) {
  return uid_from_string((struct _uid *)out, in);
}

struct _packed_uid {
  ustr_32_t buf;
};

#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)
#define LO_NIBBLE(b) ((b)&0x0F)

static inline char base11(char in) {
  // assert in == '.' || 0 <= in <= 9
  if (in == '.') return 0x0a;
  return in - '0';
}

static inline char invbase11(char in) {
  if (in == 0xf)
    return 0x0;
  else if (in == 0xa)
    return '.';
  assert(in >= 0x0 && in <= 0x9);
  return '0' + in;
}

bool packed_uid_from_string(struct _packed_uid *out, const char *in) {
  const char term = 0xf;
  if (!in) return false;
  const size_t len = strlen(in);
  if (len > 64) return false;
  for (unsigned int i = 0; i < len / 2; ++i) {
    char hi = base11(in[2 * i + 0]);
    char lo = base11(in[2 * i + 1]);
    assert(hi >= 0x0 && hi <= 0xa);
    assert(lo >= 0x0 && lo <= 0xa);
    assert(invbase11(hi) == in[2 * i + 0]);
    assert(invbase11(lo) == in[2 * i + 1]);
    out->buf[i] = hi << 4 | lo;
    assert(hi == HI_NIBBLE(out->buf[i]));
    assert(lo == LO_NIBBLE(out->buf[i]));
  }
  if (len % 2) {
    char hi = base11(in[len - 1]);
    char lo = term;
    assert(hi >= 0x0 && hi <= 0xa);
    out->buf[len / 2 + 0] = hi << 4 | lo;
    assert(hi == HI_NIBBLE(out->buf[len / 2 + 0]));
    assert(lo == LO_NIBBLE(out->buf[len / 2 + 0]));
  }
  for (int i = (len + 1) / 2; i < 32; ++i) {
    out->buf[i] = 0xff;
  }
  return true;
}
