#ifndef _HASH_H_
#define _HASH_H_

#include <limits.h>
#include <stdint.h>
#include <string>

using hash_t = __uint128_t;

// convert hast_t to std::string
std::string to_string(hash_t hash_val);

// get hashcode of a given string.
hash_t MD5encoding(const char *s);

std::string MD5decoding(hash_t hash_val);

#endif
