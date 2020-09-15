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

bool dicm_de_is_start(const struct _dataelement *de) {
  return de->tag == (tag_t)kStart;
}

bool dicm_de_is_end_item(const struct _dataelement *de) {
  return de->tag == (tag_t)kEndItem;
}

bool dicm_de_is_end_sq(const struct _dataelement *de) {
  return de->tag == (tag_t)kEndSQ;
}

bool dicm_de_is_encapsulated_pixel_data(const struct _dataelement *de) {
  return is_encapsulated_pixel_data(de);
}

bool dicm_de_is_sq(const struct _dataelement *de) {
  if (de->vl == (uint32_t)-1 && de->vr == kSQ) {
// undef sq
    return true;
  }
  else if (de->vr == kSQ) {
// defined len sq
    return true;
  }
  return false;
}


