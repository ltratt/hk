# hk: Set temporary X11 hotkeys

## Overview

`hk` allows you to set one-off hotkeys at the command-line. Its usage is:

```sh
hk [-hw] <hotkey> <cmd> [<cmdarg1> ... <cmdargn>]
```

where `<hotkey>` is of the form `[Modifier1[+Modifier2[+...]]+]<Key>`. For example:

```sh
hk Ctrl+Shift+F6 notify-send "Hello"
```

will execute the command `notify-send "Hello`" when `Ctrl+Shift+F6` is pressed.
`hk` exits as soon as the command has executed.

`hk` passes through stdin unchanged so you can pipe input to commands e.g.:

```sh
uname | hk Ctrl+F8 xargs notify-send
```

By default, `hk` executes the command as soon as the hotkey sequence is
pressed. The `-w` option makes `hk` wait until all keys (not just the hotkeys)
have been released. This can be useful if the action of the command can be
effected by key presses. For example if you use `xdotool type` to enter text on
a hotkey, then having the `Ctrl` key held down can have surprising effects
which `-w` can alleviate:

```sh
uname | hk -w Ctrl+F8 xdotool type --file -
```


## Install

```sh
$ autoconf
$ ./configure
$ make
```
