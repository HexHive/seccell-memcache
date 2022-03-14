#ifndef TLBCACHE_CONFIG_H
#define TLBCACHE_CONFIG_H

#define CACHE_LINE_BITS 6
#define ARCH_PAGE_BITS       12

/* Cache may be inclusive, 
 * TLB is non-inclusive. */
#define CACHE_INCLUSIVE 0

/* x86 PCID has 12 bits, Risc-v ASID has 16 */
#define ASID_LEN        12

/* 2-level TLB Characteristics */
#define L1_I_TLB_SIZE  64
#define L1_I_TLB_ASSOC 64
#define L1_I_TLB_LAT   1
#define L1_I_TLB_SETS  (L1_I_TLB_SIZE/L1_I_TLB_ASSOC)
#define L1_D_TLB_SIZE  64
#define L1_D_TLB_ASSOC 64
#define L1_D_TLB_LAT   1
#define L1_D_TLB_SETS  (L1_D_TLB_SIZE/L1_D_TLB_ASSOC)
#define L2_TLB_SIZE     1024
#define L2_TLB_ASSOC    4
#define L2_TLB_LAT      3
#define L2_TLB_SETS  (L2_TLB_SIZE/L2_TLB_ASSOC)

/* 2-level cache Characteristics */
#define L1_D_SIZE       (32*1024)
#define L1_D_ASSOC      8
#define L1_D_LAT        4
#define L1_D_SETS       ((L1_D_SIZE >> CACHE_LINE_BITS)/L1_D_ASSOC)
#define L1_I_SIZE       (32*1024)
#define L1_I_ASSOC      8
#define L1_I_LAT        4
#define L1_I_SETS       ((L1_I_SIZE >> CACHE_LINE_BITS)/L1_I_ASSOC)
#define LLC_SIZE        (16*1024*1024)
#define LLC_ASSOC       16
#define LLC_LAT         30
#define LLC_SETS        ((LLC_SIZE >> CACHE_LINE_BITS)/LLC_ASSOC)

#define L1_MSHRS        16

/* DRAM Characteristics */
#define MEM_LAT         100
#define WALK_LAT        (12*MEM_LAT/10)

#endif /* TLBCACHE_CONFIG_H */