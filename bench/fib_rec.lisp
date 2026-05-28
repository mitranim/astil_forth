(declaim (optimize (speed 3) (safety 0) (debug 0)))

(declaim (ftype (function (fixnum) fixnum) fib))
(defun fib (ind)
  (if
    (<= ind 1)
    1
    (+ (fib (- ind 1)) (fib (- ind 2)))
  )
)

(fib 36)

; (princ (fib 36)) (write-char #\newline)
