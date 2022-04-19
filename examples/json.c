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
#include "dicm-public.h"

#include "dicm-writer.h"

#include <assert.h>
#include <float.h>
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
  dicm_vl_t vl;
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
    self->vr = VR_NONE;
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

static void print_with_indent(int indent, const char *string) {
  printf("%*s%s", indent, "", string);
}

static void print_indent(struct _json *self) {
  if (self->pretty) {
    const int level = 2 * self->indent_level;
    print_with_indent(level, "");
  }
}
static void print_eol(struct _json *self) {
  if (self->pretty) {
    printf("\n");
  }
}

static void print_separator(struct _json *self) {
  if (self->separator) {
    if (self->pretty) printf(self->separator);
  }
}

static void print_pre_value(struct _json *self) {
  const dicm_vr_t vr = self->vr;
  const dicm_vr_t vl = self->vl;
  if (!vl) return;
  if (vr == VR_OB || vr == VR_OW) {
    printf(",");
    print_eol(self);
    print_indent(self);
    printf("\"InlineBinary\": \"");
  } else {
    printf(",");
    print_eol(self);
    print_indent(self);
    printf("\"Value\": [");
    print_eol(self);
    self->indent_level++;

    print_indent(self);
  }
}

static void print_post_value(struct _json *self) {
  const dicm_vr_t vr = self->vr;
  const dicm_vr_t vl = self->vl;
  if (!vl) return;

  if (vr == VR_OB || vr == VR_OW) {
    printf("\"");
    print_eol(self);
  } else {
    self->indent_level--;
    print_eol(self);
    print_indent(self);
    printf("]");
    print_eol(self);
  }
}

static void print_simple(struct _json *self, const char *str, size_t len) {
  assert(len);
  printf("\"%.*s\"", len, str);
}

static void print_no_whitespace(struct _json *self, const char *str,
                                size_t len) {
  assert(len);
#if 0
  const char *end = str + len - 1;
  while (str != end && *str == ' ') ++str;
  while (end != str && *end == ' ') --end;
  printf("\"%.*s\"", end - str + 1, str);
#else
  if (str[len - 1] == ' ') len--;
  printf("\"%.*s\"", len, str);
#endif
}

static void print_with_separator(struct _json *self, const char *str,
                                 size_t len, bool quotes) {
  assert(len);
  bool pretty = self->pretty;

  if (str[len - 1] == 0x0) len--;
  if (*str == ' ') {
    ++str;
    --len;
  }
  if (str[len - 1] == ' ') len--;

  if (len && quotes) printf("\"");
  for (const char *pos = str; pos != str + len; ++pos) {
    if (*pos == '\\') {
      if (quotes) printf("\"");
      printf(",");
      if (pretty) printf("\n");
      print_indent(self);
      if (quotes) printf("\"");
    } else {
      printf("%c", *pos);
    }
  }
  if (len && quotes) printf("\"");
}

static void print_person_name(struct _json *self, const char *str, size_t len) {
  assert(len);
  bool pretty = self->pretty;
  printf("{");
  print_eol(self);
  self->indent_level++;
  print_indent(self);

  printf("\"Alphabetic\": ");
  if (str[len - 1] == ' ') len--;
  printf("\"%.*s\"", len, str);
  print_eol(self);
  self->indent_level--;
  print_indent(self);
  printf("}");
}

static void print_signed_short(struct _json *self, const void *buf,
                               size_t len) {
  const int16_t *values = buf;
  const size_t nvalues = len / sizeof(int16_t);
  for (size_t n = 0; n < nvalues; ++n) {
    if (n) {
      printf(",");
      print_eol(self);
      print_indent(self);
    }

    printf("%hd", values[n]);
  }
}

static void print_unsigned_short(struct _json *self, const void *buf,
                                 size_t len) {
  const uint16_t *values = buf;
  const size_t nvalues = len / sizeof(uint16_t);
  for (size_t n = 0; n < nvalues; ++n) {
    if (n) {
      printf(",");
      print_eol(self);
      print_indent(self);
    }
    printf("%hu", values[n]);
  }
}

static void print_signed_long(struct _json *self, const void *buf, size_t len) {
  const int32_t *values = buf;
  const size_t nvalues = len / sizeof(int32_t);
  for (size_t n = 0; n < nvalues; ++n) {
    if (n) {
      printf(",");
      print_eol(self);
      print_indent(self);
    }
    printf("%d", values[n]);
  }
}

static void print_signed_very_long(struct _json *self, const void *buf,
                                   size_t len) {
  const int64_t *values = buf;
  const size_t nvalues = len / sizeof(int64_t);
  for (size_t n = 0; n < nvalues; ++n) {
    if (n) {
      printf(",");
      print_eol(self);
      print_indent(self);
    }
    printf("%ld", values[n]);
  }
}

static void print_unsigned_long(struct _json *self, const void *buf,
                                size_t len) {
  const uint32_t *values = buf;
  const size_t nvalues = len / sizeof(uint32_t);
  for (size_t n = 0; n < nvalues; ++n) {
    if (n) {
      printf(",");
      print_eol(self);
      print_indent(self);
    }
    printf("%u", values[n]);
  }
}

static void print_unsigned_very_long(struct _json *self, const void *buf,
                                     size_t len) {
  const uint64_t *values = buf;
  const size_t nvalues = len / sizeof(uint64_t);
  for (size_t n = 0; n < nvalues; ++n) {
    if (n) {
      printf(",");
      print_eol(self);
      print_indent(self);
    }
    printf("%lu", values[n]);
  }
}

// https://stackoverflow.com/questions/16839658/printf-width-specifier-to-maintain-precision-of-floating-point-value
static void print_float(struct _json *self, const void *buf, size_t len) {
  const float *values = buf;
  const size_t nvalues = len / sizeof(float);
  for (size_t n = 0; n < nvalues; ++n) {
    if (n) {
      printf(",");
      print_eol(self);
      print_indent(self);
    }
    //    printf("%g", values[n]);
    //    printf("%.9g", values[n]);
    printf("%.*g", FLT_DECIMAL_DIG, values[n]);
  }
}

static void print_double(struct _json *self, const void *buf, size_t len) {
  const double *values = buf;
  const size_t nvalues = len / sizeof(double);
  for (size_t n = 0; n < nvalues; ++n) {
    if (n) {
      printf(",");
      print_eol(self);
      print_indent(self);
    }
    //    printf("%g", values[n]);
    //    printf("%.17g", values[n]);
    printf("%.*g", DBL_DECIMAL_DIG, values[n]);
  }
}

static inline size_t base64_encoded_size(size_t inlen) {
  // inlen is uint32_t anyway:
  const size_t outlen = (inlen + 2) / 3;
  return outlen * 4;
}

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static void base64_print(const unsigned char *in, size_t len) {
  assert(len);
  unsigned char out[4];
  for (size_t i = 0, j = 0; i < len; i += 3, j += 4) {
    size_t v = in[i];
    v = i + 1 < len ? v << 8 | in[i + 1] : v << 8;
    v = i + 2 < len ? v << 8 | in[i + 2] : v << 8;

    out[0] = base64_chars[(v >> 18) & 0x3F];
    out[1] = base64_chars[(v >> 12) & 0x3F];
    out[2] = i + 1 < len ? base64_chars[(v >> 6) & 0x3F] : '=';
    out[3] = i + 2 < len ? base64_chars[v & 0x3F] : '=';

    printf("%.*s", sizeof out, out);
  }
}

static void print_inline_binary(struct _json *self, const void *buf, size_t len,
                                bool is_undefined_length) {
  if (!is_undefined_length) {
    const unsigned char *values = buf;
    const size_t nvalues = len / sizeof(unsigned char);
    base64_print(values, nvalues);
  }
}

/* writer */
int _json_write_start_attribute(void *self_, const struct dicm_attribute *da) {
  struct _json *self = (struct _json *)self_;
  print_separator(self);
  print_indent(self);

  const dicm_vr_t vr = da->vr;
  const dicm_vl_t vl = da->vl;

  printf("\"%04X%04X\": {", (unsigned int)dicm_tag_get_group(da->tag),
         (unsigned int)dicm_tag_get_element(da->tag));
  print_eol(self);
  self->indent_level++;
  print_indent(self);
  printf("\"vr\": \"%.2s\"", dicm_vr_get_string(da->vr));
  self->separator = ",\n";
  // store vr/vl so that we know what to do in end_attribute call
  self->vr = vr;
  self->vl = vl;

  print_pre_value(self);

  return 0;
}

int _json_write_end_attribute(void *self_) {
  struct _json *self = (struct _json *)self_;
  print_post_value(self);
  self->indent_level--;
  print_indent(self);
  printf("}");
  return 0;
}

int _json_write_value(void *self_, const void *buf, size_t s) {
  struct _json *self = (struct _json *)self_;
  if (!s) {
    print_eol(self);
    return 0;
  }
  bool is_undefined_length = dicm_vl_is_undefined(self->vl);
  const dicm_vr_t vr = self->vr;
  const char *debug = dicm_vr_get_string(vr);
  switch (vr) {
    case VR_AE:
    case VR_AS:
    case VR_CS:
    case VR_DA:
    case VR_DT:
    case VR_LO:
    case VR_SH:
    case VR_TM:
    case VR_UC:
    case VR_UI:
      assert(!is_undefined_length);
      print_with_separator(self, buf, s, true);
      break;
    case VR_DS:
    case VR_IS:
      assert(!is_undefined_length);
      print_with_separator(self, buf, s, false);
      break;
    case VR_LT: /* cannot be VM 1-N */
    case VR_ST:
    case VR_UR:
    case VR_UT:
      assert(!is_undefined_length);
      print_no_whitespace(self, buf, s);
      break;
    case VR_PN:
      assert(!is_undefined_length);
      print_person_name(self, buf, s);
      break;
      /* binary */
    case VR_OB:
    case VR_OD:
    case VR_OF:
    case VR_OL:
    case VR_OV:
    case VR_OW:
      print_inline_binary(self, buf, s, is_undefined_length);
      break;
    case VR_SL:
      assert(!is_undefined_length);
      print_signed_long(self, buf, s);
      break;
    case VR_SV:
      assert(!is_undefined_length);
      print_signed_very_long(self, buf, s);
      break;
    case VR_SS:
      assert(!is_undefined_length);
      print_signed_short(self, buf, s);
      break;
    case VR_UL:
      assert(!is_undefined_length);
      print_unsigned_long(self, buf, s);
      break;
    case VR_US:
      assert(!is_undefined_length);
      print_unsigned_short(self, buf, s);
      break;
    case VR_UV:
      assert(!is_undefined_length);
      print_unsigned_very_long(self, buf, s);
      break;
    case VR_FL:
      assert(!is_undefined_length);
      print_float(self, buf, s);
      break;
    case VR_FD:
      assert(!is_undefined_length);
      print_double(self, buf, s);
      break;
    case VR_AT:
      assert(!is_undefined_length);
      assert(0);
      break;
    case VR_UN:
      assert(!is_undefined_length);
      assert(0);
      break;
    case VR_SQ:
      assert(0);
      break;
    default:
      assert(0);
  }

  return 0;
}
int _json_write_start_fragment(void *self, int frag_num) { return 0; }
int _json_write_end_fragment(void *self) { return 0; }
int _json_write_start_item(void *self_, int item_num) {
  struct _json *self = (struct _json *)self_;
  print_separator(self);
  if (self->separator) print_indent(self);
  self->separator = NULL;
  self->indent_level++;
  printf("{");
  print_eol(self);

  return 0;
}
int _json_write_end_item(void *self_) {
  struct _json *self = (struct _json *)self_;
  print_eol(self);
  self->indent_level--;
  print_indent(self);
  printf("}");
  //  printf("\n");  // end of item
  return 0;
}
int _json_write_start_sequence(void *self_, const struct dicm_attribute *da) {
  struct _json *self = (struct _json *)self_;
  _json_write_start_attribute(self_, da);
  // self->indent_level++;
  self->separator = NULL;

  return 0;
}
int _json_write_end_sequence(void *self_) {
  struct _json *self = (struct _json *)self_;
  _json_write_end_attribute(self_);
  // self->indent_level--;

  return 0;
}
int _json_write_start_model(void *self_, const char *encoding) {
  struct _json *self = (struct _json *)self_;
  printf("{");
  print_eol(self);
  self->indent_level++;

  return 0;
}
int _json_write_end_model(void *self_) {
  struct _json *self = (struct _json *)self_;
  print_eol(self);
  printf("}");
  printf("\n");  // end of file ?
  self->indent_level--;
  return 0;
}
