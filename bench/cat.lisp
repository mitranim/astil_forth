; BOT-GENERATED

(declaim (optimize (speed 3) (safety 1) (debug 0)))

(defconstant +buffer-size+ (* 64 1024))

(defun copy-stream (input output buffer)
  (loop
    (let
      ((length (read-sequence buffer input)))
      (when
        (zerop length)
        (return)
      )
      (write-sequence buffer output :end length)
    )
  )
)

(defun main ()
  (let
    (
      (stdin
        (sb-sys:make-fd-stream
          0
          :input t
          :element-type '(unsigned-byte 8)
          :buffering :none
          :auto-close nil
        )
      )
      (stdout
        (sb-sys:make-fd-stream
          1
          :output t
          :element-type '(unsigned-byte 8)
          :buffering :none
          :auto-close nil
        )
      )
      (buffer
        (make-array
          +buffer-size+
          :element-type '(unsigned-byte 8)
        )
      )
      (arguments (rest sb-ext:*posix-argv*))
    )
    (if
      arguments
      (dolist
        (name arguments)
        (if
          (string= name "-")
          (copy-stream stdin stdout buffer)
          (with-open-file
            (input
              name
              :direction :input
              :element-type '(unsigned-byte 8)
            )
            (copy-stream input stdout buffer)
          )
        )
      )
      (copy-stream stdin stdout buffer)
    )
    (finish-output stdout)
  )
)

(main)
