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
  (loop :repeat runs :do (fib depth))
  ; (princ (fib depth)) (write-char #\newline)
)

(run 91 (ash 1 20))
