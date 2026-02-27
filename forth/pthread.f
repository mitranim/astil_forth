import' ./lang.f

\ ## Posix threads
\
\ MacOS 15.3 (24D60), <pthread.h>.
\
\ Translated from C definitions and only partially tested.
\
\ Posix words directly return an `errno` code on failure.
\ They should usually be followed by `try_errno_posix"`:
\
\   some_proc try_errno_posix" unable to do X"

\ ## Types

\ `pthread_t`. Used by value except in "create".
\ Can be used as a local addressed via `ref'`.
\ See usage in `examples/threads_*.f`.
Adr let: Pthread

\ Opaque `sizeof(pthread_attr_t)`. Used by reference.
64 let: Pthread_attr

\ Opaque `sizeof(pthread_cond_t)`. Used by reference.
48 let: Pthread_cond

\ Opaque `sizeof(pthread_condattr_t)`. Used by reference.
16 let: Pthread_cond_attr

\ Opaque `sizeof(struct sched_param)`. Used by reference.
8 let: Pthread_sched_param

\ Opaque `sizeof(pthread_once_t)`. Used by reference.
16 let: Pthread_once

\ `pthread_key_t`. Used by value except in "create".
Cell let: Pthread_key

\ Opaque `sizeof(pthread_mutexattr_t)`. Used by reference.
16 let: Pthread_mutex_attr

\ Opaque `sizeof(pthread_mutex_t)`. Used by reference.
64 let: Pthread_mutex

\ Opaque `sizeof(pthread_rwlockattr_t)`. Used by reference.
24 let: Pthread_rwlock_attr

\ Opaque `sizeof(pthread_rwlock_t)`. Used by reference.
200 let: Pthread_rwlock

\ `sigset_t`. Sometimes used by reference.
U32 let: Sigset

\ ## Constants

1 let: PTHREAD_CREATE_JOINABLE
2 let: PTHREAD_CREATE_DETACHED

1 let: PTHREAD_INHERIT_SCHED
2 let: PTHREAD_EXPLICIT_SCHED

1 let: PTHREAD_CANCEL_ENABLE       \ Cancel takes place at next cancellation point.
0 let: PTHREAD_CANCEL_DISABLE      \ Cancel postponed.
2 let: PTHREAD_CANCEL_DEFERRED     \ Cancel waits until cancellation point.
0 let: PTHREAD_CANCEL_ASYNCHRONOUS \ Cancel occurs immediately.

1 let: PTHREAD_CANCELED \ (void*)1

1 let: PTHREAD_SCOPE_SYSTEM
2 let: PTHREAD_SCOPE_PROCESS

1 let: PTHREAD_PROCESS_SHARED
2 let: PTHREAD_PROCESS_PRIVATE

0 let: PTHREAD_PRIO_NONE
1 let: PTHREAD_PRIO_INHERIT
2 let: PTHREAD_PRIO_PROTECT

0 let: PTHREAD_MUTEX_NORMAL
1 let: PTHREAD_MUTEX_ERRORCHECK
2 let: PTHREAD_MUTEX_RECURSIVE
PTHREAD_MUTEX_NORMAL let: PTHREAD_MUTEX_DEFAULT

1 let: PTHREAD_MUTEX_POLICY_FAIRSHARE_NP
3 let: PTHREAD_MUTEX_POLICY_FIRSTFIT_NP

\ ## Procedures
\
\ Integer addresses here tend to be `int*`, meaning: `int32*`.
\ When using local addresses, the locals should be pre-zeroed.
\ Then on little-endian, C can store with 32-bit instructions,
\ and we can load with 64-bit instructions, and it just works.

1 1 extern: pthread_attr_init                ( attr                  -- err )
1 1 extern: pthread_attr_destroy             ( attr                  -- err )
2 1 extern: pthread_attr_getdetachstate      ( attr detach_adr       -- err )
2 1 extern: pthread_attr_setdetachstate      ( attr detach           -- err )
2 1 extern: pthread_attr_getguardsize        ( attr size_adr         -- err )
2 1 extern: pthread_attr_setguardsize        ( attr size             -- err )
2 1 extern: pthread_attr_getinheritsched     ( attr inherit_adr      -- err )
2 1 extern: pthread_attr_setinheritsched     ( attr inherit          -- err )
2 1 extern: pthread_attr_getschedparam       ( attr param            -- err )
2 1 extern: pthread_attr_setschedparam       ( attr param            -- err )
2 1 extern: pthread_attr_getschedpolicy      ( attr policy_adr       -- err )
2 1 extern: pthread_attr_setschedpolicy      ( attr policy           -- err )
2 1 extern: pthread_attr_getscope            ( attr scope_adr        -- err )
2 1 extern: pthread_attr_setscope            ( attr scope            -- err )
3 1 extern: pthread_attr_getstack            ( attr adr_adr size_adr -- err )
3 1 extern: pthread_attr_setstack            ( attr stack_adr size   -- err )
2 1 extern: pthread_attr_getstackaddr        ( attr adr_adr          -- err )
2 1 extern: pthread_attr_setstackaddr        ( attr stack_adr        -- err )
2 1 extern: pthread_attr_getstacksize        ( attr size_adr         -- err )
2 1 extern: pthread_attr_setstacksize        ( attr size             -- err )

1 1 extern: pthread_cond_broadcast           ( cond            -- err )
1 1 extern: pthread_cond_destroy             ( cond            -- err )
2 1 extern: pthread_cond_init                ( cond attr       -- err )
1 1 extern: pthread_cond_signal              ( cond            -- err )
3 1 extern: pthread_cond_timedwait           ( cond mutex time -- err )
2 1 extern: pthread_cond_wait                ( cond mutex      -- err )

1 1 extern: pthread_condattr_init            ( attr            -- err )
1 1 extern: pthread_condattr_destroy         ( attr            -- err )
2 1 extern: pthread_condattr_getpshared      ( attr shared_adr -- err )
2 1 extern: pthread_condattr_setpshared      ( attr shared     -- err )

4 1 extern: pthread_create                   ( thread_adr attr fun inp -- err )
4 1 extern: pthread_create_suspended_np      ( thread_adr attr fun inp -- err )
1 1 extern: pthread_cancel                   ( thread                  -- err )
2 1 extern: pthread_kill                     ( thread sig              -- err )
2 1 extern: pthread_join                     ( thread out_adr          -- err )
1 1 extern: pthread_detach                   ( thread                  -- err )
2 1 extern: pthread_threadid_np              ( thread id_adr           -- err )
3 1 extern: pthread_getschedparam            ( thread policy_adr param -- err )
3 1 extern: pthread_setschedparam            ( thread policy     param -- err )

3 1 extern: pthread_atfork                   ( prep parent child    -- err  )
2 1 extern: pthread_once                     ( once_ctrl init_fun   -- err  )
3 1 extern: pthread_sigmask                  ( how set_adr oset_adr -- err  )
1 0 extern: pthread_exit                     ( out_adr              --      )
0 0 extern: pthread_yield_np                 (                      --      )
0 1 extern: pthread_is_threaded_np           (                      -- bool )
2 1 extern: pthread_equal                    ( thread0 thread1      -- bool )

0 1 extern: pthread_getconcurrency           (     -- lvl )
1 1 extern: pthread_setconcurrency           ( lvl -- err )

2 1 extern: pthread_setcancelstate           ( state state_adr -- err )
2 1 extern: pthread_setcanceltype            ( type  type_adr  -- err )
0 0 extern: pthread_testcancel               (                 --     )

2 1 extern: pthread_key_create               ( key_adr deinit_fun -- err )
1 1 extern: pthread_key_delete               ( key                -- err )

1 1 extern: pthread_getspecific              ( key     -- val )
2 1 extern: pthread_setspecific              ( key val -- err )

1 1 extern: pthread_mutexattr_init           ( attr            -- err )
1 1 extern: pthread_mutexattr_destroy        ( attr            -- err )
2 1 extern: pthread_mutexattr_getprioceiling ( attr ceil_adr   -- err )
2 1 extern: pthread_mutexattr_setprioceiling ( attr ceil       -- err )
2 1 extern: pthread_mutexattr_getprotocol    ( attr prot_adr   -- err )
2 1 extern: pthread_mutexattr_setprotocol    ( attr prot       -- err )
2 1 extern: pthread_mutexattr_getpshared     ( attr shared_adr -- err )
2 1 extern: pthread_mutexattr_setpshared     ( attr shared     -- err )
2 1 extern: pthread_mutexattr_gettype        ( attr type_adr   -- err )
2 1 extern: pthread_mutexattr_settype        ( attr type       -- err )
2 1 extern: pthread_mutexattr_getpolicy_np   ( attr pol_adr    -- err )
2 1 extern: pthread_mutexattr_setpolicy_np   ( attr pol        -- err )

2 1 extern: pthread_mutex_init               ( mut attr -- err )
1 1 extern: pthread_mutex_destroy            ( mut      -- err )
1 1 extern: pthread_mutex_trylock            ( mut      -- err )
1 1 extern: pthread_mutex_lock               ( mut      -- err )
1 1 extern: pthread_mutex_unlock             ( mut      -- err )

2 1 extern: pthread_mutex_getprioceiling     ( mut ceil_adr      -- err )
3 1 extern: pthread_mutex_setprioceiling     ( mut ceil ceil_adr -- err )

1 1 extern: pthread_rwlockattr_init          ( attr            -- err )
1 1 extern: pthread_rwlockattr_destroy       ( attr            -- err )
2 1 extern: pthread_rwlockattr_getpshared    ( attr shared_adr -- err )
2 1 extern: pthread_rwlockattr_setpshared    ( attr shared     -- err )

2 1 extern: pthread_rwlock_init              ( lock attr -- err )
1 1 extern: pthread_rwlock_destroy           ( lock      -- err )
1 1 extern: pthread_rwlock_tryrdlock         ( lock      -- err )
1 1 extern: pthread_rwlock_trywrlock         ( lock      -- err )
1 1 extern: pthread_rwlock_rdlock            ( lock      -- err )
1 1 extern: pthread_rwlock_wrlock            ( lock      -- err )
1 1 extern: pthread_rwlock_unlock            ( lock      -- err )

2 1 extern: pthread_cond_signal_thread_np    ( cond thread -- err )

\ `Timespec` is defined in `./time.f`.
3 1 extern: pthread_cond_timedwait_relative_np ( cond mutex timespec -- err )

0 1 extern: pthread_jit_write_protect_supported_np (         -- err )
1 0 extern: pthread_jit_write_protect_np           ( enabled --     )
0 0 extern: pthread_jit_write_freeze_callbacks_np  (         --     )

\ Not found via `dlsym`.
\ 2 1 extern: pthread_jit_write_with_callback_np
