## Changes compared to other Forths

(Which I consider to be improvements.)

Words are case-sensitive.

```forth
: one 123 ;
: ONE 234 ;
```

Numeric literals are unambiguous.
- Anything that begins with `[+-]?\d` must be a valid number followed by whitespace or EOF.
- `+1` or `-1` can only be a numeric literal.
- It's not possible to redefine a number!

Non-decimal numeric literals are denoted with `0b 0o 0x`. Radix prefixes are case-sensitive. Numbers may contain cosmetic `_` separators. There is no `hex` mode. There is no support for `& # % $` prefixes.

```forth
0b1100_0111_1110_0100
0o0123_4567_7654_3210
0x0123_4567_89AB_CDEF
```

Many unclear words are replaced with clear ones.

More ergonomic control flow structures:
- `elif` is supported.
- Any amount of `else elif` is popped with a single `end`.
- Most loops are terminated with `end`. No need to remember other terminators.
- Loop controls like `leave` and `while` are terminated by the same `end` as the loop.

```forth
10 #if 20 #else 30 #elif 40 #else 50 #end

#loop true #while #leave #leave #leave #end

#begin
  dup2 > #while
  space space
  dup pick0 over . cr
  inc
#repeat
```

Separate wordlists for regular and compile-time words.

```forth
:  char' parse_char           ;
:: char' parse_char comp_push ;

:  " parse_str ;
:: " comp_str  ;

:  log" parse_str drop     puts ;
:: log" comp_cstr compile' puts ;

: some_word
  compile'  char'
  compile'' char'
;
```

Exceptions are strings (error messages) rather than numeric codes.

```forth
throw" readable error message"

10 20 [ 2 ] throwf" error codes: %lld %lld"
```

Booleans are `0 1` rather than `0 -1`.

Special _semantic_ roles get special _syntactic_ roles.

```forth
import'  ./some_file.f
char'    A
compile' some_word

123 to:  name
234 let: CONST
345 var: VAR

#if
  some new input
  #recur
#end
```

With these rules, you can have a colorFUL Forth in ASCII! And without many special cases; extensible.
