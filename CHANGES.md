# 0.3.1 (2023-12-13)

* Add `-v` switch to print out which keys remain held after the hotkey has been
  pressed when `-w` is specified.


# 0.3.0 (2023-01-03)

* Ignore the Mode Switch key (similarly to Caps Lock et al.).

* Ignored modifiers are now also ignored by`-w`. On some setups, some modifiers
  are permanently set as "pressed" (in some cases, without the user being aware
  of it), meaning that they cause `-w` to permanently block unless they are
  ignored.



# 0.2.0 (2022-08-20)

* Various minor bug fixes, mostly around explicitly reporting error conditions
  (e.g. cannot open X display).

* Abort if the user repeats a modifier or key (e.g. Ctrl+F1+F1 now fails with
  `Repeated key 'F1'`) as these are certainly unintended and, probably, a
  source of error.


# 0.1.0 (2021-12-20)

* Initial release.
