import' std:lang.f

\ Adapted from Gforth's `sieve.fs`.

4096        let: RUNS
8192        let: CAP
CAP         mem: FLAGS
FLAGS CAP + let: EFLAG

\ Calling this separately prevents this from evicting all locals to memory
\ due to clobbering all volatile registers. The compiler needs better local
\ allocation policies. Using callee-saved registers would be a simple way
\ to solve this.
: reset FLAGS CAP 1 fill ;

: find_prime { -- num }
  0 3 { num step }

  EFLAG FLAGS +for: ptr0
    ptr0 @u8 if
      ptr0 step + { ptr1 }

      EFLAG ptr0 step +loop: ptr1
        0 ptr1 !8
      end

      num inc { num }
    end

    step 2 + { step }
  end

  num
;

: main
  \ reset find_prime .

  RUNS for
    reset
    find_prime { -- }
  end
;
main
