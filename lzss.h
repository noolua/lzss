#ifndef __lzss_h__
#define __lzss_h__
/*
LZSS encoder-decoder (Haruhiko Okumura; public domain)

REF:
  https://oku.edu.mie-u.ac.jp/~okumura/compression/lzss.c
*/

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define OFFSET_BITS  10   /* typically 10..13 */
#define LENGTH_BITS  6    /* typically 4..6 */
#define UNCODED   1       /* If match length <= UNCODED then output one character */
#define BUFF_SIZE (1 << OFFSET_BITS)  /* buffer size */
#define MAX_CODED ((1 << LENGTH_BITS) + UNCODED)  /* lookahead buffer size */

typedef struct lzss_io_s{
  int32_t (*getc)(void* in);
  int32_t (*putc)(void* out, uint8_t c);
  void *in, *out;
}lzss_io_t;

typedef struct lzss_s{
  int32_t bit_buffer, bit_mask, curr_buf, curr_mask;
  uint32_t code_count, text_count;
  uint8_t buffer[BUFF_SIZE<<1];
  lzss_io_t iostream;
}lzss_t;  // sizeof(lzss_t) = 40 + 2KB

/*
  init std FILE io
  usage:
  {
    lzss_t lz;
    int32_t ret = lzss_init_std(&lz);

  }
*/
int32_t lzss_init_std(lzss_t *lz, FILE *in, FILE *out);

/* init lzss with custom iostream*/
int32_t lzss_init_custom(lzss_t* lz, const lzss_io_t *iostream);

/* encode input stream, then write to output stream*/
int32_t lzss_encode(lzss_t *lz);

/* decode input stream, then write to output stream*/
int32_t lzss_decode(lzss_t *lz);


#ifdef __cplusplus
}
#endif


#endif //__lzss_h__
