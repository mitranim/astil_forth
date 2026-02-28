# Stack allocation

The system supports two ways of allocating stack memory:
- Local references: addresses of local variables.
- C-style `alloca`: arbitrary space on the system stack.

Both are cleaned up on `ret`. This reduces memory management headaches, especially when using `libc` procedures which require pointers to caller-allocated storage. (Don't have to `free`.)

## Local references

Initialize a local, then use `ref'`:

```forth
: word
  123      \ mov  x0, #123
  { val }  \ str  x0, [x29, #16] -- evicted to memory by `ref'`.
  234      \ mov  x0, #234
  ref' val \ add  x1, x29, #16   -- evicts `val` to memory.
  !        \ stur x0, [x1]
;
```

```asm
; stp  x29, x30, [sp, #-32]!
; mov  x29, sp

  mov  x0, #123
  str  x0, [x29, #16] // { val }
  mov  x0, #234
  add  x1, x29, #16   // ref' val
  stur x0, [x1]       // !

; ldp  x29, x30, [sp], #32
; ret
```

Such locals are generally accessed with `str`. But occasionally, the compiler is able to replace a memory op with a `mov`:

```forth
: word
  123      \ mov x0, #123
  { val }  \ str x0, [x29, #16] -- evicted to memory by `ref'`.
  ref' val \ add x0, x29, #16   -- evicts `val` to memory.
  val      \ ldr x1, [x29, #16] -- load from stable location.
  val      \ mov x2, x1         -- use temp register association.
  { -- }
;
```

Local references are handy for C interop:

```forth
: thread_spawn { fun inp -- thread }
  nil { thread }
  ref' thread nil fun inp pthread_create
  try_errno_posix" unable to spawn a thread"
  thread \ Holds a valid pointer now.
;
```

## `alloca`

Because we're using native calls, it's easy to support C-style `alloca` which reserves an arbitrary amount of space on the system stack.

When size is constant, the compiler inlines the offset into an instruction. When size is dynamic, the compiler takes care of SP alignment.

```forth
: example_alloca
  32 alloca { one } \ sub x0, sp, #32 ; mov sp, x0
  64 alloca { two } \ sub x0, sp, #64 ; mov sp, x0
  one two { -- }
;
```

In simple cases, addresses can be kept in registers. In real cases, they might spill to the stack:

```asm
stp  x29, x30, [sp, #-16]!
mov  x29, sp
sub  x0, sp, #32 // reserve memory for `one`
mov  sp, x0
mov  x2, x0      // relocate `one` due to clobber
sub  x0, sp, #64 // reserve memory for `two`
mov  sp, x0
mov  x3, x0      // relocate `two` due to clobber
mov  x0, x2      // use `one`
mov  x1, x3      // use `two`
mov  sp, x29
ldp  x29, x30, [sp], #16
ret
```

## Usage with `libc`

The main use of `alloca` is for stack-allocated structs.
This often eliminates globals and avoids heap allocation.

```forth
2 0 extern: clock_gettime

\ C: `struct timespec`
struct: Timespec
  S64 1 field: Time_sec
  S64 1 field: Time_nsec
end

: example
  Timespec alloca { inst }
  0 inst clock_gettime

  inst Timespec_sec  @ { secs }
  inst Timespec_nsec @ { nano }

  secs logf" real seconds: %zd" lf
  nano logf" real nanos:   %zd" lf
;
example

\ real seconds: 1772116542
\ real nanos:   322626000
```

Also see `forth/time.f`, `forth/io.f`, and `examples`.
