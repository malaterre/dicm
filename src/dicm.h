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

#include "dicm-features.h"
#include "dicm-io.h"
#include "dicm-errno.h"

struct _dicm_sreader;
// int dicm_sreader_init(struct _dicm_sreader *sreader, struct _src *src);
struct _dicm_sreader *dicm_sreader_init(struct _src *src);
int dicm_sreader_hasnext(struct _dicm_sreader *sreader);
int dicm_sreader_next(struct _dicm_sreader *sreader);
int dicm_sreader_fini(struct _dicm_sreader *sreader);

struct _dataelement;
int dicm_sreader_get_dataelement(struct _dicm_sreader *sreader,
                                 struct _dataelement *de);

typedef struct _dicm_sreader dicm_sreader_t;

typedef struct _src src_t;
typedef struct _dst dst_t;
typedef struct _dataelement dataelement_t;
