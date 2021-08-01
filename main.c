/* LZSS encoder-decoder (Haruhiko Okumura; public domain) */

#include <stdio.h>
#include "lzss.h"

#if 0
// pixel_image iostream
typedef struct pixel_in_s {
  int32_t size, pos;
  uint8_t *base;
}pixel_in_t;

typedef struct pixel_out_s {
  int32_t dummy;
}pixel_out_t;

static int32_t pixel_getc(void *fp){
  pixel_in_t *in = (pixel_in_t*)fp;
  if(in->pos < in->size){
    return in->base[in->pos++];
  }
  return EOF;
}

static int32_t pixel_putc(void *fp, uint8_t c){
  return 0;
}

unsigned char iii_z6[] = {
  0xa0, 0x44, 0x20, 0x90, 0x58, 0x03, 0xc3, 0x06, 0xdb, 0xaa, 0x96, 0x7f,
  0xfa, 0x57, 0xd7, 0x92, 0x07, 0x6b, 0xa9, 0x5e, 0xff, 0xbd, 0x20, 0xa0,
  0x10, 0x37, 0xb0, 0x0b, 0xc3, 0x09, 0xef, 0x8e, 0xf6, 0x8d, 0x7d, 0x00,
  0x3e, 0xb3, 0x1f, 0x41, 0xb0, 0x38, 0x24, 0x12, 0x04, 0x06, 0x80, 0x01,
  0x85, 0x81, 0xb2, 0xdf, 0xb9, 0xe0, 0xa0, 0xd0, 0x90, 0x04, 0x04, 0x2c,
  0x80, 0x12, 0xcb, 0x0b, 0x46, 0x03, 0x74, 0x02, 0xd1, 0xb0, 0x20, 0xb2,
  0x70, 0x98, 0x00, 0x35, 0xb8, 0x2a, 0x4c, 0x12, 0xff, 0x40, 0x00, 0xd0,
  0x23, 0xac, 0xc2, 0x34, 0x6b, 0xf8, 0x3c, 0x94, 0x9c, 0x56, 0x3f, 0x33,
  0x3f, 0x9d, 0xaf, 0xd0, 0xdf, 0xe9, 0x73, 0xf5, 0x3b, 0xfa, 0xde, 0xfd,
  0x8f, 0xfe, 0xd8, 0x3f, 0x74, 0x39, 0x80
};
unsigned int iii_z6_len = 115;

#endif

int main(int argc, char *argv[]) {
  int enc, ret;
  char *s;
  lzss_t lz;
  FILE *infile, *outfile;
  #if 0
  pixel_in_t px_in = {
    .size = iii_z6_len,
    .pos = 0,
    .base = iii_z6
  };
  #endif

  if (argc != 4) {
    fprintf(stderr, "Usage: lzss e/d infile outfile\n\te = encode\td = decode\tinfile('-' as stdin)\toutfile('-' as stdout)\nexample: cat plain.data | lzss e - - | hexdump\n");
    return 1;
  }
  s = argv[1];
  if (s[1] == 0 && (*s == 'd' || *s == 'D' || *s == 'e' || *s == 'E'))
    enc = (*s == 'e' || *s == 'E');
  else {
    fprintf(stderr, "? %s\n", s);  return 1;
  }
  if(argv[2][0] == '-'){
    infile = stdin;
  }else{
    if ((infile  = fopen(argv[2], "rb")) == NULL) {
      fprintf(stderr, "? %s\n", argv[2]);  return 1;
    }
  }
  if(argv[3][0] == '-'){
    outfile = stdout;
  }else{
    if ((outfile = fopen(argv[3], "wb")) == NULL) {
      fprintf(stderr, "? %s\n", argv[3]);  return 1;
    }
  }

  // working
  lzss_init_std(&lz, infile, outfile);
  ret = enc ? lzss_encode(&lz) : lzss_decode(&lz);

  // cleanup
  fclose(infile);
  fclose(outfile);
  return ret;
}
