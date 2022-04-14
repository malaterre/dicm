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
#include "dicm-writer.h"

#include "dicm-public.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct _json {
  struct dicm_writer writer;
  /* data */
  const char *separator;
  bool pretty;
  int indent_level;
  dicm_vr_t vr;
};

/* object */
static DICM_CHECK_RETURN int _json_destroy(void *self_) DICM_NONNULL;

/* writer */
static DICM_CHECK_RETURN int _json_write_start_attribute(
    void *self, const struct dicm_attribute *da) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_attribute(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_value(void *self, const void *buf,
                                               size_t s) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_start_fragment(
    void *self, int frag_num) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_fragment(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_start_item(void *self,
                                                    int item_num) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_item(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_start_sequence(
    void *self, const struct dicm_attribute *da) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_sequence(void *self) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_start_model(
    void *self, const char *encoding) DICM_NONNULL;
static DICM_CHECK_RETURN int _json_write_end_model(void *self) DICM_NONNULL;

static struct writer_vtable const g_vtable =
    {/* object interface */
     .object = {.fp_destroy = _json_destroy},
     /* writer interface */
     .writer = {
         .fp_write_start_attribute = _json_write_start_attribute,
         .fp_write_end_attribute = _json_write_end_attribute,
         .fp_write_value = _json_write_value,
         .fp_write_start_fragment = _json_write_start_fragment,
         .fp_write_end_fragment = _json_write_end_fragment,
         .fp_write_start_item = _json_write_start_item,
         .fp_write_end_item = _json_write_end_item,
         .fp_write_start_sequence = _json_write_start_sequence,
         .fp_write_end_sequence = _json_write_end_sequence,
         .fp_write_start_model = _json_write_start_model,
         .fp_write_end_model = _json_write_end_model,
     }};

int dicm_json_writer_create(struct dicm_writer **pself) {
  struct _json *self = (struct _json *)malloc(sizeof(*self));
  if (self) {
    *pself = &self->writer;
    self->writer.vtable = &g_vtable;
    self->separator = NULL;
    self->pretty = true;
    self->indent_level = 0;
    return 0;
  }
  return 1;
}

/* object */
int _json_destroy(void *self_) {
  struct _json *self = (struct _json *)self_;
  free(self);
  return 0;
}

static void print_with_indent(int indent, char *string) {
  printf("%*s%s", indent, "", string);
}

static void print_indent(void *self_) {
  struct _json *self = (struct _json *)self_;
  if (self->pretty) {
    int level = 2 * self->indent_level;
    print_with_indent(level, "");
  }
}
static void print_eol(void *self_) {
  struct _json *self = (struct _json *)self_;
  if (self->pretty) {
    printf("\n");
  }
}

static void print_separator(void *self_) {
  struct _json *self = (struct _json *)self_;
  if (self->separator) {
    if (self->pretty) printf(self->separator);
  }
}

/* writer */
int _json_write_start_attribute(void *self_, const struct dicm_attribute *da) {
  struct _json *self = (struct _json *)self_;
  print_separator(self_);
  print_indent(self_);

  unsigned int i = dicm_tag_get_group(da->tag);
  unsigned int j = dicm_tag_get_element(da->tag);
  dicm_vr_t vr = da->vr;
  const char *s = dicm_vr_get_string(vr);

  printf("\"%04X%04X\": {", (unsigned int)dicm_tag_get_group(da->tag),
         (unsigned int)dicm_tag_get_element(da->tag));
  self->indent_level++;
  print_eol(self_);
  print_indent(self_);
  printf("\"vr\": \"%.2s\",", s);
  print_eol(self_);
  print_indent(self_);
  printf("\"Value\": [");
  print_eol(self_);

  self->separator = ",\n";
  self->indent_level++;
  self->vr = vr;
  return 0;
}
int _json_write_end_attribute(void *self_) {
  struct _json *self = (struct _json *)self_;
  self->indent_level--;
  print_eol(self_);
  print_indent(self_);
  printf("]");
  print_eol(self_);
  self->indent_level--;
  print_indent(self_);
  printf("}");
  return 0;
}
int _json_write_value(void *self_, const void *buf, size_t s) {
  struct _json *self = (struct _json *)self_;
  print_indent(self_);
  const dicm_vr_t vr = self->vr;
  const char *str = (char *)&vr;
  if (vr == 21315) {  // CS
    const char *value = buf;
    printf("%.*s", s, value);
  } else {
    printf("coucou");
  }
  return 0;
}
int _json_write_start_fragment(void *self, int frag_num) { return 0; }
int _json_write_end_fragment(void *self) { return 0; }
int _json_write_start_item(void *self, int item_num) { return 0; }
int _json_write_end_item(void *self) { return 0; }
int _json_write_start_sequence(void *self, const struct dicm_attribute *da) {
  return 0;
}
int _json_write_end_sequence(void *self) { return 0; }
int _json_write_start_model(void *self_, const char *encoding) {
  struct _json *self = (struct _json *)self_;
  printf("{");
  print_eol(self);
  self->indent_level++;

  return 0;
}
int _json_write_end_model(void *self) { return 0; }
