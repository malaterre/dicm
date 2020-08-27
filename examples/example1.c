#include "dicm.h"

#include "dicm-private.h"

#include <stdlib.h> /* EXIT_SUCCESS */
#include <stdio.h> /* fopen */
#include <errno.h> /* errno */
#include <assert.h> /* assert */

static int fsrc_open(struct _src *src, const char *fspec)
{
  FILE *file = fopen(fspec, "rb");
  src->data = file;
  return errno;
}

static int fsrc_close(struct _src *src)
{
  fclose(src->data);
  return errno;
}

static int fsrc_read(struct _src *src, char *buf, size_t bsize)
{
  const size_t read = fread(buf, 1, bsize, src->data);
  /**  fread() does not distinguish between end-of-file and error, and callers
   * must use feof(3) and ferror(3) to determine which occurred. */
  if( read != bsize ) {
   if( feof(src->data) != 0 ) {
     return errno;
   }
}
  assert( bsize == read );
  return 0;
}

static int fdst_open(struct _dst *dst, const char *fspec)
{
  FILE *file = fopen(fspec, "wb");
  dst->data = file;
  return errno;
}

static int fdst_close(struct _dst *dst)
{
  fclose(dst->data);
  return errno;
}

static int fdst_write(struct _dst *dst, char *buf, size_t bsize)
{
  const size_t write = fwrite(buf, 1, bsize, dst->data);
  if( write != bsize ) {
    return errno;
  }
  return 0;
}

static const struct _src_ops fsrc_ops = {
  .open  = fsrc_open,
  .close = fsrc_close,
  .read  = fsrc_read,
};

static const struct _dst_ops fdst_ops = {
  .open  = fdst_open,
  .close = fdst_close,
  .write = fdst_write,
};

int main(__maybe_unused int argc, __maybe_unused char *argv[])
{
  src_t fsrc;
  dst_t fdst;
  dicm_sreader_t sreader;

  fsrc.ops = &fsrc_ops;

  fdst.ops = &fdst_ops;

  fsrc.ops->open( &fsrc, "input.dcm" );
  fdst.ops->open( &fdst, "output.dcm" );

//  sreader.init( &sreader, &fsrc );
//  while(sreader.has_next(&sreader)) { }
//  sreader.fini( &sreader, &fsrc );

 dicm_sreader_init(&sreader, &fsrc);
 while( dicm_sreader_hasnext(&sreader))
{
 dicm_sreader_next(&sreader);
}
 dicm_sreader_fini(&sreader);


  fsrc.ops->close( &fsrc );
  fdst.ops->close( &fdst );

  return EXIT_SUCCESS;
}
