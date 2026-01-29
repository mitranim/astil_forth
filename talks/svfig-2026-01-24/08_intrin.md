## Intrinsics

The interpreter / compiler provides intrinsics only for:
- definitions
- compilation
- parser control
- debugging

It does _not_ provide any of:
- arithmetic
- conditionals
- loops
- I/O

```
: :: ; [ ]
colon_named
colon_colon_named
#ret
#recur
comp_only
not_comp_only
inline
throws
redefine
here
alloc_data
quit

comp_instr
comp_load
comp_page_addr
comp_page_load
comp_call

char
parse
parse_word

import
import"
import'
extern_got
extern:

find_word
inline_word
execute

get_local
anon_local

debug_on
debug_off
debug_flush
debug_throw
debug_stack_len
debug_stack
debug_depth
debug_top_int
debug_top_ptr
debug_top_str
debug_mem
debug'
debug_sync_code
```
