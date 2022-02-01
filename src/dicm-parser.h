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

#pragma once

#include "dicm-de.h"
#include "dicm-io.h"
#include "dicm-private.h"

#include <assert.h>

#define MAKE_TAG(group, element) (group << 16u | element)

#define MAKE_TAG2(group, element) (element << 16u | group)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAP_TAG(t) \
  t.tag = MAKE_TAG((unsigned int)t.tags[0], (unsigned int)t.tags[1])
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
typedef uint16_t vr16_t;
enum VR16_enum {
  kINVALID =
      0x0000, /* Item, Item Delimitation Item & Sequence Delimitation Item */
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

static inline bool is_vr16(const vr_t vr) {
  const vr16_t vr16 = MAKE_VR(vr[0], vr[1]);
  switch (vr16) {
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
  const vr16_t vr16 = MAKE_VR(vr[0], vr[1]);
  switch (vr16) {
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
  if (uvr.bytes[0] < 'A' || uvr.bytes[0] > 'Z' || /* uppercase A-Z only */
      uvr.bytes[1] < 'A' || uvr.bytes[1] > 'Z')
    return false;
  return true;
}

static inline bool tag_is_equal(const struct _dataelement *de, tag_t tag) {
  return de->tag == tag;
}

static inline uint_fast16_t tag_get_group(const tag_t tag) {
#ifdef DOSWAP
  return tag >> 16;
#else
  return tag & 0xffff;
#endif
}

static inline uint_fast16_t tag_get_element(const tag_t tag) {
#ifdef DOSWAP
  return tag & 0xffff;
#else
  return tag >> 16;
#endif
}

static inline bool tag_is_lower(const struct _dataelement *de, tag_t tag) {
#ifdef DOSWAP
  return de->tag < tag;
#else
  return (tag_get_group(de->tag) << 16 | tag_get_element(de->tag)) <
         (tag_get_group(tag) << 16 | tag_get_element(tag));
#endif
}

static inline bool is_start(const struct _dataelement *de) {
  static const tag_t start = MAKE_TAG(0xFFFEu, 0xE000u);
  return de->tag == start;
}
static inline bool is_tag_start(const tag_t tag) {
#ifdef DOSWAP
  const tag_t start = MAKE_TAG(0xfffe, 0xe000);
  return tag == start;
#else
  const tag_t start = MAKE_TAG2(0xFFFEu, 0xE000u);
  return tag == start;
#endif
}
static inline bool is_tag_end_item(const tag_t tag) {
#ifdef DOSWAP
  const tag_t end_item = MAKE_TAG(0xfffe, 0xe00d);
  return tag == end_item;
#else
  const tag_t end_item = MAKE_TAG2(0xFFFEu, 0xE00Du);
  return tag == end_item;
#endif
}
static inline bool is_tag_end_sq(const tag_t tag) {
#ifdef DOSWAP
  const tag_t end_sq = MAKE_TAG(0xfffe, 0xe0dd);
  return tag == end_sq;
#else
  const tag_t end_sq = MAKE_TAG2(0xFFFEu, 0xE0DDu);
  return tag == end_sq;
#endif
}
static inline bool is_tag_pixeldata(const tag_t tag) {
#ifdef DOSWAP
  const tag_t pixel_data = MAKE_TAG(0x7fe0, 0x0010);
  return tag == pixel_data;
#else
  const tag_t pixel_data = MAKE_TAG2(0x7FE0u, 0x0010u);
  return tag == pixel_data;
#endif
}

static inline bool is_end_item(const struct _dataelement *de) {
  static const tag_t end_item = MAKE_TAG(0xFFFEu, 0xE00Du);
  return de->tag == end_item;
}
static inline bool is_end_sq(const struct _dataelement *de) {
  static const tag_t end_sq = MAKE_TAG(0xFFFEu, 0xE0DDu);
  return de->tag == end_sq;
}
static inline bool is_encapsulated_pixel_data(const struct _dataelement *de) {
  //  static const tag_t pixel_data = MAKE_TAG(0x7fe0, 0x0010);
  const bool is_pixel_data = is_tag_pixeldata(de->tag);
  if (is_pixel_data) {
    // Make sure Pixel Data is Encapsulated (Sequence of Fragments):
    if (de->vl == kUndefinedLength && de->vr == kOB) {
      return true;
    }
  }
  return false;
}

static inline bool is_undef_len(const struct _dataelement *de) {
  const bool b = de->vl == kUndefinedLength;
  if (b) {
    return de->vr == kSQ || is_encapsulated_pixel_data(de) || is_start(de);
  }
  return b;
}
#if 0
static inline uint32_t compute_fmelen(const struct _fmeelement *fme) {
  assert(!is_undef_len(fme));
  if (is_vr16(fme->vr)) {
    return 4 /* tag */ + 4 /* VR/VL */ + fme->vl /* VL */;
  }
  return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + fme->vl /* VL */;
}
#endif
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
  (void)de;
  assert(len != (uint32_t)-1);
  return 4 /* tag */ + 4 /* VR */ + 4 /* VL */ + len;
}

struct _ede32 {
  utag_t utag;
  uvr32_t uvr;
  uvl_t uvl;
};  // explicit data element. 12 bytes

struct _ede16 {
  utag_t utag;
  uvr_t uvr;
  uvl16_t uvl;
};  // explicit data element, VR 16. 8 bytes
struct _ide {
  utag_t utag;
  uvl_t uvl;
};  // implicit data element. 8 bytes

DICM_EXPORT int read_filepreamble(struct dicm_io *src, struct _filemetaset *ds);
int read_prefix(struct dicm_io *src, struct _filemetaset *ds);
int read_explicit_impl(struct dicm_io *src, struct _dataset *ds);

int read_fme(struct dicm_io *src, struct _filemetaset *ds);
int buf_into_dataelement(const struct _dataset *ds, enum state current_state,
                         struct _dataelement *de);
int buf_into_filemetaelement(const struct _filemetaset *ds,
                             enum state current_state,
                             struct _filemetaelement *fme);

typedef struct _ede32 ede32_t;
typedef struct _ede16 ede16_t;
typedef struct _ide ide_t;
