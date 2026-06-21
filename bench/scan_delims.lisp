; BOT-ASSISTED

(declaim (optimize (speed 3) (safety 0) (debug 0)))
(defconstant +cap+ (ash 1 16))
(defconstant +runs+ (/ (ash 1 24) (/ +cap+ 16)))
(defconstant +want+ (* (ash 1 24) 9))
(defparameter *buf* (make-array +cap+ :element-type '(unsigned-byte 8)))
(defparameter *dels* (make-array 256 :element-type '(unsigned-byte 8) :initial-element 0))

(let ((pat (format nil "{a,b:c[d]e} ~%~cfg" #\Tab)))
  (declare (type simple-string pat))
  (dotimes (ind +cap+)
    (setf (aref *buf* ind) (char-code (char pat (logand ind 15))))))

(dolist (char '(#\{ #\} #\[ #\] #\: #\, #\Space #\Newline #\Tab))
  (setf (aref *dels* (char-code char)) 1))

(defun scan (buf cap dels)
  (declare (type (simple-array (unsigned-byte 8) (*)) buf)
           (type (simple-array (unsigned-byte 8) (*)) dels)
           (type fixnum cap))
  (let ((out 0))
    (declare (type fixnum out))
    (dotimes (ind cap out)
      (incf out (aref dels (aref buf ind))))))

(let ((out 0))
  (declare (type fixnum out))
  (dotimes (_ +runs+)
    (incf out (scan *buf* +cap+ *dels*)))
  (when (sb-ext:posix-getenv "SCAN_DELIMS_PRINT") (format t "~D~%" out))
  (unless (= out +want+) (error "mismatch: expected ~d; got ~d" +want+ out)))
