#include "./num.h"
#include <stdio.h>
#include <time.h>

typedef struct {
  struct timespec beg;
  struct timespec end;
  char           *prefix;
  F64             diff;
  FILE           *file;
} Timing;

static void timing_beg(Timing *time) {
  clock_gettime(CLOCK_MONOTONIC, &time->beg);
}

static void timing_end(Timing *time) {
  clock_gettime(CLOCK_MONOTONIC, &time->end);

  const auto secs  = (F64)(time->end.tv_sec - time->beg.tv_sec);
  const auto nanos = (F64)(time->end.tv_nsec - time->beg.tv_nsec);
  time->diff       = secs + nanos / 1'000'000'000;

  auto file = time->file;
  if (!file) file = stderr;

  auto pre = time->prefix;
  if (!pre) pre = "[timing]";

  fprintf(file, "%selapsed: %f seconds\n", pre, time->diff);
}
