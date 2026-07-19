; BOT-TRANSLATED

(declaim (optimize (speed 3) (safety 0) (debug 0)))

(declaim (ftype (function (simple-string)) reverse-string))
(defun reverse-string (str)
  (declare (simple-string str))
  (loop
    :for low fixnum
    :from 0
    :for high fixnum
    :downfrom (1- (length str))
    :while (< low high)
    :do (rotatef (schar str low) (schar str high))
  )
)

(defun run (str runs)
  (declare (simple-string str) (fixnum runs))
  (loop :repeat runs :do (reverse-string str))
  (reverse-string str)
  (unless
    (string= str "fedcba9876543210")
    (error "reverse-string output mismatch")
  )
)

(run (copy-seq "0123456789abcdef") (ash 1 25))
