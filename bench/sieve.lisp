; BOT-TRANSLATED with tweaks.

(declaim (optimize (speed 3) (safety 0) (debug 0)))

(defmacro fntyp (name inp out) `(declaim (ftype (function ,inp ,out) ,name)))
(deftype byte-t nil '(unsigned-byte 8))
(deftype bytes-t nil '(simple-array byte-t (*)))

(fntyp reset-flags (bytes-t) t)
(defun reset-flags (flags) (fill flags 1))

(fntyp find-prime (bytes-t) t)
(defun find-prime (flags)
  (reset-flags flags)
  (let
    (
      (num 0)
      (step 3)
    )
    (declare (fixnum num step))
    (loop
      :for ind
      :below (length flags)
      :do
      (when
        (/= (aref flags ind) 0)
        (loop
          :for ind1
          :from (+ ind step)
          :below (length flags)
          :by step
          :do (setf (aref flags ind1) 0)
        )
        (incf num)
      )
      (incf step 2)
    )
    num
  )
)

(defun main (len runs)
  (
    let
    ((flags (make-array len :element-type 'byte-t)))
    (declare (bytes-t flags))
    (loop :repeat runs :do (find-prime flags))
    ; (princ (find-prime flags)) (write-char #\newline)
  )
)

(main 8192 4096)
