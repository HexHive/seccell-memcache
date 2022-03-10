#ifndef TLBCACHE_H
#define TLBCACHE_H

#include <inttypes.h>
#include <stdbool.h>

typedef struct {
  uint64_t addr;
  uint64_t size;
} access_t;

enum access_type {
  INST_ACCESS,
  DATA_ACCESS
};

typedef struct {
  uint64_t setmask;
  unsigned sets, assoc, tagshift;
  uint64_t *array;
} cache_t;

extern bool instrumentation_enable;
void dump_cache(cache_t cache);
void init_tlbcache(void);
void account_inst(void);
void account_data(void);
void account_final(void);
int cycle_access(access_t acc, uint16_t asid, enum access_type type);

#endif /* TLBCACHE_H */