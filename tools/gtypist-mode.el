 ;;; gtypist-mode.el - major-mode for editing gtypist's .typ-files

;; Author: Felix Natter <fnatter@gmx.net>
;; Created: June 30th 2001
;; Keywords: gtypist major mode

;; This program is free software; you can redistribute it and/or
;; modify it under the terms of the GNU General Public License
;; as published by the Free Software Foundation; either version 2
;; of the License, or (at your option) any later version.
;; 
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;; 
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 ;;; Commentary / README

;; This is a major-mode for editing gtypist script-files (*.typ)
;; 
;; ----- Installing
;; add the following to ~/.emacs (adapt path and remove comments !)
;; (autoload 'gtypist-mode "~/elisp/gtypist-mode")
;; or put this file in load-path and use this:
;; (autoload 'gtypist-mode "gtypist-mode")
;; *and* put this in ~/.emacs:
;; (setq auto-mode-alist       
;;      (cons '("\\.typ\\'" . gtypist-mode) auto-mode-alist))
;; If you want to, you can then byte-compile this using M-x byte-compile-file

 ;;; ChangeLog:
;; Sat Jun 30 18:06:29 2001: initial version with font-locking
;; Sat Jun 30 21:18:41 2001: add gtypist-mode-goto-label,
;; gtypist-mode-insert-hrule
;; Sun Jul  1 12:19:34 2001: don't use add-to-list in ..-get-list-of-labels
;; Sun Jul  1 13:03:28 2001: add gtypist-mode-new-lesson
;; Sun Jul  1 15:12:36 2001: add gtypist-mode-new-banner
;; Wed Jul  4 17:33:39 2001: gtypist-mode-new-banner: center in 66
;; columns (not 80), because "gtypist <version>" is in the right corner
;; Sat Jul  7 14:00:10 2001: add support for indentation, change font-locking
;; Sun Jul 8 2001: add gtypist-mode-next-label. make
;; gtypist-mode-new-banner cut off the text if it's too long, add
;; gtypist-mode-fortune-to-drill, make gtypist-mode-new-banner and
;; gtypist-mode-new-lesson use (interactive "sPrompt")
;; Mon Jul  9 18:46:50 2001: make gtypist-mode-fortune-to-drill use
;; C-u and C-u C-u prefixes, gtypist-mode-get-list-of-labels allows
;; whitespace in labels
;; Tue Jul 10 16:59:26 2001: gtypist-mode-fortune-to-drill: check for invalid
;; prefix, remove last line if it's an "author-line",
;; gtypist-mode-indent-line now checks whether (char-after) is nil
;; Wed Jul 11 16:34:36 2001: set up syntax-tables but don't use it for
;; coloring comments because a comment may only be started at the beginning
;; of the line, and by using search-based coloring this can be honored
;; Thu Jul 12 16:59:03 2001: fix a regex-bug in gtypist-mode-next-label
;; (but this will have to be changed again to allow things like *:SERIES1_L49)
;; Thu Jul 12 19:25:36 2001: (gtypist-mode-fortune-to-drill): add
;; warning + comment (and beep) if fortune is too long
;; Thu Jul 12 19:40:51 2001: (gtypist-mode-fortune-to-drill):
;; add '^' in "author-line"-regexp (otherwise a hyphen is mistaken
;; as an "author-line")
;; Thu Jul 12 21:24:16 2001: (gtypist-mode-next-label): use regexp
;; which allows labels like "*:SERIES2_L33", as mentioned above
;; (suggestion from Stefan Monnier <foo@acm.com>)
;; Sat Jul 14 09:30:45 2001: gtypist-mode-indent-line doesn't modify buffer
;; unless necessary (uses indent-line-to)
;; Sat Jul 14 09:58:31 2001: color TODO/FIXME-comments in
;; font-lock-warning-face
;; Sat Jul 14 13:59:26 2001: add gtypist-mode-font-lock-find-drill-text to
;; color drill-text in font-lock-string-face
;; Sat Jul 14 19:11:09 2001: accept lowercase commands, too (gtypist does it)
;; Sun Jul 15 18:56:20 2001: (gtypist-mode-fortune-to-drill): allow 22-lines
;; if drill-type=P:
;; Wed Jul 18 19:21:28 2001: don't accept lowercase commands
;; Wed Jul 18 19:31:24 2001: (gtypist-mode-new-banner): use insert-char

;; TODO:
;; - gtypist-mode-goto-label-history, gtypist-mode-lesson-name-history ?
;; - better documentation for gtypist-mode-next-label and gtypist-mode
;; - allow customization via customize ?
;; - gtypist-mode-fortune-to-drill: automatically split fortune if it's
;;   too long ?
;; - optimize gtypist-mode-font-lock-find-drill-text

 ;;; Code:

(require 'font-lock)
(require 'thingatpt)
(require 'executable) ;; executable-find

(defvar gtypist-mode-syntax-table nil "Syntax-table for gtypist-mode.")
(unless gtypist-mode-syntax-table
  (setq gtypist-mode-syntax-table (make-syntax-table))
  ;; this isn't used to do font-locking (strings + comments):
  ;; (the KEYWORDS-ONLY-variable of font-lock-defaults is set to t)
  ;; (quoted) strings don't need to be colored and comments must start
  ;; at the beginning of the line (which can only be honored with regexp's)
  ;;
  ;; this is not supported by XEmacs
  ;;  (modify-syntax-entry ?\ "-   " gtypist-mode-syntax-table)
  ;; is this necessary ?
  ;; (modify-syntax-entry ?: ".   " gtypist-mode-syntax-table)
  ;; don't put quoted text in font-lock-string-face:
  (modify-syntax-entry ?\" "." gtypist-mode-syntax-table)
  ;; put comments in font-lock-comment-face
  (modify-syntax-entry ?# "<" gtypist-mode-syntax-table)
  (modify-syntax-entry ?\n ">" gtypist-mode-syntax-table)
  )


(defvar gtypist-mode-map nil "Keymap for gtypist-mode.")
(unless gtypist-mode-map
  (setq gtypist-mode-map (make-sparse-keymap))
  ;; TODO: change order to be more logical (for C-h m) 
  (define-key gtypist-mode-map "\C-c\C-f" 'gtypist-mode-fortune-to-drill)
  (define-key gtypist-mode-map "\C-c\C-n" 'gtypist-mode-new-lesson)
  (define-key gtypist-mode-map "\C-c\C-r" 'gtypist-mode-insert-hrule)
  (define-key gtypist-mode-map "\C-c\C-b" 'gtypist-mode-insert-banner)
  (define-key gtypist-mode-map "\C-c\C-l" 'gtypist-mode-next-label)
  (define-key gtypist-mode-map "\C-c\C-g" 'gtypist-mode-goto-label))

;; TODO: why do some font-lock faces (i.e. font-lock-warning-face)
;; need to be quoted ?
(defconst gtypist-mode-font-lock-keywords
  (list

   (list "^\\(#\\)[ \t]*\\(TODO\\|FIXME\\):\\(.*\\)"
	 (list 1 'font-lock-comment-face)
	 (list 2 'font-lock-warning-face)
	 (list 3 'font-lock-comment-face))

   ;; this is better than coloring via syntax-table, because it
;; doesn't color comments which don't start at the beginning of the line
   (cons "^#.*" 'font-lock-comment-face)

   (list "^\\([X]\\):" 1 'font-lock-warning-face)
   (cons "^\\([A-Z]\\):" 1)
   ;;   (cons "^[A-Z\\*]:" 'font-lock-keyword-face)
   
   ;; labels: definition of labels and references to labels
   (list "^\\*:\\(.*\\)" 1 'font-lock-variable-name-face)
;; note: cannot use font-lock-constant-face because it isn't supported by
   ;; (some versions of) XEmacs
   (list "^[GYNK]:\\(.*:\\)?\\(.*\\)" 2 'font-lock-reference-face)

   ;; (list "^[A-Z\\* ]\\(:\\)" 1 'font-lock-reference-face)
   ;;   (list "^[OPD ]:\\(.*\\)" 1 'font-lock-string-face)

   (list 'gtypist-mode-font-lock-find-drill-text 1 'font-lock-string-face)

   )
  "This constant controls how font-locking is done.")

 ;;;; public functions

(defun gtypist-mode-fortune-to-drill(prefix)
  "Insert a drill (O:) with text from `fortune' (or `yow' if `fortune' isn't
available). Use C-u prefix to get P:, and C-u C-u to get D:."
  (interactive "P")
  (let ((fortune-lines)
	(drill-type
	 (cond 
	  ((null prefix)
	   "O:") ;; no prefix
	  ((numberp prefix)
	   (error "Invalid prefix !"))
	  ((= (car-safe prefix) 4)
	   "P:") ;; C-u
	  ((= (car-safe prefix) 16)
	   "D:")))) ;; C-u C-u
    (if (executable-find "fortune")
	(setq fortune-lines
	      (split-string (shell-command-to-string "fortune") "\n"))
      (setq fortune-lines (split-string (yow) "\n"))
      (message "`fortune' not found. Falling back to `yow'."))
    ;; check whether last line is an "author-line"
    (if (string-match "^[\t ]*-- \\(.+\\)[ \t]*" (car (last fortune-lines)))
	(progn ;; add banner...
	  (gtypist-mode-new-banner (match-string 1 (car (last fortune-lines))))
	  ;; ... and remove "author-line"
	  (setcdr (last fortune-lines 2) nil)))
    ;; emit warning if fortune is too long
    (if (> (length fortune-lines) (if (string-equal drill-type "P:")
				      22
				    11))
	(let ((alt-measure (if (or (string-equal drill-type "P:")
				    (> (length fortune-lines) 22))
			       ""
			     " or use P:")))
	  (insert "# TODO: this is too long (split it" alt-measure ")!\n")
	  (message "This Fortune is too long. Please split it %s!" alt-measure)
	  (beep)))
    ;; first line
    (insert drill-type (car fortune-lines) "\n")
    (setq fortune-lines (cdr fortune-lines))
    (while (car fortune-lines)
      (insert " :" (car fortune-lines) "\n")
      (setq fortune-lines (cdr fortune-lines)))
    ))

(defun gtypist-mode-new-lesson(id)
  "Insert the comments (header) and a label to start a new lesson."
  (interactive "sEnter name of lesson: ")
  (if (string-equal id "")
      (message "Canceled.")
    (progn
      (gtypist-mode-insert-hrule)
      (insert "# Lesson " id "\n")
      (gtypist-mode-insert-hrule)
      (insert "*:" id "\n"))))

(defun gtypist-mode-insert-hrule()
  "Insert horizontal rule (comment) consisting of dashes." (interactive)
  (insert "#")
  (insert-char ?- 78)
  (insert "\n"))

(defun gtypist-mode-insert-banner(text)
  "Insert B:-command with centered content (66 columns because \"gtypist <version>\" is in the right corner)"
  (interactive "sEnter text for banner: ")  
  (if (string-equal text "")
      (message "Canceled."))
  ;; this functionality is needed in gtypist-mode-fortune-to-lesson
  (if (> (length text) 65)
      ;; TODO: first try to find any separators ([;:,]) ?
      ;; no separators found, so just cut off and put "..." at the end
      (setq text (concat (substring text 0 62) "..")))
  (insert "B:")
  (insert-char ?\ (- 33 (/ (length text) 2)))
  (insert text ?\n))

(defun gtypist-mode-next-label()
  "Generate the next numbered label."
  (interactive)
  (let ((pos (point))) ;; TODO: the following regexp needs some testing
    (if (re-search-backward "^\\*:\\(.*[^0-9]\\)\\([0-9]+\\)[ \t]*" nil t)
	(progn
	  (goto-char pos)
	  (insert (concat "*:" (match-string 1)
			  (number-to-string
			   (+ 1 (string-to-number (match-string 2)))) "\n")))
      (message "No numbered label found. You must insert the first one."))))

(defun gtypist-mode-goto-label()
  "Query for a label to go to (with completion)." (interactive)
  (let ((labels (gtypist-mode-get-list-of-labels))
       ;; this is necessary so that searching is done case-sensitively
	(case-fold-search nil)
	(label
	 (if (string-match "^[GYN]:\\(.*\\)[\t ]*" (thing-at-point 'line))
	     (match-string 1 (thing-at-point 'line))
	   (if (string-match "^K:.*:\\(.*\\)[\t ]*" (thing-at-point 'line))
	       (match-string 1 (thing-at-point 'line))
	     nil))))
    (if (string-equal label "NULL")
	(setq label nil))
    (setq label (completing-read "Goto label: " labels nil t label
				 ;; history
				 nil nil))
    (goto-char (cdr (assoc label labels)))
    ))

(defun gtypist-mode-indent-line()
  "Indent the current line."
  (interactive)
  (if (string-match "^[ \t]*:.*" (thing-at-point 'line))
      (indent-line-to 1)
    (indent-line-to 0)))

(defun gtypist-mode()
  "Major mode for editing gtypist's .typ-files.
\\{gtypist-mode-map}"
  (interactive)
  (kill-all-local-variables)
  (use-local-map gtypist-mode-map)
  (set-syntax-table gtypist-mode-syntax-table)
  (make-local-variable 'indent-line-function)
  (setq indent-line-function 'gtypist-mode-indent-line)
  ;; these are used for i.e. comment-region
  (make-local-variable 'comment-start)
  (make-local-variable 'comment-end)
  (make-local-variable 'comment-start-skip)
  (setq comment-start "# "
	comment-end ""
	comment-start-skip "#[^\n]*")
  (make-local-variable 'font-lock-defaults)
  (setq font-lock-defaults
     ;; KEYWORDS KEYWORDS-ONLY CASE-FOLD SYNTAX-ALIST SYNTAX-BEGIN ...
	'(gtypist-mode-font-lock-keywords t nil nil nil))
  (setq major-mode 'gtypist-mode
	mode-name "GNU Typist")
  (run-hooks 'gtypist-mode-hook))

;;;; internal

(defun gtypist-mode-get-list-of-labels()
  "Get the list of gtypist-labels in this buffer (alist name . position)."
  (save-excursion
    (let ((deactivate-mark)
	  (labels))
      (goto-char (point-min))
      (while (re-search-forward "^\\*:\\(.+\\)[\t ]*" nil t nil)
	;;(add-to-list 'labels (cons (match-string 1) (point))))
	;; this is faster:
	(setq labels (nconc labels (list (cons (match-string 1) (point))))))
      labels)))

(defun gtypist-mode-in-drill-text-p(pos)
  "Find out whether pos is in drill-text. This changes the match data."
  (let ((temp))
    (save-excursion
      (goto-char pos)
      (if (not (re-search-backward "^\\([A-Z]:\\)"))
	  (error "%s: syntactic error" (what-line)))
      (setq temp (match-string 1))
      (if (string-match "^[OPD]:$" temp)
	  t
	nil))))

(defun gtypist-mode-font-lock-find-drill-text(limit)
  "This is used to color drill-text in font-lock-mode."
  (let ((result)
	(done nil)
 	(match-data))
    (while (not done)
      (setq result (re-search-forward "^[OPD ]:\\(.*\\)" limit t))
      (setq match-data (match-data))
      ;; gtypist-mode-in-drill-text-p changes the match data
      (if (or (gtypist-mode-in-drill-text-p (point))
 	      (null result))
 	  (setq done t))
      (set-match-data match-data)
      )
    result))


(provide 'gtypist-mode)

;;; gtypist-mode.el ends here
