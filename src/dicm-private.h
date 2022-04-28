/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-features.h"

// strnlen requires >= 200809
#define _POSIX_C_SOURCE 200809L

#include <stdint.h> /* uint32_t */

#define _DICM_POISON(replacement) error_use_##replacement##_instead
//#define fseek _DICM_POISON(fseeko)
//#define ftell _DICM_POISON(ftello)
#define strtod _DICM_POISON(_dicm_parse_double)

static inline bool _is_vr16(const uint32_t vr) {
  switch (vr) {
    case VR_AE:
    case VR_AS:
    case VR_AT:
    case VR_CS:
    case VR_DA:
    case VR_DS:
    case VR_DT:
    case VR_FD:
    case VR_FL:
    case VR_IS:
    case VR_LO:
    case VR_LT:
    case VR_PN:
    case VR_SH:
    case VR_SL:
    case VR_SS:
    case VR_ST:
    case VR_TM:
    case VR_UI:
    case VR_UL:
    case VR_US:
      return true;
  }
  return false;
}

struct _ede32 {
  uint32_t tag;
  uint32_t vr;
  uint32_t vl;
};  // explicit data element. 12 bytes

struct _ede16 {
  uint32_t tag;
  uint16_t vr16;
  uint16_t vl16;
};  // explicit data element, VR/VL 16. 8 bytes

struct _ide {
  uint32_t tag;
  uint32_t vl;
};  // implicit data element. 8 bytes

union _ude {
  uint32_t bytes[4];    // 16 bytes, 32bits aligned
  struct _ede32 ede32;  // explicit data element (12 bytes)
  struct _ede16 ede16;  // explicit data element (8 bytes)
  struct _ide ide;      // implicit data element (8 bytes)
};

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SWAP_TAG(t) t = (t >> 16) | (t >> 16)
#else
#error
#endif

static inline uint32_t _ide_get_tag(const union _ude *ude) {
  // byte-swap tag:
  union {
    uint32_t t;
    uint16_t a[2];
  } u;
  u.t = ude->ide.tag;
  return (uint32_t)(u.a[0] << 16u | u.a[1]);
}
static inline void _ide_set_tag(union _ude *ude, const uint32_t tag) {
  // byte-swap tag:
  union {
    uint32_t t;
    uint16_t a[2];
  } u;
  u.t = tag;
  ude->ide.tag = (uint32_t)(u.a[0] << 16u | u.a[1]);
}
static inline uint32_t _ide_get_vl(const union _ude *ude) {
  return ude->ide.vl;
}
static inline void _ide_set_vl(union _ude *ude, const uint32_t vl) {
  ude->ide.vl = vl;
}

static inline uint32_t _ede16_get_vr(const union _ude *ude) {
  return ude->ede16.vr16;
}
static inline void _ede16_set_vr(union _ude *ude, const uint32_t vr) {
  assert(vr < 0x5A5A);
  ude->ede16.vr16 = (uint16_t)vr;
}
static inline void _ede32_set_vr(union _ude *ude, const uint32_t vr) {
  ude->ede32.vr = vr;
}

static inline uint32_t _ede16_get_vl(const union _ude *ude) {
  return ude->ede16.vl16;
}

static inline void _ede16_set_vl(union _ude *ude, const uint32_t vl) {
  assert(vl <= UINT16_MAX);
  ude->ede16.vl16 = (uint16_t)vl;
}

static inline uint32_t _ede32_get_vl(const union _ude *ude) {
  return ude->ede32.vl;
}
static inline void _ede32_set_vl(union _ude *ude, const uint32_t vl) {
  ude->ede32.vl = vl;
}

static inline bool _ude_init(union _ude *ude, const struct dicm_attribute *da) {
  _ide_set_tag(ude, da->tag);
  const bool is_vr16 = _is_vr16(da->vr);
  if (is_vr16) {
    _ede16_set_vr(ude, da->vr);
    _ede16_set_vl(ude, da->vl);
  } else {
    _ede32_set_vr(ude, da->vr);
    _ede32_set_vl(ude, da->vl);
  }

  return is_vr16;
}
