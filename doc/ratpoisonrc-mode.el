;;; ratpoisonrc-mode.el --- .ratpoisonrc syntax-highlighting mode for Emacs

;; Author: Gergely Nagy <algernon@debian.org>
;; Maintainer: Gergely Nagy
;; Version: 0.1
;; Keywords: faces, ratpoison, X
;; CVS Id: $Id: ratpoisonrc-mode.el,v 1.1 2001/09/06 20:09:50 algernon Exp $
;; Last updated: <2001/09/06 17:57:24 algernon>

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
;; This file provides a major mode for editing .ratpoisonrc files. At
;; them moment it only provides syntax-highlighting.

;;; Todo:
;; - auto-completion of commands
;; - more intelligent highlighting (eg: highlight arguments)
;; - syntax checking

(define-generic-mode 'ratpoisonrc-mode
     (list ?#)
        (list
	 "abort"
	 "next"
	 "prev"
	 "exec"
	 "select"
	 "colon"
	 "kill"
	 "delete"
	 "other"
	 "windows"
	 "title"
	 "clock"
	 "maximize"
	 "newwm"
	 "generate"
	 "version"
	 "bind"
	 "unbind"
	 "source"
	 "escape"
	 "echo"
	 "split"
	 "hsplit"
	 "vsplit"
	 "focus"
	 "only"
	 "remove"
	 "banish"
	 "curframe"
	 "help"
	 "quit"
	 "number"
	 "rudeness"
	 )
	nil
	(list "\\.ratpoisonrc\\'")
	nil
	"Generic mode for ratpoison configuration files.")

(provide 'ratpoison-mode)
