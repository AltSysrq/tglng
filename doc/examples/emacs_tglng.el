; Example integration of TglNG into Emacs.
; Tested on: GNU Emacs 24

(defun invoke-tglng-on-region ()
  "Invokes tglng on the current region, or the current line if the
region is not in use."
  (interactive)
  ; First, kill the error buffer if it exists, so that we can detect failure of
  ; the inferior tglng process can be observed
  (let ((error-buff (get-buffer "* TglNG Errors *")))
    (when error-buff (kill-buffer error-buff)))

  (let ((begin
         (if (use-region-p) (region-beginning)
           (save-excursion
             (beginning-of-line)
             (point))))
        (end
         (if (use-region-p) (region-end)
           (save-excursion
             (end-of-line)
             (point))))
        (source-buff (current-buffer)))
    (with-temp-buffer
      (let ((temp-buff (current-buffer)))
        (with-current-buffer source-buff
          (shell-command-on-region begin end
                                   (concat "tglng -lf "
                                           (shell-quote-argument
                                            (or (buffer-file-name)
                                                "")))
                                   temp-buff nil
                                   "* TglNG Errors *" t)
          ; Check for errors
          (let ((error-buff (get-buffer "* TglNG Errors *")))
            (if (and error-buff
                     (< 0 (length (with-current-buffer error-buff
                                    (buffer-string)))))
                ; Error occurred, move point to input at offset indicated by
                ; integer on standard output
                (progn
                  (goto-char
                   (+ begin
                      (string-to-number
                       (with-current-buffer temp-buff
                         (buffer-string)))))
                  nil)
              ; Success, replace region with output
              (kill-region begin end)
              (insert (with-current-buffer temp-buff
                        (buffer-string)))
              t)))))))

(defun smart-invoke-tglng-and-newline-and-indent ()
  "Call invoke-tglng-on-region, then either indent (if at BOL) or
newline-and-indent otherwise."
  (interactive)
  (when (invoke-tglng-on-region)
    (if (= (point)
           (save-excursion (beginning-of-line) (point)))
        (indent-according-to-mode)
      (newline-and-indent))))

; Bind smart-invoke-tglng-and-newline-and-indent to C-M-j
(global-set-key "\e\C-j" 'smart-invoke-tglng-and-newline-and-indent)

; Some modes (like the c-mode family) rebind M-j and C-M-j back to the same
; thing. Prevent that from happening.
(add-hook 'after-change-major-mode-hook
          (lambda () (local-unset-key "\e\C-j")))
