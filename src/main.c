#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "cache_wrapper.h"
#include "cache.h"

#define ALPHABETSIZE       (26 + 26 + 10 + 2)
static inline char whichchar(unsigned i) {
  if (i < 26)
    return 'A' + i;
  else if (i < (26 + 26))
    return 'a' + (i - 26);
  else if (i < (26 + 26 + 10))
    return '0' + (i - (26 + 26));
  else if (i < (ALPHABETSIZE))
    return ':' + (i - (26 + 26 + 10));
  else
    return '\0';
}

typedef struct __attribute__((packed)) {
  int len;
  char keybuf[60];
} cachekey_t;

static void make_key(int intkey, cachekey_t *key) {
  key->len = NCHARS;

  int j;
  for(j = 0; j < NCHARS; j++) {
    key->keybuf[j] = whichchar(intkey % ALPHABETSIZE);
    intkey /= ALPHABETSIZE;
  }
  assert(intkey == 0);
  key->keybuf[j] = '\0';
}

static int encrypt_key(mbedtls_aes_context *ctx, cachekey_t *key, char *buf) {
  int enclen = 0, remaining;
  int ret;

  remaining = key->len + sizeof(int);
  while(remaining > 0) {
    ret = mbedtls_aes_crypt_ecb(ctx, MBEDTLS_AES_ENCRYPT,
                              ((char *)key) + enclen, buf + enclen);
    if(ret)
      return -1;

    remaining -= 16;
    enclen += 16;
  }
  return enclen;
}

int main(int argc, char **argv) {
  cachekey_t key;
  char enckeybuf[256];
  char encvalbuf[256];
  mbedtls_aes_context *ctx;

  if(argc != 3) {
    printf("Usage: %s <npasses> <nitems>\n", argv[0]);
    printf("npasses: Number of passes of get calls\n");
    printf("nitems: Number of items to access\n");
    exit(1);
  }
  unsigned npasses = (unsigned)strtoul(argv[1], NULL, 10);
  unsigned nitems = (unsigned)strtoul(argv[2], NULL, 10);

  ctx = wrapper_init();

  /* Set value into cache */
  for(int i = 0; i < nitems; i++) {
    make_key(i, &key);

    int r = cache_set(key.keybuf, NCHARS, "hell1hell2hell3hell4hell5", 25);
    assert(r >=0);
  }

  /* Retrieve same values into cache */
  for(int j = 0; j < npasses; j++) {
    asm volatile("verr (%rsp)");      /* Rep start marker */
    for(int i = 0; i < nitems; i++) {
      memset(&key, 0, sizeof(key));
      make_key(i, &key);
      int enclen = encrypt_key(ctx, &key, enckeybuf);
      cache_get_wrapper(enckeybuf, enclen, encvalbuf, 256);
    }
    asm volatile("verw (%rsp)");      /* Rep end marker */
  }
  // dump_cache();

  wrapper_free();

  return 0;
}
