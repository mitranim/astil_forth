import' ../forth/lang_r.f

\ Adapted from Gforth's `sieve.fs`.

4096        let: RUNS
8192        let: CAP
CAP         mem: FLAGS
FLAGS CAP + let: EFLAG

: find_prime { -- num }
  FLAGS CAP 1 fill

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

: main
  \ find_prime .

  RUNS for
    find_prime { -- }
  end
;
main
