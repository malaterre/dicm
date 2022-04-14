/* SPDX-License-Identifier: LGPLv3 */
#pragma once

#ifdef __GNUC__

#define DICM_UNUSED __attribute__((__unused__))
#define DICM_CHECK_RETURN __attribute__((__warn_unused_result__))
#define DICM_PACKED __attribute__((packed))
#define DICM_NONNULL __attribute__((nonnull))
#define DICM_NONNULL1(x) __attribute__((nonnull(x)))
#define DICM_NONNULL2(x, y) __attribute__((nonnull(x, y)))

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#elif _MSC_VER
#define DICM_UNUSED
#define DICM_CHECK_RETURN
#define DICM_PACKED

#define likely(x) (x)
#define unlikely(x) (x)

#define DICM_NONNULL
#define DICM_NONNULL1(x)
#define DICM_NONNULL2(x, y)

#define _CRT_SECURE_NO_WARNINGS

#else
#error sorry
#endif

//#define DOSWAP

// strnlen requires >= 200809
#define _POSIX_C_SOURCE 200809L

#include "dicm_export.h"
