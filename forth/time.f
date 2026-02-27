import' ./lang.f

\ ## Libc time
\
\ MacOS 15.3 (24D60), <time.h>.
\
\ Translated from C definitions and only partially tested.

S64 let: Clock \ clock_t
S64 let: Time  \ time_t

\ `struct timespec`. Used by reference.
struct: Timespec
  S64 1 field: Timespec_sec
  S64 1 field: Timespec_nsec
end

\ `struct tm`. Used by reference.
struct: Date
  S32  1 field: Time_sec    \ seconds after the minute [0-60]
  S32  1 field: Time_min    \ minutes after the hour [0-59]
  S32  1 field: Time_hour   \ hours since midnight [0-23]
  S32  1 field: Time_mday   \ day of the month [1-31]
  S32  1 field: Time_mon    \ months since January [0-11]
  S32  1 field: Time_year   \ years since 1900
  S32  1 field: Time_wday   \ days since Sunday [0-6]
  S32  1 field: Time_yday   \ days since January 1 [0-365]
  S32  1 field: Time_isdst  \ Daylight Savings Time flag
  S64  1 field: Time_gmtoff \ offset from UTC in seconds
  Cstr 1 field: Time_zone   \ timezone abbreviation
end

1000000 let: CLOCKS_PER_SEC

0 let: CLOCK_REALTIME
6 let: CLOCK_MONOTONIC

extern_val: getdate_err getdate_err

0 1 extern: clock       (          -- clock )
1 1 extern: time        ( time_adr -- time  )
1 1 extern: getdate     ( str      -- date  )

4 1 extern: strftime    ( buf size fmt date -- len )
4 1 extern: strptime    ( buf fmt date      -- str )

1 1 extern: asctime     ( date          -- str  )
2 1 extern: asctime_r   ( date buf      -- str  )
1 1 extern: ctime       ( time_adr      -- str  )
2 1 extern: ctime_r     ( time_adr buf  -- str  )
1 1 extern: gmtime      ( time_adr      -- date )
2 1 extern: gmtime_r    ( time_adr date -- date )
1 1 extern: localtime   ( time_adr      -- date )
2 1 extern: localtime_r ( time_adr date -- date )
1 1 extern: mktime      ( date          -- time )
1 1 extern: timegm      ( date          -- time )
1 1 extern: timelocal   ( date          -- time )

1 1 extern: posix2time  ( time -- time )
1 1 extern: time2posix  ( time -- time )

0 0 extern: tzset       ( -- )
0 0 extern: tzsetwall   ( -- )

2 1 extern: timespec_get ( timespec base -- base )

2 1 extern: clock_gettime         ( clock_id timespec -- err )
2 1 extern: clock_settime         ( clock_id timespec -- err )
2 1 extern: clock_getres          ( clock_id timespec -- err )
1 1 extern: clock_gettime_nsec_np ( clock_id          -- val )

1 1 extern: sleep     ( secs              -- left )
1 1 extern: usleep    ( usec              -- err  )
2 1 extern: nanosleep ( timespec timespec -- err  )
