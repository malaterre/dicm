/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-features.h"

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

typedef uint32_t dicm_tag_t;
typedef uint32_t dicm_vr_t;
typedef uint32_t dicm_vl_t;

struct dicm_attribute {
  dicm_tag_t tag;
  dicm_vr_t vr;
  dicm_vl_t vl;
};

/* Convert the integer VR representation into a c-string (ASCII) NULL terminated
 */
#define dicm_vr_get_string(vr) ((const char *)&vr)

static inline uint_fast16_t dicm_tag_get_group(dicm_tag_t tag) {
  return (uint16_t)(tag >> 16);
}
static inline uint_fast16_t dicm_tag_get_element(dicm_tag_t tag) {
  return (uint16_t)(tag & (uint16_t)0xffff);
}

/* common object private vtable */
struct object_prv_vtable {
  int (*fp_destroy)(void *const) DICM_NONNULL;
};

/* common object interface */
#define object_destroy(t) ((t)->vtable->object.fp_destroy((t)))
