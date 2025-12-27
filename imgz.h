#ifndef __imgz_h__
#define __imgz_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef __MACH__
  typedef unsigned long imgz_size_t;
#else
  typedef __SIZE_TYPE__ imgz_size_t;
#endif

#define RGBA_SIZE 4

typedef struct imgz_progm_s{
  const uint16_t _size;
  const uint8_t _content[0];
}imgz_progm_t;

#define imgz4progm(d)   (const imgz_progm_t*)(d)


typedef struct imgz_decoder_cb_s{
  int32_t (*on_header)(void *ud, uint8_t width, uint8_t height, uint8_t mode, uint8_t palette_count);
  int32_t (*on_palette)(void *ud, uint8_t (*palettes)[RGBA_SIZE], uint8_t palette_count);
  int32_t (*on_pixel)(void *ud, int32_t x, int32_t y, uint8_t rgba[RGBA_SIZE]);
  int32_t (*on_done)(void *ud);
  void *ud; // user data
}imgz_decoder_cb_t;


void *imgz_decoder_malloc(void *(*func_malloc)(imgz_size_t sz));
void imgz_decoder_free(void *decoder, void (*func_free)(void *));

int32_t imgz_decode_memory(void *decoder, const uint8_t *encode_data, const int32_t data_sz, imgz_decoder_cb_t *cb);
int32_t imgz_decode_by_reader(void *decoder, int32_t (*read)(void *in, void *buff, int32_t buff_len), void *reader_handle, imgz_decoder_cb_t *cb);

/*
int32_t imgz_decode(lzss_t *lz, imgz_stream_t *stream, const uint8_t *encode_data, int32_t data_len, imgz_decoder_cb_t *cb);
int32_t imgz_decode_file(lzss_t *lz, imgz_filestream_t *stream, imgz_decoder_cb_t *cb);
*/

#ifdef __cplusplus
}
#endif

#endif //__imgz_h__
