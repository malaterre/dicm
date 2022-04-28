/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-features.h"
#include "dicm_export.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

enum ml_event {
  START_MODEL = 0,
  END_MODEL,
  /* attribute */
  START_ATTRIBUTE,
  BYTES, /* kValue */
  /* fragment */
  START_FRAGMENT,
  START_PIXELDATA, /* kStartFragments */
  END_PIXELDATA,   /* kEndFragments */
  /* item */
  START_OBJECT, /* kStartItem */
  END_OBJECT,   /* kEndItem */
  START_ARRAY,  /* kStartSequence */
  END_ARRAY,    /* kEndSequence */
};

enum dicm_state {
  STATE_INVALID = -1,
  STATE_INIT = 0,  // just before "START_DATASET"
  STATE_ATTRIBUTE,
  STATE_VALUE,
  /* fragment */
  STATE_FRAGMENT,
  STATE_STARTFRAGMENTS,
  STATE_ENDFRAGMENTS,
  /* item */
  STATE_STARTITEM,
  STATE_ENDITEM,
  STATE_STARTSEQUENCE,
  STATE_ENDSEQUENCE,
  /* dataset */
  STATE_STARTDATASET,
  STATE_ENDDATASET,
};

enum dicm_token {
  /* attribute */
  TOKEN_ATTRIBUTE = 0,
  /* data value, only when not undefined length */
  TOKEN_VALUE,
  /* fragments (encapsulated pixel data) */
  TOKEN_STARTFRAGMENTS,
  /* item start */
  TOKEN_STARTITEM,
  /* item end */
  TOKEN_ENDITEM,
  /* defined or undefined length sequence */
  TOKEN_STARTSEQUENCE,
  /* sq or fragments end */
  TOKEN_ENDSQITEM,
  /* end of file */
  TOKEN_EOF,
  /* invalid token */
  TOKEN_INVALID_DATA
};

/* attribute */

typedef uint32_t dicm_tag_t;
typedef uint32_t dicm_vr_t;
typedef uint32_t dicm_vl_t;

struct dicm_attribute {
  dicm_tag_t tag;
  dicm_vr_t vr;
  dicm_vl_t vl;
};

/* tag */

/* Retrieve the group part from a tag */
static inline uint_fast16_t dicm_tag_get_group(const dicm_tag_t tag) {
  return (uint16_t)(tag >> 16u);
}
/* Retrieve the element part from a tag */
static inline uint_fast16_t dicm_tag_get_element(const dicm_tag_t tag) {
  return (uint16_t)(tag & 0x0000ffff);
}

#define MAKE_TAG(group, element) (uint32_t)(group << 16u | element)

static inline dicm_tag_t dicm_tag_set_group(const dicm_tag_t tag,
                                            const uint_fast16_t group) {
  const uint_fast16_t element = dicm_tag_get_element(tag);
  return MAKE_TAG(group, element);
}
static inline dicm_tag_t dicm_tag_set_element(const dicm_tag_t tag,
                                              const uint_fast16_t element) {
  const uint_fast16_t group = dicm_tag_get_group(tag);
  return MAKE_TAG(group, element);
}

enum SPECIAL_TAGS {
  TAG_PIXELDATA = MAKE_TAG(0x7fe0, 0x0010),
  TAG_STARTITEM = MAKE_TAG(0xfffe, 0xe000),
  TAG_ENDITEM = MAKE_TAG(0xfffe, 0xe00d),
  TAG_ENDSQITEM = MAKE_TAG(0xfffe, 0xe0dd),
};

static inline bool dicm_tag_is_start_item(const dicm_tag_t tag) {
  return tag == TAG_STARTITEM;
}
static inline bool dicm_tag_is_end_item(const dicm_tag_t tag) {
  return tag == TAG_ENDITEM;
}
static inline bool dicm_tag_is_end_sq_item(const dicm_tag_t tag) {
  return tag == TAG_ENDSQITEM;
}

/* vr */

/* Convert the integer VR representation into a c-string (ASCII) NULL terminated
 */
#define dicm_vr_get_string(vr) ((const char *)&vr)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MAKE_VR(left, right) (right << 8 | left)
#else
#define MAKE_VR(left, right) (left << 8 | right)
#endif

enum VALUE_REPRESENTATIONS {
  VR_NONE = 0,
  VR_AE = MAKE_VR('A', 'E'),
  VR_AS = MAKE_VR('A', 'S'),
  VR_AT = MAKE_VR('A', 'T'),
  VR_CS = MAKE_VR('C', 'S'),
  VR_DA = MAKE_VR('D', 'A'),
  VR_DS = MAKE_VR('D', 'S'),
  VR_DT = MAKE_VR('D', 'T'),
  VR_FL = MAKE_VR('F', 'L'),
  VR_FD = MAKE_VR('F', 'D'),
  VR_IS = MAKE_VR('I', 'S'),
  VR_LO = MAKE_VR('L', 'O'),
  VR_LT = MAKE_VR('L', 'T'),
  VR_OB = MAKE_VR('O', 'B'),
  VR_OD = MAKE_VR('O', 'D'),
  VR_OF = MAKE_VR('O', 'F'),
  VR_OL = MAKE_VR('O', 'L'),
  VR_OV = MAKE_VR('O', 'V'),
  VR_OW = MAKE_VR('O', 'W'),
  VR_PN = MAKE_VR('P', 'N'),
  VR_SH = MAKE_VR('S', 'H'),
  VR_SL = MAKE_VR('S', 'L'),
  VR_SQ = MAKE_VR('S', 'Q'),
  VR_SS = MAKE_VR('S', 'S'),
  VR_ST = MAKE_VR('S', 'T'),
  VR_SV = MAKE_VR('S', 'V'),
  VR_TM = MAKE_VR('T', 'M'),
  VR_UC = MAKE_VR('U', 'C'),
  VR_UI = MAKE_VR('U', 'I'),
  VR_UL = MAKE_VR('U', 'L'),
  VR_UN = MAKE_VR('U', 'N'),
  VR_UR = MAKE_VR('U', 'R'),
  VR_US = MAKE_VR('U', 'S'),
  VR_UT = MAKE_VR('U', 'T'),
  VR_UV = MAKE_VR('U', 'V'),
};

bool dicm_vr_is_16(dicm_vr_t vr);

/* vl */
enum VALUE_LENGTHS { VL_UNDEFINED = 0xffffffff };

static inline bool dicm_vl_is_undefined(const dicm_vl_t vl) {
  return vl == VL_UNDEFINED;
}

/* attribute */
static inline bool dicm_attribute_is_encapsulated_pixel_data(
    const struct dicm_attribute *da) {
  // Make sure Pixel Data is Encapsulated (Sequence of Fragments):
  if (da->tag == TAG_PIXELDATA && da->vl == VL_UNDEFINED && da->vr == VR_OB) {
    return true;
  }
  return false;
}

/* item */

/* fragment */

/* common object private vtable */
struct object_prv_vtable {
  int (*fp_destroy)(void *const) DICM_NONNULL;
};

/* common object interface */
#define object_destroy(t) ((t)->vtable->object.fp_destroy((t)))
