#ifndef __imgz_h__
#define __imgz_h__

#ifdef __cplusplus
extern "C" {
#endif


#include "lzss.h"
#define RGBA_SIZE 4

typedef struct imgz_decoder_cb_s{
  int32_t (*on_header)(void *ud, uint8_t width, uint8_t height, uint8_t mode, uint8_t palette_count);
  int32_t (*on_palette)(void *ud, uint8_t *palettes[RGBA_SIZE], uint8_t palette_count);
  int32_t (*on_pixel)(void *ud, int32_t x, int32_t y, uint8_t rgba[RGBA_SIZE]);
  int32_t (*on_done)(void *ud);
  void *ud; // user data
}imgz_decoder_cb_t;

typedef struct pixel_in_s {
  int32_t size, pos;
  const uint8_t *base;
}pixel_in_t;

typedef struct pixel_out_s {
  uint8_t width, height, mode, palette_count;
  uint8_t palettes[256][RGBA_SIZE]; // RGBA format
  int32_t parse_state, wrote, pos_pixel_begin, pos_pixel_end;
  imgz_decoder_cb_t cb;
}pixel_out_t;

typedef struct imgz_stream_s {
  pixel_in_t in;
  pixel_out_t out;
}imgz_stream_t;

int32_t imgz_decode(lzss_t *lz, imgz_stream_t *stream, const uint8_t *encode_data, int32_t data_len, imgz_decoder_cb_t *cb);

#ifdef __cplusplus
}
#endif

#endif //__imgz_h__
