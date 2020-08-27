#include "dicm.h"

#include "dicm-private.h"
#include "parser.h"

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

static size_t fsrc_read(struct _src *src, void *buf, size_t bsize)
{
  assert( bsize != (size_t)-1 );
  const size_t read = fread(buf, 1, bsize, src->data);
  /**  fread() does not distinguish between end-of-file and error, and callers
   * must use feof(3) and ferror(3) to determine which occurred. */
  if( read != bsize ) {
   if( feof(src->data) != 0 ) {
     return (size_t)-1;
   }
return read;
}
  assert( bsize == read );
  return bsize;
}

static inline offset_t osign(offset_t x) {
    return (x > (offset_t)0) - (x < (offset_t)0);
}

static inline offset_t oabs(offset_t v) 
{
  return v * osign( v );
}

static int fsrc_seek(struct _src *src, offset_t offset)
{
  // return fseeko(src->data, oabs(offset), osign(offset));
  return fseeko(src->data, offset, SEEK_CUR);
}

static offset_t fsrc_tell(struct _src *src)
{
  return ftello(src->data);
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

static size_t fdst_write(struct _dst *dst, void *buf, size_t bsize)
{
  const size_t write = fwrite(buf, 1, bsize, dst->data);
  if( write != bsize ) {
    return bsize;
  }
  return bsize;
}

static const struct _src_ops fsrc_ops = {
  .open  = fsrc_open,
  .close = fsrc_close,
  .read  = fsrc_read,
  .seek  = fsrc_seek,
  .tell  = fsrc_tell,
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
 struct _dataelement de = {0};
 while( dicm_sreader_hasnext(&sreader))
{
 int next = dicm_sreader_next(&sreader);
    switch (next)
    {
        case kStartInstance:
       break;

        case kFilePreamble:
       break;

        case kPrefix:
       break;

        case kFileMetaElement:
        dicm_sreader_get_dataelement(&sreader, &de);
        print_dataelement(&de);

       break;

        case kEndInstance:
        /* Do something different and set current_state */
        break;
    }
 
}
 dicm_sreader_fini(&sreader);


  fsrc.ops->close( &fsrc );
  fdst.ops->close( &fdst );

  return EXIT_SUCCESS;
}
