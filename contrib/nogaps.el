(require 'cl)

(defun ratpoison-nogaps ()
  (let ((wins (mapcar 'string-to-number (split-string (ratpoison-windows "%n")))))
  (loop for n in wins
	for i from 1 to (length wins)
	do (ratpoison-number i n))))
