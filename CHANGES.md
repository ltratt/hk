# 0.2.0 (2022-08-20)

* Various minor bug fixes, mostly around explicitly reporting error conditions
  (e.g. cannot open X display).

* Abort if the user repeats a modifier or key (e.g. Ctrl+F1+F1 now fails with
  `Repeated key 'F1'`) as these are certainly unintended and, probably, a
  source of error.


# 0.1.0 (2021-12-20)

* Initial release.
