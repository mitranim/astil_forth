(declaim (optimize (speed 3) (safety 0) (debug 0)))

(declaim (ftype (function (fixnum) fixnum) fib))
(defun fib (ind)
  (if
    (<= ind 1)
    1
    (+ (fib (- ind 1)) (fib (- ind 2)))
  )
)

(unless
  (= (fib 39) 102334155)
  (error "recursive Fibonacci output mismatch")
)
