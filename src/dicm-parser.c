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

#include "dicm-parser.h"

#include "dicm-private.h"
#include "dicm.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAKE_TAG(group, element) (group << 16 | element)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAP_TAG(t) t.tag = MAKE_TAG(t.tags[0], t.tags[1])
#else
#define SWAP_TAG(t)                \
  t.tags[0] = bswap_16(t.tags[0]); \
  t.tags[1] = bswap_16(t.tags[1])
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MAKE_VR(left, right) (right << 8 | left)
#else
#define MAKE_VR(left, right) (left << 8 | right)
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAP_VL(vl)
#define SWAP_VL16(vl)
#else
#define SWAP_VL(vl) vl = bswap_32(vl)
#define SWAP_VL16(vl) vl = bswap_16(vl)
#endif

// Full list of VRs as of DICOM 2017a
enum VR {
  kINVALID = 0, /* Item, Item Delimitation Item & Sequence Delimitation Item */
  kAE = MAKE_VR('A', 'E'),
  kAS = MAKE_VR('A', 'S'),
  kAT = MAKE_VR('A', 'T'),
  kCS = MAKE_VR('C', 'S'),
  kDA = MAKE_VR('D', 'A'),
  kDS = MAKE_VR('D', 'S'),
  kDT = MAKE_VR('D', 'T'),
  kFL = MAKE_VR('F', 'L'),
  kFD = MAKE_VR('F', 'D'),
  kIS = MAKE_VR('I', 'S'),
  kLO = MAKE_VR('L', 'O'),
  kLT = MAKE_VR('L', 'T'),
  kOB = MAKE_VR('O', 'B'),
  kOD = MAKE_VR('O', 'D'),
  kOF = MAKE_VR('O', 'F'),
  kOL = MAKE_VR('O', 'L'),
  kOW = MAKE_VR('O', 'W'),
  kPN = MAKE_VR('P', 'N'),
  kSH = MAKE_VR('S', 'H'),
  kSL = MAKE_VR('S', 'L'),
  kSQ = MAKE_VR('S', 'Q'),
  kSS = MAKE_VR('S', 'S'),
  kST = MAKE_VR('S', 'T'),
  kTM = MAKE_VR('T', 'M'),
  kUC = MAKE_VR('U', 'C'),
  kUI = MAKE_VR('U', 'I'),
  kUL = MAKE_VR('U', 'L'),
  kUN = MAKE_VR('U', 'N'),
  kUR = MAKE_VR('U', 'R'),
  kUS = MAKE_VR('U', 'S'),
  kUT = MAKE_VR('U', 'T'),
};

enum {
  kPixelData = MAKE_TAG(0x7fe0, 0x0010),
  kStart = MAKE_TAG(0xfffe, 0xe000),
  kEndItem = MAKE_TAG(0xfffe, 0xe00d),
  kEndSQ = MAKE_TAG(0xfffe, 0xe0dd),
};

bool dicm_de_is_start(const struct _dataelement *de) {
  return de->tag == (tag_t)kStart;
}

bool dicm_de_is_end_item(const struct _dataelement *de) {
  return de->tag == (tag_t)kEndItem;
}

bool dicm_de_is_end_sq(const struct _dataelement *de) {
  return de->tag == (tag_t)kEndSQ;
}

static inline bool is_vr16(const vr_t vr) {
  switch (vr) {
    case kAE:
    case kAS:
    case kAT:
    case kCS:
    case kDA:
    case kDS:
    case kDT:
    case kFD:
    case kFL:
    case kIS:
    case kLO:
    case kLT:
    case kPN:
    case kSH:
    case kSL:
    case kSS:
    case kST:
    case kTM:
    case kUI:
    case kUL:
    case kUS:
      return true;
  }
  return false;
}

static inline bool is_vr32(const vr_t vr) {
  switch (vr) {
    case kAE:
    case kAS:
    case kAT:
    case kCS:
    case kDA:
    case kDS:
    case kDT:
    case kFD:
    case kFL:
    case kIS:
    case kLO:
    case kLT:
    case kPN:
    case kSH:
    case kSL:
    case kSS:
    case kST:
    case kTM:
    case kUI:
    case kUL:
    case kUS:
      /* 16bits: */
      return false;
    case kOB:
    case kOD:
    case kOF:
    case kOL:
    case kOW:
    case kSQ:
    case kUC:
    case kUN:
    case kUR:
    case kUT:
      /* 32bits: */
      return true;
    default:
      /* parser error, or newer DICOM standard */
      /* return 32bits by default (required) */
      return true;
  }
}

static inline bool isvr_valid(const uvr_t uvr) {
  if (uvr.str[0] < 'A' || uvr.str[0] > 'Z' || /* uppercase A-Z only */
      uvr.str[1] < 'A' || uvr.str[1] > 'Z')
    return false;
  return true;
}

static inline bool tag_is_equal(const struct _dataelement *de, tag_t tag) {
  return de->tag == tag;
}

static inline bool tag_is_lower(const struct _dataelement *de, tag_t tag) {
  return de->tag < tag;
}

static inline bool is_start(const struct _dataelement *de) {
  static const tag_t start = MAKE_TAG(0xfffe, 0xe000);
  return de->tag == start;
}
static inline bool is_end_item(const struct _dataelement *de) {
  static const tag_t end_item = MAKE_TAG(0xfffe, 0xe00d);
  return de->tag == end_item;
}
static inline bool is_end_sq(const struct _dataelement *de) {
  static const tag_t end_sq = MAKE_TAG(0xfffe, 0xe0dd);
  return de->tag == end_sq;
}
static inline bool is_encapsulated_pixel_data(const struct _dataelement *de) {
  static const tag_t pixel_data = MAKE_TAG(0x7fe0, 0x0010);
  const bool is_pixel_data = tag_is_equal(de, pixel_data);
  if (is_pixel_data) {
    // Make sure Pixel Data is Encapsulated (Sequence of Fragments):
    if (de->vl == (uint32_t)-1 && (de->vr == kOB || de->vr == kOW)) {
      return true;
    }
  }
  return false;
}
static inline bool is_undef_len(const struct _dataelement *de) {
  const bool b = de->vl == (uint32_t)-1;
  if (b) {
    return de->vr == kSQ || is_encapsulated_pixel_data(de) || is_start(de);
  }
  return b;
}
static inline uint32_t compute_len(const struct _dataelement *de) {
  assert(!is_undef_len(de));
  if (is_vr16(de->vr)) {
    return 4 /* tag */ + 4 /* VR/VL */ + de->vl /* VL */;
  }
  return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + de->vl /* VL */;
}
static inline uint32_t compute_undef_len(const struct _dataelement *de,
                                         uint32_t len) {
  assert(is_undef_len(de));
  assert(len != (uint32_t)-1);
  return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + len;
}

static int read_explicit1(struct _dataelement *de, const char *buf,
                          size_t len) {
  uvr_t vr;

  assert(len == 2);
  // Value Representation
  memcpy(vr.str, buf + 0, sizeof vr.str);
  /* a lot of VR are not valid (eg: non-ASCII), however the standard may add
   * them in a future edition, so only exclude the impossible ones */
  if (!isvr_valid(vr)) return -kDicmInvalidVR;

  de->vr = vr.vr;
  return 0;
}

static int read_explicit2(struct _dataelement *de, const char *buf,
                          size_t len) {
  uvl_t vl;

  // Value Length
  if (!is_vr16(de->vr)) {
    assert(len == 4);
    memcpy(vl.bytes, buf + 0 + 0 + 0, 1 * 4);
    SWAP_VL(vl.vl);
  } else {
    assert(len == 2);
    // padding and/or 16bits VL
    uvl16_t vl16;
    memcpy(vl16.bytes, buf + 0 + 0, sizeof *vl16.bytes * 2);

    SWAP_VL16(vl16.vl16);
    vl.vl = vl16.vl16;
  }
  de->vl = vl.vl;
  return true;
}

static inline size_t get_explicit2_len(struct _dataelement *de) {
  if (is_vr16(de->vr)) {
    return 2;
  }
  return 4;
}

int read_explicit(struct _src *src, struct _dataelement *de) {
  // http://dicom.nema.org/medical/dicom/current/output/chtml/part05/chapter_7.html#sect_7.1.2
  typedef union {
    char str[12];
    struct {
      utag_t utag;
      uvr32_t uvr;
      uvl_t uvl;
    } ede;  // explicit data element. 12 bytes
    struct {
      utag_t utag;
      uvr_t uvr;
      uvl16_t uvl;
    } ede16;  // explicit data element, VR 16. 8 bytes
    struct {
      utag_t utag;
      uvl_t uvl;
    } ide;  // implicit data element. 8 bytes
  } ude_t;
  assert( sizeof(ude_t) == 12 );

  ude_t ude;
  size_t ret = src->ops->read(src, ude.str, 8);
  if (ret < 8) {
    return -kNotEnoughData;
  }
  SWAP_TAG(ude.ede.utag);

  if (ude.ede.utag.tag == (tag_t)kStart /*is_start(de)*/) {
    de->tag = ude.ide.utag.tag;
    de->vr = kINVALID;
    de->vl = ude.ide.uvl.vl;

    if (de->vl != kUndefinedLength) {
      src->ops->seek(src, de->vl);
    }

    return 0;
  } else if (ude.ide.utag.tag == (tag_t)kEndItem /*is_end_item(de)*/
             || ude.ide.utag.tag == (tag_t)kEndSQ /*is_end_sq(de)*/) {
    de->tag = ude.ide.utag.tag;
    de->vr = kINVALID;
    de->vl = ude.ide.uvl.vl;

    assert( de->vl == 0 );

    return 0;
  }

  if (!tag_is_lower(de, ude.ide.utag.tag)) {
    assert(0);
    return -kDicmOutOfOrder;
  }

  // VR16 ?
  if (is_vr16(ude.ede16.uvr.vr)) {
    de->tag = ude.ede16.utag.tag;
    de->vr = ude.ede16.uvr.vr;
    de->vl = ude.ede16.uvl.vl16;
  } else {
    // padding must be set to zero
    if (ude.ede.uvr.vr.reserved != 0) return -kDicmPaddingNotZero;

    ret = src->ops->read(src, ude.ede.uvl.bytes, 4);
    if (ret < 4) return -kNotEnoughData;

    de->tag = ude.ede.utag.tag;
    de->vr = ude.ede.uvr.vr.vr;
    de->vl = ude.ede.uvl.vl;
  }

  if (de->vl != kUndefinedLength) src->ops->seek(src, de->vl);

  return 0;
}

int read_explicit_old(struct _src *src, struct _dataelement *de) {
  char buf[16];
  size_t ret = src->ops->read(src, buf, 4);
  if (ret < 4) {
    return -kNotEnoughData;
  }
  utag_t t;

  // Tag
  memcpy(t.tags, buf, sizeof t.tags);
  SWAP_TAG(t);

  if (t.tag == (tag_t)kStart /*is_start(de)*/) {
    de->tag = t.tag;
    de->vr = kINVALID;
    de->vl = 0;

    ret = src->ops->read(src, buf + 4, 4);
    if (ret < 4) {
      return -kNotEnoughData;
    }
    read_explicit2(de, buf + 4, 4);
    if (de->vl != kUndefinedLength) src->ops->seek(src, de->vl);

    return 0;
  }
  if (t.tag == (tag_t)kEndItem /*is_end_item(de)*/) {
    de->tag = t.tag;
    de->vr = kINVALID;
    de->vl = 0;

    ret = src->ops->read(src, buf + 4, 4);
    if (ret < 4) return -kNotEnoughData;
    read_explicit2(de, buf + 4, 4);

    return 0;
  }
  if (t.tag == (tag_t)kEndSQ /*is_end_sq(de)*/) {
    de->tag = t.tag;
    de->vr = kINVALID;
    de->vl = 0;

    ret = src->ops->read(src, buf + 4, 4);
    if (ret < 4) return -kNotEnoughData;
    read_explicit2(de, buf + 4, 4);

    return 0;
  }
  if (!tag_is_lower(de, t.tag)) {
    assert(0);
    return -kDicmOutOfOrder;
  }

  de->tag = t.tag;

  ret = src->ops->read(src, buf + 4, 0 + 2);
  if (ret < 2) return -kNotEnoughData;
  read_explicit1(de, buf + 4, 2);
  // VR16 ?
  if (!is_vr16(de->vr)) {
    uvl16_t vl16;
    ret = src->ops->read(src, vl16.bytes, 2);
    /* padding must be set to zero */
    if (vl16.vl16 != 0) return -kDicmPaddingNotZero;
  }

  size_t llen = get_explicit2_len(de);
  ret = src->ops->read(src, buf + 6, llen);
  if (ret < llen) return -kNotEnoughData;
  read_explicit2(de, buf + 6, llen);
  if (de->vl != kUndefinedLength) src->ops->seek(src, de->vl);
  return 0;
}
