;; stupid reason for macro, shutup
(defvar *manifest*)
(defmacro manifest-data (&rest data-args)
  `(setf *manifest* (list
                     ,@(loop for item in data-args collect
	                     `(list ,(first item)
                                    ,(second item)
                                    ,(third item))))))

(load "src/entity_asset_list.txt")

;; This is technically bad, but since I load in the file, they should be interned in the same package.
(defun compress (triplet)
  (list (first triplet) (third triplet)))

(defun generate-sounds-fn (sounds)
  (format t "// ~a sounds?~%" (length sounds))
  (loop for sound in sounds do
        (format t "local sound_id ~a = {};~%" (string-downcase (symbol-name (first sound)))))
  (format t "local void initialize_entity_audio_assets(void) {~%")
  (loop for sound in sounds do
        (format t "~a = load_sound(\"~a\");~%" (string-downcase (symbol-name (first sound))) (second sound)))
  (format t "}~%"))

(defun generate-textures-fn (textures)
  (format t "// ~a textures?~%" (length textures))
  (loop for texture in textures do
        (format t "local texture_id ~a = {};~%" (string-downcase (symbol-name (first texture)))))
  (format t "local void initialize_entity_graphics_assets(void) {~%")
  (loop for texture in textures do
        (format t "~a = load_texture(\"~a\");~%" (string-downcase (symbol-name (first texture))) (second texture)))
  (format t "}~%"))

(with-open-file (*standard-output* "src/generated_initialize_entity_assets.c"
                                   :direction :output
                                   :if-exists :supersede
                                   :external-format :utf8)
  (let ((textures (remove-if-not (lambda (item) (equal (second item) :texture)) *manifest*))
        (sounds   (remove-if-not (lambda (item) (equal (second item) :sound)) *manifest*)))
    (format t "/*~%")
    (format t "~a~%" *manifest*)
    (format t "~a~%" textures)
    (format t "~a~%" sounds)
    (format t "*/~%")
    (generate-textures-fn (map 'list #'compress textures))
    (generate-sounds-fn (map 'list #'compress sounds))))
