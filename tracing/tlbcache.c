#include "config.h"
#include "tlbcache.h"
#include "util.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Types */
typedef struct {
  uint16_t asid;
  uint64_t tag;
} tlbe_t;

/* Stats */
uint64_t l1i_tlb_misses = 0;
uint64_t l1d_tlb_misses = 0;
uint64_t l2_stlb_misses = 0;
uint64_t l1i_misses = 0;
uint64_t l1d_misses = 0;
uint64_t llc_misses = 0;
uint64_t user_insts = 0, kern_insts = 0;

/* Storage */
static cache_t l1itlb, l1dtlb, l2stlb;
static uint64_t array_l1itlb[L1_I_TLB_SIZE];
static uint64_t array_l1dtlb[L1_D_TLB_SIZE];
static uint64_t array_l2stlb[L2_TLB_SIZE];

static cache_t l1i, l1d, llc;
static uint64_t array_l1i[L1_I_SIZE >> CACHE_LINE_BITS];
static uint64_t array_l1d[L1_D_SIZE >> CACHE_LINE_BITS];
static uint64_t array_llc[LLC_SIZE >> CACHE_LINE_BITS];

static unsigned long inst_counter = 0;
static unsigned long data_counter = 0;
bool instrumentation_enable;

void account_inst(void) {
  char out[4096];

  if(!instrumentation_enable)
    return;

  // if((inst_counter++ & 0xffffff) == 0) {
  //   printf("insn count %ld\n", inst_counter);

  //   printf("%15"PRIu64" %15"PRIu64" %15"PRIu64" %15"
  //           PRIu64" %15"PRIu64" %15"PRIu64" \n",
  //           l1i_tlb_misses,
  //           l1d_tlb_misses,
  //           l2_stlb_misses,
  //           l1i_misses,
  //           l1d_misses,
  //           llc_misses);
  // }
}

void account_data(void) {
  if(!instrumentation_enable)
    return;

  data_counter++;
}

void account_final(void) {
  // printf("insn count %ld\n", inst_counter);
  printf("%15"PRIu64" %15"PRIu64" %15"PRIu64" %15"
        PRIu64" %15"PRIu64" %15"PRIu64" \n",
        l1i_tlb_misses,
        l1d_tlb_misses,
        l2_stlb_misses,
        l1i_misses,
        l1d_misses,
        llc_misses);
  // printf("kern = %lu user = %lu\n", kern_insts, user_insts);

  // uint64_t mat =   (data_counter * (L1_D_TLB_LAT + L1_D_LAT))
  //             + (l1d_tlb_misses * L2_TLB_LAT)
  //             + (l1d_misses     * LLC_LAT)
  //             + (l2_stlb_misses * WALK_LAT)
  //             + (llc_misses     * MEM_LAT);
  // uint64_t cpr = ((inst_counter - data_counter) + mat);
  // printf("mat = %"PRIu64"\ndata_count = %"PRIu64"\ninst_count = %"PRIu64"\n", mat, data_counter, inst_counter);
}

void init_tlbcache(void) {
  /* Initializing TLB */
  l1itlb.sets = L1_I_TLB_SETS;
  l1itlb.assoc = L1_I_TLB_ASSOC;
  l1itlb.setmask = L1_I_TLB_SETS - 1;
  l1itlb.tagshift = vallog2(L1_I_TLB_SETS);
  l1itlb.array = array_l1itlb;

  l1dtlb.sets = L1_D_TLB_SETS;
  l1dtlb.assoc = L1_D_TLB_ASSOC;
  l1dtlb.setmask = L1_D_TLB_SETS - 1;
  l1dtlb.tagshift = vallog2(L1_D_TLB_SETS);
  l1dtlb.array = array_l1dtlb;

  l2stlb.sets = L2_TLB_SETS;
  l2stlb.assoc = L2_TLB_ASSOC;
  l2stlb.setmask = L2_TLB_SETS - 1;
  l2stlb.tagshift = vallog2(L2_TLB_SETS);
  l2stlb.array = array_l2stlb;

  /* Initialize caches with 0, assuming the 
   * null address is actually invalid */
  memset(array_l1itlb, 0, sizeof(array_l1itlb));
  memset(array_l1dtlb, 0, sizeof(array_l1dtlb));
  memset(array_l2stlb, 0, sizeof(array_l2stlb));

  /* Initilizing caches */
  l1i.sets = L1_I_SETS;
  l1i.assoc = L1_I_ASSOC;
  l1i.setmask = L1_I_SETS - 1;
  l1i.tagshift = vallog2(L1_I_SETS);
  l1i.array = array_l1i;

  l1d.sets = L1_D_SETS;
  l1d.assoc = L1_D_ASSOC;
  l1d.setmask = L1_D_SETS - 1;
  l1d.tagshift = vallog2(L1_D_SETS);
  l1d.array = array_l1d;

  llc.sets = LLC_SETS;
  llc.assoc = LLC_ASSOC;
  llc.setmask = LLC_SETS - 1;
  llc.tagshift = vallog2(LLC_SETS);
  llc.array = array_llc;

  memset(array_l1i, 0, sizeof(array_l1i));
  memset(array_l1d, 0, sizeof(array_l1d));
  memset(array_llc, 0, sizeof(array_llc)); 

  user_insts = 0;
  kern_insts = 0;

  instrumentation_enable = false;
}

void __attribute__((unused))
dump_cache(cache_t cache) {
  unsigned i, j;
  unsigned zero_set_count = 0;
  char out[1024];
  
  printf("Dumping cache\n");
  for(i = 0; i < cache.sets; i++) {
    uint64_t *cache_set = cache.array + (cache.assoc * i);

    /* Checking for zero-set */
    int zerocount = 0;
    for(j = 0; j < cache.assoc; j++)
      if(cache_set[j] == 0)
        zerocount++;

    if(zerocount == cache.assoc)
      zero_set_count++;
    else {
      if(zero_set_count > 0) {
        printf("Zero-set * %d\n", zero_set_count);
      }
      zero_set_count = 0;
      
      printf("%"PRIx64, cache_set[0]);
      for(j = 1; j < cache.assoc; j++) {
        printf(", %"PRIx64, cache_set[j]);
      }
      printf("\n");
    }
  }
  if(zero_set_count > 0) {
    printf("Zero-set * %d\n", zero_set_count);
  }
}

static uint64_t evict(cache_t cache, uint64_t addr) {
  /* In this config, there should not be any evictions */
  assert(0);

  uint64_t set = addr & cache.setmask;
  uint64_t tag = addr >> cache.tagshift;
  uint64_t *cache_set = cache.array + (cache.assoc * set);
  uint64_t idx;

  /* The loop will break on the index 
   * which matches the tag */
  for(idx = 0; idx < cache.assoc; idx++) {
    if(cache_set[idx] == tag)
      break;
  }

  /* Keep copying the rest of the 
   * set down to the last. 
   * When nothing has matched, the idx is 
   * already cache.assoc, so will skip this loop */
  for(; idx < cache.assoc - 1; idx++) {
    cache_set[idx] = cache_set[idx + 1];
  }

  /* Clear last element */
  if(idx == cache.assoc) 
    cache_set[cache.assoc - 1] = 0;
}

/* Touches addr in cache, 
 * then performs LRU update, and
 * returns:
 * - 0    if hit
 * - 1    if miss (hey, what are the chances miss addr is 1?)
 * - addr if miss and evicted
 */
static uint64_t access_cache(cache_t cache, uint64_t addr) {
  uint64_t set = addr & cache.setmask;
  uint64_t tag = addr >> cache.tagshift;
  uint64_t idx, bounded_idx;
  uint64_t evict_addr;

  uint64_t *cache_set = cache.array + (cache.assoc * set);
  evict_addr = cache_set[cache.assoc - 1];
  for(idx = 0; idx < cache.assoc; idx++) {
    if(cache_set[idx] == tag) 
      break;
  }

  /* Perform LRU boosting
   * Bounding idx takes care of miss case */
  bounded_idx = (idx == cache.assoc)? cache.assoc - 1: idx;
  for(unsigned j = 0; j < bounded_idx; j++) {
    cache_set[bounded_idx - j] = cache_set[bounded_idx - j - 1];
  }
  cache_set[0] = tag;

  if(idx == cache.assoc) /* Miss */ {
    return (evict_addr)? evict_addr: 1;
  } else
    return 0;
}

static void 
cycle_helper(access_t acc, cache_t l1cache, uint64_t *l1stat, 
            cache_t l2cache, uint64_t *l2stat,
            int incl) {
  /* Might need to repeat if access spans cache line */
  for(unsigned i = 0; i < acc.size; i++) {
    if(access_cache(l1cache, acc.addr + i)) {
      *l1stat += 1;

      uint64_t evict_addr = access_cache(l2cache, acc.addr + i);
      if(evict_addr)
        *l2stat += 1;

      if(incl && (evict_addr != 1)) 
        evict(l1cache, evict_addr);
    }
  }
}


int cycle_access(access_t access, uint16_t asid, enum access_type type) {
  access_t _acc;
  cache_t l1tlb, l1cache;
  uint64_t *l1tlb_stat, *l1cache_stat;

  if(!instrumentation_enable)
    return 0;

  if(type == DATA_ACCESS) {
    l1tlb = l1dtlb;
    l1tlb_stat = &l1d_tlb_misses;
    l1cache = l1d;
    l1cache_stat = &l1d_misses;
  } else {
    l1tlb = l1itlb;
    l1tlb_stat = &l1i_tlb_misses;
    l1cache = l1i;
    l1cache_stat = &l1i_misses;
  }

  if(type == INST_ACCESS) {
    user_insts += ((access.addr >> 48) == 0);
    kern_insts += ((access.addr >> 48) == 0xffff);
  }

  /* Access TLB, storing asid in top ASID_LEN bits by calling mixpageasid */
  _acc.size = ((access.addr + access.size - 1) >> ARCH_PAGE_BITS) - (access.addr >> ARCH_PAGE_BITS) + 1;
  _acc.addr = mixpageasid((int64_t)access.addr >> ARCH_PAGE_BITS, asid);
  cycle_helper(_acc, l1tlb, l1tlb_stat, l2stlb, &l2_stlb_misses, 0);

  /* Access caches after removing pesky sub-granular indices */
  _acc.size = ((access.addr + access.size - 1) >> CACHE_LINE_BITS) - (access.addr >> CACHE_LINE_BITS) + 1;
  _acc.addr = access.addr >> CACHE_LINE_BITS;
  cycle_helper(_acc, l1cache, l1cache_stat, llc, &llc_misses, CACHE_INCLUSIVE);

  return 0;
}
 
