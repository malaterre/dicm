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

#include <stdint.h>

typedef uint32_t tag_t;
typedef uint16_t vr_t;
typedef uint32_t vl_t;

typedef union {
  uint16_t tags[2];
  tag_t tag;
} utag_t;
typedef union {
  char str[2];
  vr_t vr;
} uvr_t;
typedef union {
  char bytes[4];
  vl_t vl;
} uvl_t;
typedef union {
  char bytes[2];
  uint16_t vl16;
} uvl16_t;

static inline uint_fast16_t get_group(tag_t tag) {
  return (uint16_t)(tag >> 16);
}
static inline uint_fast16_t get_element(tag_t tag) {
  return (uint16_t)(tag & (uint16_t)0xffff);
}

static inline uvr_t get_vr_impl(vr_t vr) {
  uvr_t ret;
  ret.vr = vr;
  return ret;
}

#define get_vr(vr) get_vr_impl(vr).str

struct _dataelement {
  tag_t tag;
  vr_t vr;
  /*
   * Implementation design. VL is part of the dataelement, since there is a
   * tight relation in between VR and VL.
   */
  vl_t vl;
};

enum {
  kUndefinedLength = (vl_t)-1,
};

// struct _dataelement;
uint16_t dicm_de_get_group(struct _dataelement *de);
vl_t dicm_de_get_vl(struct _dataelement *de);

typedef struct _dataelement dataelement_t;
