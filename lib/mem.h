#pragma once

#define is_aligned(ptr) __builtin_is_aligned(ptr, alignof(typeof(*ptr)))
