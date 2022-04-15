/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-features.h"
#include "dicm_export.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

enum state {
  kStartModel = 0,
  kEndModel,
  kStartAttribute,
  kEndAttribute,
  kValue,
  kStartFragment,
  kEndFragment,
  kStartItem,
  kEndItem,
  kStartSequence,
  kEndSequence,
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
static inline uint_fast16_t dicm_tag_get_group(dicm_tag_t tag) {
  return (uint16_t)(tag >> 16);
}
/* Retrieve the element part from a tag */
static inline uint_fast16_t dicm_tag_get_element(dicm_tag_t tag) {
  return (uint16_t)(tag & 0x0000ffff);
}

static inline dicm_tag_t dicm_tag_set_group(dicm_tag_t tag,
                                            uint_fast16_t group) {
  assert(0);
  return 0;
}
static inline dicm_tag_t dicm_tag_set_element(dicm_tag_t tag,
                                              uint_fast16_t element) {
  assert(0);
  return 0;
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

/* vl */

static inline bool dicm_vl_is_undefined(dicm_vl_t vl) {
  return vl == 0xffffffff;
}

/* item */

/* fragment */

/* common object private vtable */
struct object_prv_vtable {
  int (*fp_destroy)(void *const) DICM_NONNULL;
};

/* common object interface */
#define object_destroy(t) ((t)->vtable->object.fp_destroy((t)))
