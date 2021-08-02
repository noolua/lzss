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

// io_out not use.
typedef struct lzss_chunked_decoder_s{
  lzss_t lz;
  int32_t state, curr_offset, curr_length, curr_k, curr_r, curr_c;
}lzss_chunked_decoder_t;

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



/* init chunked decorder*/
int32_t lzss_chunked_decode_init_std(lzss_chunked_decoder_t *lcdz, FILE *in);

int32_t lzss_chunked_decode_init_custom(lzss_chunked_decoder_t *lcdz, const lzss_io_t *iostream);

/*
  decode input stream partial, then write to out_buffer.
  can pause, resume
  lzss_io_t.out not work
  out_buffer store decode data, pass in real buff_size of out_buffer, then function return, buff_size store used_size.
  return:
    -1 error
     0 success, all data decode complete
     1 success-continue
*/
int32_t lzss_chunked_decode(lzss_chunked_decoder_t *lcdz, uint8_t *out_buffer, int32_t *buff_size);


#ifdef __cplusplus
}
#endif


#endif //__lzss_h__
