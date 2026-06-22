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
  (dotimes (_ runs) (fib depth))
  ; (princ (fib depth)) (write-char #\newline)
)

(run 184 (ash 1 18))
