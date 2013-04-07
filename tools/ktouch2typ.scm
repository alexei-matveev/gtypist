#!/usr/bin/guile -s
!#

(use-modules (sxml simple)
             (sxml transform)
             (sxml match)
             (ice-9 pretty-print))

;;;
;;; a) KTouch XML as input:
;;;
;;; (*TOP* (*PI* xml "version=\"1.0\" encoding=\"UTF-8\"")
;;;        (KTouchLecture
;;;          (Title "German (auto-generated)")
;;;          (Comment "German ...")
;;;          (FontSuggestions "Courier 10 Pitch")
;;;          (Levels
;;;            (Level (LevelComment "Grundstellung Teil 1")
;;;                   (NewCharsLabel "Grundstellung (1)")
;;;                   (NewCharacters "asdfjklöä")
;;;                   (Line "fff ...")
;;;                   (Line "fjf ...")
;;;                   (Line "ddd ...")
;;;                   (Line "fff ..."))
;;;            ...))
;;;        ...)
;;;
;;; Unfortunately, KTouch is not very strict with its schema.  I guess
;;; LevelComment and NewCharacters are optional.
;;;
;;; b) Nested "SXML" as intermediate and lossy Gtypist representation:
;;;
;;; ((Lecture
;;;    (Title "German (auto-generated)")
;;;    (Comment "German ...")
;;;    (Levels
;;;      (Lesson
;;;        (Banner "Grundstellung (1)")
;;;        (Lines "fff ..."
;;;               "fjf ..."
;;;               "ddd ..."
;;;               "fff ..."))
;;;      ...))
;;;  ...)
;;;
;;;
;;;
;;; Outputs the (Lesson (Banner banner) (Lines line ...)) structure:
;;;
(define (level->lesson sxml)
  (sxml-match sxml
    [(Level (LevelComment ,comment)
            (NewCharsLabel ,label)
            (NewCharacters ,characters)
            (Line ,line) ...)
     `(Lesson (Banner ,label)
              (Lines ,line ...))]
    [(Level (NewCharsLabel ,label)
            (NewCharacters ,characters)
            (Line ,line) ...)
     `(Lesson (Banner ,label)
              (Lines ,line ...))]
    [(Level (LevelComment ,comment)
            (NewCharsLabel ,label)
            (Line ,line) ...)
     `(Lesson (Banner ,label)
              (Lines ,line ...))]
    [(Level (NewCharsLabel ,label)
            (Line ,line) ...)
     `(Lesson (Banner ,label)
              (Lines ,line ...))]))
;;;
;;; The B: line, centered banner on every page:
;;;
(define (write-banner banner)
  (display "B:")
  (let ((n (string-length banner))
        (w 33))        ; FIXME: literal 33, so the original gtypist.pm
    (display (string-pad banner (+ w (- n (quotient n 2))))))
  (newline))

;;;
;;; A go-to label,  K12 (?) escape, followed by  the lesson data split
;;; into a few lines per page, finally go-to to the jumb table:
;;;
(define (write-lesson lesson-count lesson)
  (sxml-match lesson
    [(Lesson (Banner ,banner)
             (Lines . ,lines))
     (begin
       (format #t "*:S_LESSON~a\n" lesson-count)
       (format #t "K:12:MENU\n")        ; FIXME: literal 12?
       (write-banner (string-append (format #f "Lesson ~a: " lesson-count)
                                    banner))
       (write-lines lesson-count lines)
       (format #t "G:E_LESSON~a\n\n" lesson-count))]))

(define *lines-per-page* 4)             ; FIXME: literal 4

;;;
;;; Here is the logic of splitting a lesson into pages:
;;;
(define (write-lines lesson-count lines)
  (let loop ((count 0)
             (lines lines))
    (if (not (null? lines))
        (begin
          (let ((q (quotient count *lines-per-page*))
                (r (modulo count *lines-per-page*)))
            (if (zero? r)
                (begin
                  (format #t "*:LESSON~a_D~a\n" lesson-count (1+ q))
                  (format #t "I:(~a)\n" (1+ q))
                  (display "D:"))
                (display " :")))
          (display (car lines))
          (newline)
          (loop (1+ count)
                (cdr lines))))))

;;;
;;; Annihilated SXML nodes leave () holes, this is used to prune them:
;;;
(define (prune rest)
  (filter (lambda (x) (not (null? x)))
          rest))

;;;
;;; Code is  copied from SSAX.scm.  Test if a  string is made  of only
;;; whitespace.  An  empty string is considered made  of whitespace as
;;; well.   FIXME: see if  your version  of Guile  supports (xml->sxml
;;; #:trim-whitespace? #t).
;;;
(define (whitespace? str)
  (and (string? str) ; for some reason xml in <?xml ...> is treated as *text*
       (let ((len (string-length str)))
         (cond
          ((zero? len)
           #t)
          ((= 1 len)
           (char-whitespace? (string-ref str 0)))
          ((= 2 len)
           (and (char-whitespace? (string-ref str 0))
                (char-whitespace? (string-ref str 1))))
          (else
           (let loop ((i 0))
             (or (>= i len)
                 (and (char-whitespace? (string-ref str i))
                      (loop (+ 1 i))))))))))

;;;
;;; This strips all *text* nodes  that are whitespace. FIXME: not very
;;; delicate ...
;;;
(define (strip-space sxml)
  ;; for some reason xml in <?xml ...> is treated as *text*
  (let ((bindings
         `((*PI*	*preorder*	. ,(lambda node node))
           (*text*			. ,(lambda (tag str)
                                             (if (whitespace? str) '() str)))
           (*default*			. ,(lambda node (prune node))))))
    (pre-post-order sxml bindings)))

;;;
;;; Result  is  not  XML,  just   a  nested  structure.  So  drop  the
;;; boilerplate:
;;;
(define (ktouch->gtypist sxml)
  (let ((bindings
         `((*TOP*			. ,(lambda (tag . body) (prune body)))
           (*PI*			. ,(lambda node '())) ;  kid of *TOP*
           (KTouchLecture		. ,(lambda (tag . body) (cons 'Lecture (prune body))))
           (FontSuggestions		. ,(lambda node '())) ; kid of KTouchLecture
           (Level	*preorder*	. ,(lambda node (level->lesson node))) ; <- magic here
           (*text*			. ,(lambda (tag str) str))
           (*default*			. ,(lambda node node)))))
    (pre-post-order sxml bindings)))

;;;
;;; Flow control of the lesson here:
;;;
(define (write-jump-table lesson-count)
  (format #t "# jump-table\n")
  (let loop ((lesson 1))
    (format #t "*:E_LESSON~a\n" lesson)
    (if (< lesson lesson-count)
        (begin
          (format #t "Q: Do you want to continue to lesson ~a [Y/N] ?\n" (1+ lesson))
          (format #t "N:MENU\nG:S_LESSON~a\n" (1+ lesson))
          (loop (1+ lesson)))
        (format #t "G:MENU\n"))))

;;;
;;; Initial menu as presented to the user here:
;;;
(define (write-menu lecture-title lesson-banners)
  (format #t "\n*:MENU\n")
  (format #t "M: \"~a\"\n" lecture-title)
  (let loop ((i 1)
             (banners lesson-banners))
    (if (not (null? banners))
        (begin
          (format #t " :S_LESSON~a \"~a\"\n" i (car banners))
          (loop (1+ i)
                (cdr banners))))))

;;;
;;; A "getter":
;;;
(define (lesson-banner lesson)
  (sxml-match lesson
    [(Lesson (Banner ,banner)
             (Lines . ,lines))
     banner]))

;;;
;;; Go to menu, then actual lessons, then jump table, and, finally the
;;; menu:
;;;
(define (write-lecture lecture)
  (sxml-match lecture
    [(Lecture (Title ,title)
              (Comment ,comment)
              (Levels . ,lessons))
     (begin
       (format #t "G:MENU\n\n")
       (let loop ((i 1)
                  (lessons lessons))
         (if (not (null? lessons))
             (begin
               (write-lesson i (car lessons))
               (loop (1+ i)
                     (cdr lessons)))))
       (write-jump-table (length lessons))
       (write-menu title (map lesson-banner lessons)))]))

;;;
;;; Reads from stdin ...
;;;
(let ((sxml (xml->sxml)))
  ;; (pretty-print (strip-space sxml))
  (let ((lectures (ktouch->gtypist (strip-space sxml)))) ; always one?
    ;; (pretty-print lectures)
    (for-each write-lecture lectures)))
