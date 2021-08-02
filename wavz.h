#ifndef __wavz_h__
#define __wavz_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "lzss.h"

typedef struct wavz_progm_s{
  const uint32_t _size;
  const uint8_t _content[0];
}wavz_progm_t;

#define wavz4progm(d)   (const wavz_progm_t*)(d)

#ifdef __cplusplus
}
#endif

#endif //__wavz_h__
