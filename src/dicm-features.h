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

//#include <stdbool.h>

#ifdef __GNUC__

#define DICM_UNUSED __attribute__((__unused__))
#define DICM_CHECK_RETURN __attribute__((__warn_unused_result__))
#define DICM_PACKED __attribute__((packed))

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#elif _MSC_VER
#define DICM_UNUSED
#define DICM_CHECK_RETURN
#define DICM_PACKED

#define likely(x) (x)
#define unlikely(x) (x)

#define _CRT_SECURE_NO_WARNINGS

#else
#error sorry
#endif

//#define DOSWAP

// strnlen requires >= 200809
#define _POSIX_C_SOURCE 200809L

#include "dicm_export.h"
