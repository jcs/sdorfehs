;;; Copyright (C) 2003, 2004 Shawn Betts
;;;
;;; Copying and distribution of this file, with or without modification,
;;; are permitted in any medium without royalty provided the copyright
;;; notice and this notice are preserved.

(eval-when-compile
  (require 'cl))

(require 'ratpoison-cmd)

(defun ratpoison-nogaps ()
  (let ((wins (mapcar 'string-to-number (split-string (ratpoison-windows "%n")))))
  (loop for n in wins
        for i from 1 to (length wins)
        do (ratpoison-number i n))))
