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
