; BOT-TRANSLATED from reg-CC file.

(declaim (optimize (speed 3) (safety 0) (debug 0)))

(declaim
  (ftype (function (fixnum) cons) node-tree-make)
  (ftype (function (cons) fixnum) node-tree-count)
)

(defun node-tree-make (depth)
  (declare (type fixnum depth))
  (if
    (zerop depth)
    (cons nil nil)
    (let
      ((depth (1- depth)))
      (declare (type fixnum depth))
      (cons (node-tree-make depth) (node-tree-make depth))
    )
  )
)

(defun node-tree-count (node)
  (declare (type cons node))
  (let
    ((left (car node)))
    (declare (type (or null cons) left))
    (if
      left
      (the
        fixnum
        (+
          1
          (node-tree-count left)
          (node-tree-count (the cons (cdr node)))
        )
      )
      1
    )
  )
)

(defun run ()
  (let*
    (
      (min-depth 4)
      (max-depth 18)
      (stretch-depth (1+ max-depth))
      (long-lived-tree (node-tree-make max-depth))
    )
    (let
      ((out (with-output-to-string (stream) (let ((tree (node-tree-make stretch-depth))) (format stream "stretch tree of depth ~D; count: ~D~%" stretch-depth (node-tree-count tree))) (loop :for depth :from min-depth :to max-depth :by 2 :do (let ((count 0) (iters (ash 1 (+ (- max-depth depth) min-depth)))) (dotimes (run iters) (declare (ignore run)) (incf count (node-tree-count (node-tree-make depth)))) (format stream "~D trees of depth ~D; count: ~D~%" iters depth count))) (format stream "long lived tree of depth ~D; count: ~D~%" max-depth (node-tree-count long-lived-tree)))))
      (unless
        (string=
          out
          "stretch tree of depth 19; count: 1048575
262144 trees of depth 4; count: 8126464
65536 trees of depth 6; count: 8323072
16384 trees of depth 8; count: 8372224
4096 trees of depth 10; count: 8384512
1024 trees of depth 12; count: 8387584
256 trees of depth 14; count: 8388352
64 trees of depth 16; count: 8388544
16 trees of depth 18; count: 8388592
long lived tree of depth 18; count: 524287
"
        )
        (error "mismatch:~%~A" out)
      )
    )
  )
)

(run)
