/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#include "dicm-features.h"

#include <stdint.h> /* uint16_t */

#define _DICM_POISON(replacement) error__use_##replacement##_instead
//#define fseek _DICM_POISON(fseeko)
//#define ftell _DICM_POISON(ftello)
#define strtod _DICM_POISON(_dicm_parse_double)

typedef char byte_t;
// Fast access to Tag (group, element)
typedef uint32_t tag_t;
// A value representation (upper case ASCII)
typedef byte_t(vr_t)[2];
// A value length (-1 means undefined)
typedef uint32_t vl_t;
