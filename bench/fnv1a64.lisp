; BOT-ASSISTED

(declaim (optimize (speed 3) (safety 0) (debug 0)))

(defconstant +cap+ 65536)
(defconstant +runs+ 2048)
(defconstant +offset+ #xcbf29ce484222325)
(defconstant +prime+ #x100000001b3)
(defconstant +mask+ #xffffffffffffffff)
(defconstant +want+ #xb0a1ea8560222325)
(defparameter *buf* (make-array +cap+ :element-type '(unsigned-byte 8)))

(defun init ()
  (declare (type (simple-array (unsigned-byte 8) (*)) *buf*))
  (let
    ((pat "0123456789abcdef"))
    (declare (type simple-string pat))
    (dotimes
      (ind +cap+)
      (setf (aref *buf* ind) (char-code (char pat (logand ind 15))))
    )
  )
)

(defun fnv1a64 (hash src len)
  (declare
    (type (unsigned-byte 64) hash)
    (type (simple-array (unsigned-byte 8) (*)) src)
    (type fixnum len)
  )
  (dotimes
    (ind len hash)
    (setf hash (logand (* (logxor hash (aref src ind)) +prime+) +mask+))
  )
)

(init)
(let
  ((hash +offset+))
  (dotimes
    (_ +runs+)
    (setf hash (fnv1a64 hash *buf* +cap+))
  )
  (unless
    (= hash +want+)
    (error "mismatch: expected ~x; got ~x" +want+ hash)
  )
)
