#include "imgz.h"
#include <stdio.h>
#include <stdlib.h>

const uint8_t imgz_spr_background_cd00[] = {
 0xea,0x00,0xa8,0x44,0x20,0x90,0x38,0x03,0xc3,0x06,0x61,0x34,0xb6,0xff,0xfb,0xde,
 0xee,0xb7,0xfd,0xe1,0x84,0xf4,0xbf,0x80,0x40,0x5e,0xa7,0xe0,0x1f,0xf1,0x2e,0xd4,
 0x08,0x5a,0x00,0x1c,0xff,0x0b,0x25,0x90,0x40,0x06,0x1f,0xe1,0x61,0x91,0x6a,0x28,
 0xb5,0xe4,0xaa,0x98,0x69,0x40,0x2d,0x61,0x1b,0x50,0x0d,0x29,0x44,0x10,0x13,0x76,
 0x09,0xc7,0x10,0x76,0xb0,0x74,0x06,0x34,0xa7,0x20,0x20,0x50,0x18,0x67,0xe4,0xd4,
 0x0e,0x71,0xe4,0x31,0x0e,0x92,0x8f,0x44,0x46,0x45,0xa0,0x83,0xce,0x48,0x69,0x64,
 0x71,0x68,0x08,0xb4,0x19,0x36,0x8c,0x99,0xc4,0x20,0x04,0xa3,0xd2,0x14,0xa8,0x2a,
 0x94,0x21,0xf4,0x62,0x4b,0x08,0xd9,0x82,0xaa,0xc3,0x2d,0x62,0xac,0x60,0xe0,0x26,
 0xe4,0x45,0x6c,0x42,0x93,0x21,0x26,0xa6,0xb7,0x44,0x55,0x22,0x30,0x40,0x53,0xc1,
 0x09,0xd0,0x66,0x00,0x6a,0xf4,0x18,0xf5,0x8e,0xa8,0xcd,0x66,0xc3,0x32,0x51,0x57,
 0xd9,0x29,0x84,0xb5,0xb2,0x51,0xc6,0x19,0x71,0x88,0xbc,0xca,0x6a,0xe4,0xad,0x51,
 0xd4,0xb8,0xee,0xdc,0x77,0x44,0x6b,0x96,0x15,0x76,0x8a,0xe2,0x05,0x6e,0x04,0x35,
 0x33,0x1d,0x8a,0x07,0x20,0x86,0xfa,0x42,0x68,0x19,0x77,0x96,0xf7,0x06,0x64,0xe5,
 0x26,0xa2,0xd4,0x10,0xe1,0x0c,0xc0,0xe2,0x40,0x41,0x15,0x99,0x90,0xb7,0x86,0x08,
 0xe5,0x8a,0x32,0xda,0x70,0xe2,0x5c,0x91,0x72,0x28,0xc9,0x04
}; // bytes = 236 (0.23KB)

typedef struct imgz_ctx_s{
  uint8_t width, height;
  // lzss_t lz;
  // imgz_stream_t stream;
  // imgz_filestream_t file_stream;
}imgz_ctx_t;

static int32_t on_header_impl(void* ud, uint8_t width, uint8_t height, uint8_t mode, uint8_t palette_count){
  imgz_ctx_t *ctx = (imgz_ctx_t*)ud;
  ctx->width = width;
  ctx->height = height;
  printf("header is: %d, %d, %d, %d\n", width, height, mode, palette_count);
  return 0;
}

static int32_t on_palette_impl(void *ud, uint8_t *palettes[4], uint8_t palette_count){
  printf("palettes: %p, %d\n", palettes, palette_count);
  return 0;
}

static int32_t on_pixel_impl(void *ud, int32_t x, int32_t y, uint8_t rgba[4]){
  imgz_ctx_t *ctx = (imgz_ctx_t*)ud;
  if(rgba[3] != 0){
    printf("*");
  }else{
    printf(" ");
  }
  if(x == ctx->width - 1) printf("\n");
  return 0;
}

static int32_t on_done_impl(void *ud){
  printf("all done\n");
  return 0;
}

int main(int argc, char *argv[]) {
  int ret = -1;
  void *decoder = imgz_decoder_malloc(&malloc);
  imgz_ctx_t ctx;
  if(decoder) {
    imgz_decoder_cb_t callbacks = {0};
    callbacks.on_header = on_header_impl;
    callbacks.on_palette = on_palette_impl;
    callbacks.on_pixel = on_pixel_impl;
    callbacks.on_done = on_done_impl;
    callbacks.ud = &ctx;

    const imgz_progm_t *progm = imgz4progm(imgz_spr_background_cd00);

    // working
    ret = imgz_decode_memory(decoder, progm->_content, progm->_size, &callbacks);

    // cleanup
    imgz_decoder_free(decoder, &free);
  }

  return ret;
}

static int32_t file_read(void* fp, void *buff, int32_t buff_len) {
  int32_t ret = fread(buff, 1, buff_len, (FILE*)fp);
  return ret;
}

int main22(){
  int ret = -1;
  FILE *fp = fopen("cd.imgz", "rb");
  void *decoder = imgz_decoder_malloc(&malloc);
  imgz_ctx_t ctx;

  if(fp && decoder){
    fseek(fp, 2, SEEK_SET); // skip header_length(uint16_t)

    imgz_decoder_cb_t callbacks = {0};
    callbacks.on_header = on_header_impl;
    callbacks.on_palette = on_palette_impl;
    callbacks.on_pixel = on_pixel_impl;
    callbacks.on_done = on_done_impl;
    callbacks.ud = &ctx;

    // working
    ret = imgz_decode_by_reader(decoder, file_read, fp, &callbacks);

    // cleanup
    imgz_decoder_free(decoder, &free);
    fclose(fp);
  }

  return ret;
}
