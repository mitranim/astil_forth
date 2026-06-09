(declaim (optimize (speed 3) (safety 0) (debug 0)))

(defun fib (depth)
  (let
    (
      (prev 0)
      (next 1)
    )
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

(run 184 (ash 1 18))
