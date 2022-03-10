#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

static inline 
uint8_t vallog2(uint64_t val) {
  uint8_t logval = 0;

  while(!(val & 1)) {
    val >>= 1;
    logval++;
  }
  return logval;
}

static inline
uint64_t mixpageasid(uint64_t pageno, uint16_t asid) {
  uint64_t mash;

  uint64_t top16bits = (pageno & (0xfffful << 48)) >> 48;
  if(!((top16bits == 0) || (top16bits == 0xfffful))) {
    printf("address: %lx\n", pageno);
  }
  assert((top16bits == 0) || (top16bits == 0xfffful));
  
  mash = ((uint64_t)asid << 48) | pageno;
  return mash;
}

#endif /* UTIL_H */