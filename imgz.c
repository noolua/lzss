#include "lzss.h"
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

enum {
  IMGZ_MEMORY,
  IMGZ_READER
};

typedef struct imgz_decode_stream_s {
  uint8_t width, height, mode, palette_count;
  uint8_t palettes[256][RGBA_SIZE]; // RGBA format
  int32_t parse_state, wrote, pos_pixel_begin, pos_pixel_end;
  int32_t stream_type;
  imgz_decoder_cb_t cb;
  union {
    struct {
      const void *content;
      int32_t content_sz;
      int32_t position;
    } mem;
    struct {
      int32_t (*read)(void *handle, void *buff, int32_t buff_len);
      void *handle;
    } reader;
  }input;
  lzss_t lz;
}imgz_decode_stream_t;

static int32_t pixel_read(void* fp, void* buff, int32_t buff_len) {
  int32_t read_sz = EOF;
  imgz_decode_stream_t *stream = (imgz_decode_stream_t*)fp;
  if(stream->stream_type == IMGZ_MEMORY) {
    if(stream->input.mem.position < stream->input.mem.content_sz && buff_len > 0) {
      read_sz = (stream->input.mem.content_sz - stream->input.mem.position) > buff_len ? buff_len : (stream->input.mem.content_sz - stream->input.mem.position);
      memcpy(buff, stream->input.mem.content + stream->input.mem.position, read_sz);
      stream->input.mem.position += read_sz;
    }
  }else{
    return stream->input.reader.read(stream->input.reader.handle, buff, buff_len);
  }
  return read_sz;
}

static int32_t pixel_putc(void *fp, uint8_t c){
  imgz_decode_stream_t *out = (imgz_decode_stream_t*)fp;
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

static void _init_decoder(imgz_decode_stream_t *stream, imgz_decoder_cb_t *cb){
  if(!stream) return;
  stream->cb.on_header = cb->on_header;
  stream->cb.on_palette = cb->on_palette;
  stream->cb.on_pixel = cb->on_pixel;
  stream->cb.on_done = cb->on_done;
  stream->cb.ud = cb->ud;
  stream->parse_state = stream->wrote = stream->pos_pixel_begin = stream->pos_pixel_end = 0;
  stream->width = stream->height = stream->mode = stream->palette_count = 0;

  lzss_io_t iostream = {
    .read = pixel_read,
    .write = pixel_write,
    .in = stream,
    .out = stream
  };
  lzss_init_custom(&stream->lz, &iostream);
}

/*
  PUBLIC API
*/

void *imgz_decoder_malloc(void *(*func_malloc)(imgz_size_t)) {
  if(!func_malloc) return NULL;
  return func_malloc(sizeof(imgz_decode_stream_t));
}

void imgz_decoder_free(void *decoder, void (*func_free)(void *)){
  if(decoder && func_free) func_free(decoder);
}

int32_t imgz_decode_memory(void *decoder, const uint8_t *encode_data, const int32_t data_sz, imgz_decoder_cb_t *cb) {
  if(!decoder || !encode_data || data_sz <= 0 || !cb) return -1;
  imgz_decode_stream_t *stream = (imgz_decode_stream_t *)decoder;
  stream->stream_type = IMGZ_MEMORY;
  stream->input.mem.content = encode_data;
  stream->input.mem.content_sz = data_sz;
  stream->input.mem.position = 0;
  _init_decoder(stream, cb);
  return lzss_decode(&stream->lz);
}

int32_t imgz_decode_by_reader(void *decoder, int32_t (*read)(void *in, void *buff, int32_t buff_len), void *reader_handle, imgz_decoder_cb_t *cb){
  if(!decoder || !read || !reader_handle || !cb) return -1;
  imgz_decode_stream_t *stream = (imgz_decode_stream_t *)decoder;
  stream->stream_type = IMGZ_READER;
  stream->input.reader.read = read;
  stream->input.reader.handle = reader_handle;
  _init_decoder(stream, cb);
  return lzss_decode(&stream->lz);
}
