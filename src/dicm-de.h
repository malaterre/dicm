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
#include "dicm-public.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  vr_t vr;
  uint16_t reserved;
} vr32_t;
typedef uint16_t vl16_t;

typedef union {
  uint16_t tags[2];
  tag_t tag;
} utag_t;
typedef union {
  byte_t bytes[2];
  vr_t vr;
} uvr_t;
typedef union {
  byte_t bytes[4];
  struct {
    vr_t vr;
    uint16_t reserved;
  } vr;
} uvr32_t;
typedef union {
  byte_t bytes[4];
  vl_t vl;
} uvl_t;
typedef union {
  byte_t bytes[2];
  uint16_t vl16;
} uvl16_t;

// Define the minimal dataset info structure with a max nesting level of one to
// parse defined length SQ + defined length item.
// Implementation should use tail-recursion optimization to allow processing of
// infinite depth defined length SQ.
struct _dataset {
  char buffer[128 /*4096*/];  // Minimal amount of memory (preamble is the
                              // bigest one ?)
  size_t bufsize;             //

  // group length machinery:
  uint_fast16_t curgroup;
  vl_t grouplen;
  vl_t curgrouplen;

  // Fragments are easier to handle since they cannot be nested
  int sequenceoffragments;  // -1: none, 0: BasicOffsetTable, >0: Fragment
  // defined length Item:
  int _levelitem;
#define MAX_LEVEL_ITEM 10
  vl_t _deflenitem[MAX_LEVEL_ITEM];
  vl_t _curdeflenitem[MAX_LEVEL_ITEM];

  // defined length SQ are harder to handle since we can have infinite nesting:
  int _levelsq;
#define MAX_LEVEL_SQ 10
  vl_t _deflensq[MAX_LEVEL_SQ];
  vl_t _curdeflensq[MAX_LEVEL_SQ];
};

enum {
  kUndefinedLength = (vl_t)-1,
};

static inline void pushsqlevel(struct _dataset *ds) {
  assert(ds->_levelsq < MAX_LEVEL_SQ);
  ds->_levelsq++;
  assert(ds->_levelsq >= 0);
}

static inline void popsqlevel(struct _dataset *ds) {
  assert(ds->_levelsq < MAX_LEVEL_SQ);
  assert(ds->_levelsq >= 0);
  ds->_levelsq--;
}

static inline void reset_defined_length_sequence(struct _dataset *ds) {
  ds->_levelsq = -1;
  for (int i = 0; i < MAX_LEVEL_SQ; ++i) {
    ds->_deflensq[i] = kUndefinedLength;
    ds->_curdeflensq[i] = 0;
  }
}

static inline void reset_cur_defined_length_sequence(struct _dataset *ds) {
  assert(ds->_deflensq[ds->_levelsq] != kUndefinedLength);
  ds->_deflensq[ds->_levelsq] = kUndefinedLength;
  ds->_curdeflensq[ds->_levelsq] = 0;
  popsqlevel(ds);
}

static inline void set_deflensq(struct _dataset *ds, vl_t len) {
  pushsqlevel(ds);
  assert(ds->_deflensq[ds->_levelsq] == kUndefinedLength);
  ds->_deflensq[ds->_levelsq] = len;
}

static inline vl_t get_deflensq(struct _dataset *ds) {
  if (ds->_levelsq == -1) return kUndefinedLength;
  assert(ds->_levelsq >= 0);
  assert(ds->_levelsq < MAX_LEVEL_SQ);
  return ds->_deflensq[ds->_levelsq];
}

static inline void set_curdeflensq(struct _dataset *ds, vl_t len) {
  assert(ds->_levelsq >= 0);
  assert(ds->_levelsq < MAX_LEVEL_SQ);
  assert(ds->_curdeflensq[ds->_levelsq] <= ds->_deflensq[ds->_levelsq]);
  ds->_curdeflensq[ds->_levelsq] = len;
  assert(ds->_curdeflensq[ds->_levelsq] <= ds->_deflensq[ds->_levelsq]);
}

static inline vl_t get_curdeflensq(struct _dataset *ds) {
  if (ds->_levelsq == -1) return 0;
  assert(ds->_levelsq >= 0);
  assert(ds->_levelsq < MAX_LEVEL_SQ);
  return ds->_curdeflensq[ds->_levelsq];
}
#undef MAX_LEVEL_SQ
static inline void pushitemlevel(struct _dataset *ds) {
  assert(ds->_levelitem < MAX_LEVEL_ITEM);
  ds->_levelitem++;
  assert(ds->_levelitem >= 0);
}

static inline void popitemlevel(struct _dataset *ds) {
  assert(ds->_levelitem < MAX_LEVEL_ITEM);
  assert(ds->_levelitem >= 0);
  ds->_levelitem--;
}

static inline void reset_defined_length_item(struct _dataset *ds) {
  ds->_levelitem = -1;
  for (int i = 0; i < MAX_LEVEL_ITEM; ++i) {
    ds->_deflenitem[i] = kUndefinedLength;
    ds->_curdeflenitem[i] = 0;
  }
}
static inline void reset_cur_defined_length_item(struct _dataset *ds) {
  assert(ds->_deflenitem[ds->_levelitem] != kUndefinedLength);
  ds->_deflenitem[ds->_levelitem] = kUndefinedLength;
  ds->_curdeflenitem[ds->_levelitem] = 0;
  popitemlevel(ds);
}

static inline void set_deflenitem(struct _dataset *ds, vl_t len) {
  assert(ds->_deflenitem[ds->_levelitem] == kUndefinedLength);
  ds->_deflenitem[ds->_levelitem] = len;
}

static inline vl_t get_deflenitem(struct _dataset *ds) {
  if (ds->_levelitem == -1) return kUndefinedLength;
  assert(ds->_levelitem >= 0);
  assert(ds->_levelitem < MAX_LEVEL_ITEM);
  return ds->_deflenitem[ds->_levelitem];
}

static inline void set_curdeflenitem(struct _dataset *ds, vl_t len) {
  assert(ds->_levelitem >= 0);
  assert(ds->_levelitem < MAX_LEVEL_ITEM);
  assert(ds->_curdeflenitem[ds->_levelitem] <= ds->_deflenitem[ds->_levelitem]);
  ds->_curdeflenitem[ds->_levelitem] = len;
  assert(ds->_curdeflenitem[ds->_levelitem] <= ds->_deflenitem[ds->_levelitem]);
}

static inline vl_t get_curdeflenitem(struct _dataset *ds) {
  if (ds->_levelitem == -1) return 0;
  assert(ds->_levelitem >= 0);
  assert(ds->_levelitem < MAX_LEVEL_ITEM);
  return ds->_curdeflenitem[ds->_levelitem];
}

#undef MAX_LEVEL_ITEM

struct _filemetaset {
  char buffer[128 /*4096*/];  // Minimal amount of memory (preamble is the
                              // bigest one ?)
  size_t bufsize;             //

  // fme length
  vl_t fmelen;
  vl_t curfmelen;
};

static inline void reset_dataset(struct _dataset *ds) {
  ds->curgroup = 0;  // uint_fast16_t
  ds->grouplen = kUndefinedLength;
  ds->curgrouplen = 0;
  reset_defined_length_item(ds);
  reset_defined_length_sequence(ds);

  ds->sequenceoffragments = -1;
  memset(ds->buffer, 0, sizeof ds->buffer);
  ds->bufsize = 0;
}

static inline void reset_filemetaset(struct _filemetaset *ds) {
  ds->fmelen = kUndefinedLength;
  ds->curfmelen = 0;

  memset(ds->buffer, 0, sizeof ds->buffer);
  ds->bufsize = 0;
}

// struct _dataelement;
tag_t dicm_de_get_tag(struct _dataelement *de);
uint16_t dicm_de_get_group(struct _dataelement *de);
vl_t dicm_de_get_vl(struct _dataelement *de);

bool dicm_de_is_start(const struct _dataelement *de);
bool dicm_de_is_end_item(const struct _dataelement *de);
bool dicm_de_is_end_sq(const struct _dataelement *de);
bool dicm_de_is_encapsulated_pixel_data(const struct _dataelement *de);
bool dicm_de_is_sq(const struct _dataelement *de);

// void dicm_de_flush(struct _dataelement *de);
