/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include <stdint.h>

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
