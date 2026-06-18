; BOT-GENERATED

(declaim (optimize (speed 3) (safety 0) (debug 0)))
(defconstant +cap+ (ash 1 16))
(defconstant +reps+ (/ (ash 1 24) (/ +cap+ 16)))
(defconstant +want+ (* (ash 1 24) 9))
(defparameter *pat*
  (make-array 16 :element-type '(unsigned-byte 8)
                 :initial-contents '(123 97 44 98 58 99 91 100
                                      93 101 125 32 10 9 102 103)))
(defparameter *buf* (make-array +cap+ :element-type '(unsigned-byte 8)))

(dotimes (i +cap+)
  (setf (aref *buf* i) (aref *pat* (logand i 15))))

(declaim (inline delim-p))
(defun delim-p (c)
  (declare (type (unsigned-byte 8) c))
  (if (or (= c 123) (= c 125) (= c 91) (= c 93) (= c 58)
          (= c 44) (= c 32) (= c 10) (= c 9))
      1 0))

(let ((out 0))
  (declare (type fixnum out))
  (dotimes (_ +reps+)
    (dotimes (j +cap+)
      (incf out (delim-p (aref *buf* j)))))
  (when (sb-ext:posix-getenv "SCAN_DELIMS_PRINT") (format t "~D~%" out))
  (unless (= out +want+) (sb-ext:exit :code 1)))
