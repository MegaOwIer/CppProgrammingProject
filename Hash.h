#ifndef _HASH_H_
#define _HASH_H_

#include <stdint.h>

using hash_t = __uint128_t;

// get hashcode of a given string.
hash_t MD5encoding(const char *s);

#endif