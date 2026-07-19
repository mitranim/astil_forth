; BOT-TRANSLATED with tweaks.

(declaim (optimize (speed 3) (safety 0) (debug 0)))

(deftype fixnums-t nil '(simple-array fixnum (*)))

(defun list-init (list)
  (declare (fixnums-t list))
  (let
    ((seed 74755))
    (loop
      :for ind
      :below (length list)
      :do
      (let
        ((val (logand (+ (* seed 1309) 13849) 65535)))
        (setf seed val (aref list ind) val)
      )
    )
  )
)

(defun list-verify (list)
  (declare (fixnums-t list))
  (loop
    :for ind
    :below (1- (length list))
    :when (> (aref list ind) (aref list (1+ ind)))
    :do (error "[bubble] not sorted")
  )
)

(defun bubble (list)
  (declare (fixnums-t list))
  (loop
    :for ceil
    :downfrom (1- (length list))
    :above 0
    :do
    (loop
      :for ind
      :below ceil
      :when (> (aref list ind) (aref list (1+ ind)))
      :do (rotatef (aref list ind) (aref list (1+ ind)))
    )
  )
)

(defun bubble-sort (len)
  (let
    ((list (make-array len :element-type 'fixnum)))
    (declare (fixnums-t list))
    (list-init list)
    (bubble list)
    (list-verify list)
  )
)

(bubble-sort 32768)
