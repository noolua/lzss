#include <stdio.h>
#include "lzss.h"

int main(int argc, char *argv[]) {
  int enc, ret;
  char *s;
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
  if(enc){
    lzss_t lz;
    lzss_init_std(&lz, infile, outfile);
    ret = lzss_encode(&lz);
  }else{
    #if 0

    lzss_t lz;
    lzss_init_std(&lz, infile, outfile);
    ret = lzss_decode(&lz);

    #else

    lzss_chunked_decoder_t lcdz;
    fprintf(stderr, "sizeof(lzss_chunked_decoder_t): %zu\n", sizeof(lcdz));
    uint8_t buffer[7];
    int32_t buff_size = 7;
    lzss_chunked_decode_init_std(&lcdz, infile);
    while((ret = lzss_chunked_decode(&lcdz, buffer, &buff_size)) >= 0){
      if(buff_size > 0)
        fwrite(buffer, 1, buff_size, outfile);
      buff_size = 7;
      if(ret == 0)
        break;
    }

    #endif
  }

  // cleanup
  fclose(infile);
  fclose(outfile);
  return ret;
}
