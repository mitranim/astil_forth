(declaim (optimize (speed 3) (safety 0) (debug 0)))

(deftype u64 () '(unsigned-byte 64))

(declaim (ftype (function (fixnum) u64) fib))
(defun fib (depth)
  (let
    (
      (prev 0)
      (next 1)
    )
    (declare (u64 prev next))
    (loop
      :repeat depth
      :do (psetq prev next next (+ prev next))
    )
    next
  )
)

(defun run (depth runs)
  (let
    ((out 0))
    (declare (u64 out))
    (loop :repeat runs :do (setf out (fib depth)))
    (unless
      (= out 7540113804746346429)
      (error "iterative Fibonacci output mismatch")
    )
  )
)

(run 91 (ash 1 22))
