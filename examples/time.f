import' std:lang.f
import' std:time.f

: main
  Timespec alloca { inst }

  CLOCK_MONOTONIC inst clock_gettime try_errno" unable to get time"

  log" mono seconds: " inst Timespec_sec  @ log_int lf
  log" mono nanos:   " inst Timespec_nsec @ log_int lf

  CLOCK_REALTIME inst clock_gettime try_errno" unable to get time"

  log" real seconds: " inst Timespec_sec  @ log_int lf
  log" real nanos:   " inst Timespec_nsec @ log_int lf

  log" CPU  elapsed: " clock log_int lf
  log" Unix seconds: " nil time log_int lf
;
main
