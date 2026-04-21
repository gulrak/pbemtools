#pragma once

#include <cstdint>

typedef uint32_t ub4; /* unsigned 4-byte quantities */
typedef uint8_t ub1;
ub4 Hash(const ub1* k, ub4 length, ub4 initval);

