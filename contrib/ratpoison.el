;;; ratpoison.el --- ratpoison support for Emacs
;;
;; Copyright (C) 2003 Gergely Nagy, Shawn Betts, Jay Belanger
;;
;; Authors: Gergely Nagy <algernon@debian.org>,
;;          Shawn Betts <sabetts@users.sourceforge.net>,
;;          Jay Belanger <belanger@truman.edu>,
;; Maintainer: Gergely Nagy <algernon@debian.org>
;; Version: 0.2
;; Keywords: faces, ratpoison, X
;; CVS Id: $Id: ratpoison.el,v 1.5 2006/03/16 00:33:34 sabetts Exp $
;; Last updated: <2001/10/05 17:58:38 algernon>

;; This file is NOT part of GNU Emacs.

;; This program is free software; you can redistribute it and/or
;; modify it under the terms of the GNU General Public License as
;; published by the Free Software Foundation; either version 2 of
;; the License, or (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public
;; License along with this program; if not, write to the Free
;; Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
;; MA 02111-1307 USA

;;; Commentary:
;;
;; This file provides a major mode for editing .ratpoisonrc files, and
;; functions to access ratpoison from within Emacs.

;;; History:
;; Version 0.2:
;;  - Added command-interpreter (from Shawn and Jay)
;;  - Added info-lookup functions (from Jay)
;;  - renamed to ratpoison.el
;;  - far better font-locking
;; Version 0.1:
;;  - initial version

;;; Todo:
;; - auto-completion of commands
;; - probably a bunch of other things

(require 'font-lock)
(require 'generic-x)

(defvar ratpoison-commands-0
  (list
   "abort"
   "banish"
   "clock"
   "curframe"
   "delete"
   "focus"
   "focusup"
   "focusdown"
   "focusleft"
   "focusright"
   "meta"
   "help"
   "info"
   "kill"
   "lastmsg"
   "redisplay"
   "restart"
   "next"
   "only"
   "other"
   "prev"
   "quit"
   "remove"
   "split"
   "hsplit"
   "version"
   "vsplit"
   ))

(defvar ratpoison-commands-rest
  (list
   "bind"
   "chdir"
   "colon"
   "defbarloc"
   "msgwait"
   "defborder"
   "deffont"
   "definputwidth"
   "defmaxsizepos"
   "defpadding"
   "deftranspos"
   "defwaitcursor"
   "defwinfmt"
   "defwinname"
   "defwinpos"
   "deffgcolor"
   "defbgcolor"
   "escape"
   "echo"
   "exec"
   "newwm"
   "number"
   "pos"
   "rudeness"
   "select"
   "setenv"
   "source"
   "startup_message"
   "title"
   "unbind"
   "unsetenv"
   "windows"
   ))

;; ratpoisonrc-mode
(define-generic-mode 'ratpoisonrc-mode
  ;; comments
  (list ?#)
  ;; keywords
  nil
  ;; font-lock stuff
  (list
    ;; commands without arguments
    (generic-make-keywords-list
     ratpoison-commands-0 font-lock-builtin-face "^[ \t]*")
    ;; commands with arguments
    (generic-make-keywords-list
     ratpoison-commands-rest font-lock-builtin-face "^[ \t]*" "[ \t]+")
    ;; exec <arg>
    (list "^[ \t]*\\(exec\\)[ \t]+\\(.*\\)"
          '(1 'font-lock-builtin-face)
          '(2 'font-lock-string-face))
    ;; arguments, the first is a keyword, the rest is tring
    (list (concat
                (car (generic-make-keywords-list
                      ratpoison-commands-rest font-lock-builtin-face "^[ \t]*" "[ \t]+"))
                "\\([0-9a-zA-Z\\/\\.\\-]+\\)[ \t]*\\(.*\\)")
          '(2 'font-lock-keyword-face)
          '(3 'font-lock-string-face)))
  ;; auto-mode alist
  (list "\\.ratpoisonrc\\'")
  ;; additional setup functions
  (list 'ratpoisonrc-mode-setup)
  "Generic mode for ratpoison configuration files.")

(defun ratpoisonrc-mode-setup()
  (defvar ratpoisonrc-mode-keymap (make-sparse-keymap)
    "Keymap for ratpoisonrc-mode")
  (define-key ratpoisonrc-mode-keymap "\C-c\C-e" 'ratpoison-line)
  (use-local-map ratpoisonrc-mode-keymap))

(provide 'ratpoisonrc-mode)

;; Ratpoison access
; Groups & Variables
(defgroup ratpoison nil "Ratpoison access"
  :group 'languages
  :prefix "ratpoison-")

(defcustom ratpoison-program "ratpoison"
  "The command to call the window manager."
  :group 'ratpoison
  :type 'string)

; Command stuff
(defun ratpoison-command (command)
  (interactive "sRP Command: ")
  (call-process ratpoison-program nil nil nil "-c" command))

(defun ratpoison-command-on-region (start end)
  (interactive "r")
  (mapcar 'ratpoison-command
          (split-string (buffer-substring start end)
                        "\n")))

(defun ratpoison-line ()
  "Send the current line to ratpoison."
  (interactive)
  (ratpoison-command
   (buffer-substring-no-properties
    (line-beginning-position)
    (line-end-position))))

;; Documentation
(defun ratpoison-info ()
  "Call up the ratpoison info page."
  (interactive)
  (info "ratpoison"))

(defun ratpoison-command-info ()
  "Call up the info page listing the ratpoison commands."
  (interactive)
  (info "(ratpoison) Commands"))

(provide 'ratpoison)
