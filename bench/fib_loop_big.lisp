(declaim (optimize (speed 3) (safety 0) (debug 0)))

(declaim (ftype (function (fixnum) integer) fib))
(defun fib (depth)
  (declare (fixnum depth))
  (let
    (
      (prev 0)
      (next 1)
    )
    (dotimes (_ depth) (psetq prev next next (+ prev next)))
    next
  )
)

(declaim (ftype (function (fixnum fixnum) null) run))
(defun run (depth runs)
  (declare (fixnum depth runs))
  (let
    ((out 0))
    (declare (integer out))
    (dotimes (_ runs) (setf out (fib depth)))
    (unless
      (= out 205697230343233228174223751303346572685)
      (error "big iterative Fibonacci output mismatch")
    )
  )
)

(run 184 (ash 1 21))
