# Exceptions done right

This system implements exceptions in an interesting way, designed for perfect ABI interop between languages.

An exception is a Go-style return value, implicitly appended to the success outputs. By default it's invisible and implicitly rethrown ("try"). However, the compiler allows to "opt out" of exceptions in two ways:

- Per function call: regular "catch".
- _Per function definition_, revealing all errors!

If you "catch", a call becomes Go-style. The compiler reveals the error and skips the "try" instructions.

```forth
: outer
  \ With hidden "try" instructions:
  inner0 {           }
  inner1 { val0      }
  inner2 { val0 val1 }

  \ No exceptions and no hidden instructions:
  catch' inner0 {           err } \ registers: x0
  catch' inner1 { val0      err } \ registers: x0 x1
  catch' inner2 { val0 val1 err } \ registers: x0 x1 x2
;
```

- The compiler knows which words throw. When callee doesn't throw, no "try" instructions are emitted.
- "Throw" uses 1 instruction.
- "Try", which is implicit by default, uses between 1 and 3 instructions depending on the order of output registers between caller and callee.

```asm
// If register matches:
cbnz <err>, <epilogue>


// If caller has more outputs:
mov  <err>, <callee_err>
cbnz <err>, <epilogue>


// If callee has more outputs:
cbz <callee_err>, <skip>
mov <err>, <callee_err>
b   <epilogue>
```

You can automatically catch _everything_, revealing all errors and ensuring that the current word doesn't implicitly throw:

```forth
: outer [ true catches ]
  inner0 {           err }
  inner1 { val0      err }
  inner2 { val1 val2 err }
;
```

Even with implicit `catch`, you can `throw` or `try`. The control flow is entirely local; there are no _implicit_ exceptions. (The word is now known to "throw"; its error can be caught. External callers such as `libc` would silently ignore the error.)

```forth
: outer [ true catches ]
  inner0 {           err } err try
  inner1 { val0      err } err throw
  inner2 { val1 val2 err } err try

  ( E: --     ) \ Official signature of `outer`.
  ( E: -- err ) \ Real signature of `outer` in the ABI.
;
```

This is an exact inverse of Go (and Rust). By default, errors are exceptions and don't clutter the code. But when you really want explicit errors, it's _for real_, without hidden panics. There is no separate panic mechanism, and no stack unwinder. At the ABI level, calling code always has local control.

## ABI compat

The best part is _cross-language ABI compatibility_. Having exceptions without a runtime or unwinder means that other languages can seamlessly call our functions and handle returned errors. We can pass callbacks to foreign callers, allow them to receive Forth errors as return values at no added cost, and guarantee no surprises, such as a panic handler unwinding the C stack.

```forth
: some_forth_word
  { some_input -- }      \ Parameter signature in Forth.
  ( E: inp -- err )      \ Real ABI signature.
  throw" mock exception" \ Returned in first output register.
;
```

```c
typedef const char *Err;

extern Err some_forth_word(int64_t some_input);

int main() {
  const auto err = some_forth_word(123);
  puts(err);
}
```

This is easy to implement. Not aware of any other language taking this approach, which is strange. I think it's the only reasonable way of doing exceptions and errors in general.

## Passing callbacks to `libc`

See `/examples/threads_*.f` which show seamless interop between `libc` and our system, which uses "exceptions".

## Reg-CC vs stack-CC

The system comes in two variants: reg-CC (main) and stack-CC (legacy).

The "catching" mode is only really usable in reg-CC, where outputs map to registers, and output arity is fixed. It's not practical in stack-CC, where the amount of values pushed to the stack depends on whether an exception had occurred. Trying to `{ val err }` often results in a stack underflow in case of exceptions.

Stack-CC actually does have a hidden unwinder, but only for detecting and reporting stack underflow and overflow, which cause segfaults in compiled code. It only unwinds Forth frames. Reg-CC doesn't have this because the compiler finds these mistakes before they happen.
