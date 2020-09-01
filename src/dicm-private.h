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

//#include <sys/types.h> /* off_t */
#include <stdint.h> /* uint16_t */

enum state {
  kStartInstance = 0,
  // http://dicom.nema.org/medical/dicom/current/output/chtml/part10/chapter_7.html#table_7.1-1
  kFilePreamble,
  kPrefix,
  kFileMetaElement,
  kDataElement,
  kItem,                      // (FFFE,E000)
  kItemDelimitationItem,      // (FFFE,E00D)
  kSequenceDelimitationItem,  // (FFFE,E0DD)
  kEndInstance
};

#define _DICM_POISON(replacement) error__use_##replacement##_instead
#define fseek _DICM_POISON(fseeko)
#define ftell _DICM_POISON(ftello)
#define strtod _DICM_POISON(_dicm_parse_double)
