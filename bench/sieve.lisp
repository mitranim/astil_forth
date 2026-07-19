; BOT-TRANSLATED with tweaks.

(declaim (optimize (speed 3) (safety 0) (debug 0)))

(defmacro fntyp (name inp out) `(declaim (ftype (function ,inp ,out) ,name)))
(deftype byte-t nil '(unsigned-byte 8))
(deftype bytes-t nil '(simple-array byte-t (*)))

(fntyp reset-flags (bytes-t) t)
(defun reset-flags (flags) (fill flags 1))

(fntyp find-prime (bytes-t) fixnum)
(defun find-prime (flags)
  (declare (bytes-t flags))
  (reset-flags flags)
  (let
    (
      (num 0)
      (step 3)
      (len (length flags))
    )
    (declare (fixnum num step len))
    (loop
      :for ind fixnum
      :below len
      :do
      (when
        (/= (aref flags ind) 0)
        (loop
          :for ind1 fixnum
          :from (+ ind step)
          :below len
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

(declaim (ftype (function (fixnum fixnum) null) main))
(defun main (len runs)
  (declare (fixnum len runs))
  (let
    (
      (flags (make-array len :element-type 'byte-t))
      (out 0)
    )
    (declare (bytes-t flags) (fixnum out))
    (dotimes (_ runs) (setf out (find-prime flags)))
    (unless (= out 1899) (error "mismatch: expected 1899; got ~d" out))
  )
)

(main 8192 16384)
