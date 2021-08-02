#include <stdio.h>
#include <stdlib.h>
#include "lzss.h"


enum{
  LZDE_ERROR = -1,
  LZDE_READY = 0,
  LZDE_REASON0,
  LZDE_REASON1
};

static int32_t file_getc(void *fp){
  return fgetc((FILE *)fp);
}

static int32_t file_putc(void *fp, uint8_t c){
  return fputc(c, (FILE *)fp);
}


static void putbit(lzss_t *lz, int32_t not_zero){
  if(not_zero)
    lz->bit_buffer |= lz->bit_mask;
  if((lz->bit_mask >>= 1) == 0){
    lz->iostream.putc(lz->iostream.out, lz->bit_buffer);
    lz->bit_buffer = 0;
    lz->bit_mask = 128;
    lz->code_count++;
  }
}

static void flush_bit_buffer(lzss_t *lz) {
  if(lz->bit_mask != 128) {
    lz->iostream.putc(lz->iostream.out, lz->bit_buffer);
    lz->code_count++;
  }
}

static void output1(lzss_t *lz, int32_t x) {
  int32_t mask = 256;
  putbit(lz, 1);
  while(mask >>= 1) {
    putbit(lz, x & mask);
  }
}

static void output2(lzss_t *lz, int32_t x, int32_t y) {
  int32_t mask = BUFF_SIZE;
  putbit(lz, 0);
  while(mask >>= 1) {
    putbit(lz, x & mask);
  }
  mask = (1 << LENGTH_BITS);
  while(mask >>= 1) {
    putbit(lz, y & mask);
  }
}

static int32_t getbit(lzss_t *lz, int32_t n){
  int32_t i = 0, x = 0;
  for(;i < n; i++){
    if(lz->curr_mask == 0){
      if((lz->curr_buf = lz->iostream.getc(lz->iostream.in)) == EOF) return EOF;
      lz->curr_mask = 128;
    }
    x <<= 1;
    if(lz->curr_buf & lz->curr_mask) x++;
    lz->curr_mask >>= 1;
  }
  return x;
}

/*
  PUBLIC API IMPL
*/

int32_t lzss_init_std(lzss_t *lz, FILE *in, FILE *out){
  if(!lz || !in || !out) return -1;
  lz->bit_buffer = 0;
  lz->bit_mask = 128;
  lz->curr_buf = 0;
  lz->curr_mask = 0;
  lz->code_count = 0;
  lz->text_count = 0;
  lz->iostream.putc = file_putc;
  lz->iostream.getc = file_getc;
  lz->iostream.in = in;
  lz->iostream.out = out;
  return 0;
}

int32_t lzss_init_custom(lzss_t* lz, const lzss_io_t *iostream){
  if(!lz || !iostream) return -1;
  if(!iostream->in || !iostream->out || !iostream->getc || !iostream->putc) return -1;
  lz->bit_buffer = 0;
  lz->bit_mask = 128;
  lz->curr_buf = 0;
  lz->curr_mask = 0;
  lz->code_count = 0;
  lz->text_count = 0;
  lz->iostream.putc = iostream->putc;
  lz->iostream.getc = iostream->getc;
  lz->iostream.in = iostream->in;
  lz->iostream.out = iostream->out;
  return 0;
}

int32_t lzss_encode(lzss_t *lz) {
  int32_t i, j, f1, x, y, r, s, bufferend, c;

  if(!lz) return -1;

  for (i = 0; i < BUFF_SIZE - MAX_CODED; i++)
    lz->buffer[i] = ' ';
  for (i = BUFF_SIZE - MAX_CODED; i < BUFF_SIZE * 2; i++) {
    if((c = lz->iostream.getc(lz->iostream.in)) == EOF) break;
    lz->buffer[i] = c;
    lz->text_count++;
  }
  bufferend = i;
  r = BUFF_SIZE - MAX_CODED;
  s = 0;
  while (r < bufferend) {
    f1 = (MAX_CODED <= bufferend - r) ? MAX_CODED : bufferend - r;
    x = 0;
    y = 1;
    c = lz->buffer[r];
    for (i = r - 1; i >= s; i--){
      if (lz->buffer[i] == c) {
        for (j = 1; j < f1; j++)
          if (lz->buffer[i + j] != lz->buffer[r + j]) break;
        if (j > y) {
          x = i;
          y = j;
        }
      }
    }
    if (y <= UNCODED) {
      y = 1;
      output1(lz, c);
    }else{
      output2(lz, x & (BUFF_SIZE - 1), y - 2);
    }
    r += y;
    s += y;
    if (r >= BUFF_SIZE * 2 - MAX_CODED) {
      for (i = 0; i < BUFF_SIZE; i++)
        lz->buffer[i] = lz->buffer[i + BUFF_SIZE];
      bufferend -= BUFF_SIZE;
      r -= BUFF_SIZE;
      s -= BUFF_SIZE;
      while (bufferend < BUFF_SIZE * 2) {
        if((c = lz->iostream.getc(lz->iostream.in)) == EOF) break;
        lz->buffer[bufferend++] = c;
        lz->text_count++;
      }
    }
  }
  flush_bit_buffer(lz);
  fprintf(stderr, "text:  %u bytes\n", lz->text_count);
  fprintf(stderr, "code:  %u bytes (%u%%)\n", lz->code_count, (lz->code_count * 100) / lz->text_count);
  return 0;
}

int32_t lzss_decode(lzss_t *lz){
  int32_t i, j, k, r, c;

  if(!lz) return -1;

  for(i = 0; i < BUFF_SIZE - MAX_CODED; i++)
    lz->buffer[i] = ' ';
  r = BUFF_SIZE - MAX_CODED;
  while((c = getbit(lz, 1)) != EOF){
    if(c){
      if((c = getbit(lz, 8)) == EOF) break;
      lz->iostream.putc(lz->iostream.out, c);
      lz->buffer[r++] = c;
      r &= (BUFF_SIZE - 1);
    }else{
      if((i = getbit(lz, OFFSET_BITS)) == EOF) break;
      if((j = getbit(lz, LENGTH_BITS)) == EOF) break;
      for(k = 0; k <= j+1; k++){
        c = lz->buffer[(i + k) & (BUFF_SIZE - 1)];
        lz->iostream.putc(lz->iostream.out, c);
        lz->buffer[r++] = c;
        r &= (BUFF_SIZE - 1);
      }
    }
  }
  return 0;
}

int32_t lzss_chunked_decode_init_std(lzss_chunked_decoder_t *lcdz, FILE *in){
  if(!lcdz || !in) return -1;
  lzss_t *lz = &lcdz->lz;
  lz->bit_buffer = 0;
  lz->bit_mask = 128;
  lz->curr_buf = 0;
  lz->curr_mask = 0;
  lz->code_count = 0;
  lz->text_count = 0;
  lz->iostream.getc = file_getc;
  lz->iostream.putc = NULL;
  lz->iostream.in = in;
  lz->iostream.out = NULL;

  lcdz->state = LZDE_READY;
  lcdz->curr_r = BUFF_SIZE - MAX_CODED;
  for(int32_t i = 0; i < BUFF_SIZE - MAX_CODED; i++)
    lcdz->lz.buffer[i] = ' ';
  return 0;
}

int32_t lzss_chunked_decode_init_custom(lzss_chunked_decoder_t *lcdz, const lzss_io_t *iostream) {
  if(!lcdz || !iostream) return -1;
  if(!iostream->in || !iostream->getc) return -1;
  lzss_t *lz = &lcdz->lz;
  lz->bit_buffer = 0;
  lz->bit_mask = 128;
  lz->curr_buf = 0;
  lz->curr_mask = 0;
  lz->code_count = 0;
  lz->text_count = 0;
  lz->iostream.getc = iostream->getc;
  lz->iostream.putc = NULL;
  lz->iostream.in = iostream->in;
  lz->iostream.out = NULL;

  lcdz->state = LZDE_READY;
  lcdz->curr_r = BUFF_SIZE - MAX_CODED;
  for(int32_t i = 0; i < BUFF_SIZE - MAX_CODED; i++)
    lcdz->lz.buffer[i] = ' ';
  return 0;
}

/*
  return 0 success-done, 1 success-continue-0, -1 error
*/
int32_t lzss_chunked_decode(lzss_chunked_decoder_t *lcdz, uint8_t *out_buffer, int32_t *buff_size){
  int32_t buff_used = 0;
  if(!lcdz || !out_buffer || !buff_size) return -1;
  if(lcdz->state < LZDE_READY || *buff_size <= 0) return -1;
  lzss_t *lz = &lcdz->lz;
  switch(lcdz->state){
    case LZDE_REASON0:
      goto continue_reason0;
    case LZDE_REASON1:
      goto continue_reason1;
    default:
      break;
  }

  //working
  while((lcdz->curr_c = getbit(lz, 1)) != EOF){
    if(lcdz->curr_c){
      if((lcdz->curr_c = getbit(lz, 8)) == EOF) break;
      out_buffer[buff_used++] = lcdz->curr_c;
      lz->buffer[lcdz->curr_r++] = lcdz->curr_c;
      lcdz->curr_r &= (BUFF_SIZE - 1);
      if(buff_used == *buff_size){
        lcdz->state = LZDE_REASON0;
        goto exit_continue;
      }
continue_reason0:
      asm("nop");
    }else{
      if((lcdz->curr_offset = getbit(lz, OFFSET_BITS)) == EOF) break;
      if((lcdz->curr_length = getbit(lz, LENGTH_BITS)) == EOF) break;
      for(lcdz->curr_k = 0; lcdz->curr_k <= lcdz->curr_length+1; lcdz->curr_k++){
        lcdz->curr_c = lz->buffer[(lcdz->curr_offset + lcdz->curr_k) & (BUFF_SIZE - 1)];
        out_buffer[buff_used++] = lcdz->curr_c;
        lz->buffer[lcdz->curr_r++] = lcdz->curr_c;
        lcdz->curr_r &= (BUFF_SIZE - 1);
        if(buff_used == *buff_size){
          lcdz->state = LZDE_REASON1;
          goto exit_continue;
        }
continue_reason1:
        asm("nop");
      }
    }
  }
  *buff_size = buff_used;
  return 0;
exit_continue:
  return 1;
}

