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

// Define the public types that are exposed to the user level:
typedef char byte_t;
// Fast access to Tag (group, element)
typedef uint32_t tag_t;
// A value representation (upper case ASCII)
typedef byte_t(vr_t)[2];
// A value length (-1 means undefined)
typedef uint32_t vl_t;

static inline uint_fast16_t get_group(tag_t tag) {
  return (uint16_t)(tag >> 16);
}
static inline uint_fast16_t get_element(tag_t tag) {
  return (uint16_t)(tag & (uint16_t)0xffff);
}
static inline const char * get_vr(vr_t vr) {
  return vr;
}
struct _dataelement {
  tag_t tag;
  vr_t vr;
  /*
   * Implementation design. VL is part of the dataelement, since there is a
   * tight relation in between VR and VL.
   */
  vl_t vl;
};

// FIXME copy paste of _dataelement so that compiler know this is a different type...(FIXME???)
struct _filemetaelement {
  tag_t tag;
  vr_t vr;
  vl_t vl;
};

typedef struct _dataelement dataelement_t;
typedef struct _filemetaelement filemetaelement_t;