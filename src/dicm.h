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
#include "dicm-errno.h"
#include "dicm-features.h"
#include "dicm-io.h"
#include "dicm-mem.h"

struct _dicm_sreader;

struct _dicm_sreader *dicm_sreader_init(struct _mem *mem, struct _src *src);
int dicm_sreader_fini(struct _dicm_sreader *sreader);

/**
 * Indicate whether or not there is a next dataelement
 */
__must_check int dicm_sreader_hasnext(struct _dicm_sreader *sreader);

/**
 * Move to next dataelement
 */
int dicm_sreader_next(struct _dicm_sreader *sreader);

__must_check const char *dicm_sreader_get_file_preamble(
    struct _dicm_sreader *sreader);

__must_check const char *dicm_sreader_get_prefix(struct _dicm_sreader *sreader);

/**
 * Return current dataelement
 */
__must_check struct _dataelement *dicm_sreader_get_dataelement(
    struct _dicm_sreader *sreader);

__must_check struct _filemetaelement *dicm_sreader_get_filemetaelement(
    struct _dicm_sreader *sreader);

typedef struct _dicm_sreader dicm_sreader_t;
