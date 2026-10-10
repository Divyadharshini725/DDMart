#ifndef SODIUM_H
#define SODIUM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define crypto_pwhash_STRBYTES 128
#define crypto_pwhash_OPSLIMIT_INTERACTIVE 2
#define crypto_pwhash_MEMLIMIT_INTERACTIVE 67108864

static inline int sodium_init(void) {
    return 0;
}

int crypto_pwhash_str(char out[crypto_pwhash_STRBYTES],
                      const char *passwd,
                      unsigned long long passwdlen,
                      unsigned long long opslimit,
                      size_t memlimit);

int crypto_pwhash_str_verify(const char str[crypto_pwhash_STRBYTES],
                             const char *passwd,
                             unsigned long long passwdlen);

#ifdef __cplusplus
}
#endif

#endif // SODIUM_H
