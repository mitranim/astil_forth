; BOT-GENERATED

(require :sb-bsd-sockets)

(declaim (optimize (speed 3) (safety 1) (debug 0)))

(defconstant +ready+ (char-code #\R))
(defconstant +data+ (char-code #\D))
(defconstant +port+ 19777)

(defun handle-connection (socket)
  (unwind-protect
    (let
      ((buffer (make-array 1 :element-type '(unsigned-byte 8) :initial-element +ready+)))
      (assert (= 1 (sb-bsd-sockets:socket-send socket buffer 1)))
      (let
        ((length (nth-value 1 (sb-bsd-sockets:socket-receive socket buffer 1))))
        (assert (and (= length 1) (= (aref buffer 0) +data+)))
      )
      (assert (= 1 (sb-bsd-sockets:socket-send socket buffer 1)))
    )
    (sb-bsd-sockets:socket-close socket)
  )
)

(defun run ()
  (let
    ((listener (make-instance 'sb-bsd-sockets:inet-socket :type :stream :protocol :tcp)))
    (setf (sb-bsd-sockets:sockopt-reuse-address listener) t)
    (sb-bsd-sockets:socket-bind listener #(127 0 0 1) +port+)
    (sb-bsd-sockets:socket-listen listener 128)
    (loop
      (let
        ((socket (sb-bsd-sockets:socket-accept listener)))
        (sb-thread:make-thread
          (lambda () (handle-connection socket))
        )
      )
    )
  )
)

; Warmup generic dispatch before connection threads close concurrently.
(sb-bsd-sockets:socket-close
  (make-instance 'sb-bsd-sockets:inet-socket :type :stream :protocol :tcp)
)

(run)
