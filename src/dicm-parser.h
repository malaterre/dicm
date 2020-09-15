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

struct _ede {
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

int read_explicit(struct _src *src, struct _dataset *ds);

typedef struct _ede ede_t;
typedef struct _ede16 ede16_t;
typedef struct _ide ide_t;
