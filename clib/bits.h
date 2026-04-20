#pragma once
#include "./num.h"
#include <limits.h>

typedef Uint Bits;

static constexpr Bits BITS_ALL  = (Bits)-1;
static constexpr U8   BITS_CEIL = sizeof(Bits) * CHAR_BIT;
