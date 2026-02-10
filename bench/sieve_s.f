import' ../forth/lang_s.f

\ Adapted from Gforth's `sieve.fs`.

4096        let: RUNS
8192        let: LEN
LEN         mem: FLAGS
FLAGS LEN + let: EFLAG

: find_prime ( -- num )
  FLAGS LEN 1 fill

  0 3 ( num step )

  EFLAG FLAGS +for: ptr0
    ptr0 c@ if
      dup ptr0 +
      dup EFLAG < if
        EFLAG swap 2 pick +loop: ptr1 0 ptr1 c! end
      else
        drop
      end
      swap inc swap
    end
    2 +
  end
  drop
;

: main
  \ find_prime .

  RUNS for
    find_prime drop
  end
;
main

(
Runs notably slower.

: find_prime
  FLAGS LEN 1 fill

  0 3 { num step }

  EFLAG FLAGS +for: ptr0
    ptr0 c@ if
      ptr0 step + { ptr1 }

      EFLAG ptr0 step +loop: ptr1
        0 ptr1 c!
      end

      num inc { num }
    end

    step 2 + { step }
  end

  num
;
)
