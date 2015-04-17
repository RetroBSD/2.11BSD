;;  %Z%%M% %I% %E%
;;
;;  From Roger Riggs <riggs@suneast.East.Sun.COM>
;;
;;  I and other people here have created functions to print with mp.
;;  This is particularly good with mail and newsgroup articles
;;  and the PostScript page decorations can be tailored if you know a
;;  little PostScript. Both are variations of the original lpr and print
;;  buffer/region code.
;;
;;  Interface to mp.

(defun mp-buffer ()
  "Print buffer contents as with Unix command `mp | lpr -h'.
`lpr-switches' is a list of extra switches (strings) to pass to lpr."
  (interactive)
  (mp-region-1 (point-min) (point-max)))

(defun mp-region (start end)
  "Print region contents as with Unix command `mp | lpr -h'.
`lpr-switches' is a list of extra switches (strings) to pass to lpr."
  (interactive "r")
  (mp-region-1 start end))

(defun mp-region-1 (start end)
  "Print region using mp to lpr"
  (interactive)
  (let ((mp-switches) (msg "Formatting...") (oldbuf (current-buffer)))
    (cond ((string-equal major-mode 'gnus-Article-mode)
	   (if (and (= (point-min) start) (= (point-max) end))
	       (setq mp-switches (list "-a"))
	     (setq mp-switches (list "-o" "-s" (mail-fetch-field "Subject")))))
	  ((string-equal major-mode 'rmail-mode)
	   (if (and (= (point-min) start) (= (point-max) end))
	       (setq mp-switches nil)
	     (setq mp-switches (list "-o" "-s" (mail-fetch-field "Subject")))))
	  (t 
	   (setq mp-switches
		 (list "-o" "-s" (concat "\"" (buffer-name) " Emacs buffer" "\"")))))
    (save-excursion
      (message "%s" msg)
      (set-buffer (get-buffer-create "*spool temp*"))
      (widen) (erase-buffer)
      (insert-buffer-substring oldbuf start end)
      (if (/= tab-width 8)
	  (progn
	    (setq msg (concat msg " tabs..."))
	    (message "%s" msg)
	    (setq tab-width tab-width)
	    (untabify (point-min) (point-max))))
      
      (setq msg (concat msg " mp..."))
      (message "%s" msg)
      (apply 'call-process-region
	     (nconc (list (point-min) (point-max) "mp" t t nil )
		    mp-switches))
      
      (setq msg (concat msg " lpr..."))
      (message "%s" msg)
      (apply 'call-process-region
	     (nconc (list (point-min) (point-max) "lpr" t nil nil "-h" )
		    lpr-switches))
      (setq msg (concat msg " done."))
      (message "%s" msg))))
