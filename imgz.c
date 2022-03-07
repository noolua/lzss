#include "imgz.h"
#include <string.h>
#define IMGZ_HEAD_SIZE 4

enum{
  IMGZ_ERROR = -1,
  IMGZ_READY = 0,
  IMGZ_HEAD_DONE,
  IMGZ_PALETTE_DONE,
  IMGZ_PIXEL_DONE,
};

static int32_t pixel_read(void* fp, void* buff, int32_t buff_len) {
  int32_t read_sz = EOF;
  pixel_in_memory_t *in = (pixel_in_memory_t*)fp;
  if(in->pos < in->size && buff_len > 0) {
    read_sz = (in->size - in->pos) > buff_len ? buff_len : (in->size - in->pos);
    memcpy(buff, in->base+in->pos, read_sz);
    in->pos += read_sz;
  }
  return read_sz;
}

static int32_t pixel_putc(void *fp, uint8_t c){
  pixel_out_t *out = (pixel_out_t*)fp;
  if(out->parse_state == IMGZ_READY){
    if(out->wrote == 0) out->width = c;
    else if(out->wrote == 1) out->height = c;
    else if(out->wrote == 2) out->mode = c;
    else if(out->wrote == 3) {
      out->palette_count = c;
      out->parse_state = IMGZ_HEAD_DONE;
      out->pos_pixel_begin = IMGZ_HEAD_SIZE + c * RGBA_SIZE;
      out->pos_pixel_end = out->pos_pixel_begin + out->width * out->height;
      if(out->cb.on_header)
        out->cb.on_header(out->cb.ud, out->width, out->height, out->mode, out->palette_count);
    }
    out->wrote++;
  }else if(out->parse_state == IMGZ_HEAD_DONE){
    uint8_t *pl = (uint8_t*)out->palettes;
    pl[out->wrote - IMGZ_HEAD_SIZE] = c;
    out->wrote++;
    if(out->wrote == out->pos_pixel_begin){
      out->parse_state = IMGZ_PALETTE_DONE;
      if(out->cb.on_palette)
        out->cb.on_palette(out->cb.ud, (uint8_t **)out->palettes, out->palette_count);
    }
  }else if(out->parse_state == IMGZ_PALETTE_DONE){
    int32_t pixel_index = out->wrote - out->pos_pixel_begin;
    int32_t x = pixel_index % out->width, y = pixel_index / out->width;
    if(out->cb.on_pixel)
      out->cb.on_pixel(out->cb.ud, x, y, out->palettes[c]);
    out->wrote++;
    if(out->wrote == out->pos_pixel_end){
      out->parse_state = IMGZ_PIXEL_DONE;
      if(out->cb.on_done)
        out->cb.on_done(out->cb.ud);
    }
  }
  return 0;
}

static int32_t pixel_write(void *fp, void* data, int32_t data_len) {
  int32_t consume = 0;
  uint8_t *base = (uint8_t *)data;
  while(consume < data_len) {
    pixel_putc(fp, base[consume++]);
  }
  return data_len;
}

/*
  PUBLIC API
*/
int32_t imgz_decode(lzss_t *lz, imgz_stream_t *stream, const uint8_t *encode_data, int32_t data_len, imgz_decoder_cb_t *cb){
  if(!encode_data || data_len < 0 || !stream) return -1;
  pixel_in_memory_t *px_in = &stream->in;
  pixel_out_t *px_out = &stream->out;

  // init
  px_in->size = data_len;
  px_in->pos = 0;
  px_in->base = encode_data;

  memset(px_out, 0, sizeof(pixel_out_t));
  if(cb){
    px_out->cb.on_header = cb->on_header;
    px_out->cb.on_palette = cb->on_palette;
    px_out->cb.on_pixel = cb->on_pixel;
    px_out->cb.on_done = cb->on_done;
    px_out->cb.ud = cb->ud;
  }

  lzss_io_t iostream = {
    .read = pixel_read,
    .write = pixel_write,
    .in = px_in,
    .out = px_out
  };

  // decoding
  lzss_init_custom(lz, &iostream);
  return lzss_decode(lz);
}

int32_t imgz_decode_file(lzss_t *lz, imgz_filestream_t *stream, imgz_decoder_cb_t *cb) {
  if(!stream || !stream->in.file || !stream->in.read) return -1;
  pixel_out_t *px_out = &stream->out;

  // init

  memset(px_out, 0, sizeof(pixel_out_t));
  if(cb){
    px_out->cb.on_header = cb->on_header;
    px_out->cb.on_palette = cb->on_palette;
    px_out->cb.on_pixel = cb->on_pixel;
    px_out->cb.on_done = cb->on_done;
    px_out->cb.ud = cb->ud;
  }

  lzss_io_t iostream = {
    .read = stream->in.read,
    .write = pixel_write,
    .in = stream->in.file,
    .out = px_out
  };

  // decoding
  lzss_init_custom(lz, &iostream);
  return lzss_decode(lz);
}
