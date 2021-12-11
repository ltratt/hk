# hk: Set temporary X11 hotkeys

## Overview

`hk` allows you to set one-off hotkeys at the command-line. Its usage is:

```sh
hk [-h] <hotkey> <cmd> [<cmdarg1> ... <cmdargn>]
```

where `<hotkey>` is of the form `[Modifier1[+Modifier2[+...]]+]<Key>`. For example:

```sh
hk Ctrl+Shift+F6 notify-send "Hello"
```

will execute the command `notify-send "Hello`" when `Ctrl+Shift+F6` is pressed.
`hk` exits as soon as the command has executed.

`hk` passes through stdin unchanged so you can pipe input to commands e.g.:

```sh
uname | hk Ctrl+F8 sh -c "sleep 0.5 && xdotool type --file -"
```

Pressing `Ctrl+F8` will then "type" text into the current X11 window (notice
that the `sleep` is so that the `Ctrl` key is still not pressed when `xdotool`
starts typing).


## Install

```sh
$ autoconf
$ ./configure
$ make
```
