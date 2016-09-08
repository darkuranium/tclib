# TCLibs

**License: Dual-licensed [CC0](https://creativecommons.org/publicdomain/zero/1.0/) and [Boost](http://www.boost.org/users/license.html)** (pick one)

A set of single-file public domain libraries done in the spirit of the [stb libraries](https://github.com/nothings/stb).

Note that some of the libraries have dependencies. For a quick reference see the "`DEPENDS:`" line near the top of each file.

The following libraries are currently available (*all are currently **alpha version***). The prefix ("`tc_`") is subject to change until stable release.

| Library       | Version | Description                                                       | Dependencies                       |
|:--------------|:--------|:------------------------------------------------------------------|:-----------------------------------|
| tc_string.h   | 0.0.1   | String utility functions *(may be removed if merged)*             |                                    |
| tc_terminal.h | 0.0.1   | System terminal (console) abstraction layer (colors, cursor, ...) |                                    |
| tc_history.h  | 0.0.1   | Line history handling; independent of system terminal.            | tc_string                          |
| tc_editline.h | 0.0.1   | Terminal line input with history handling.                        | tc_terminal tc_history / tc_string |

Target OSes are Windows, Linux, FreeBSD and Mac OS X. Note that I do not currently have access to OS X, so the code might be buggier than usual.

## Planned and WiP Libraries:

- `tc_vfs.h` (WiP): Virtualized filesystem with directory+zip support (will either use [miniz](https://github.com/richgel999/miniz), or [because miniz has issues and is unmaintained] a custom `tc_[un]zip.h`).

## TODOs:

- ***All:***
    - ***document!***
    - add tests (where appropriate)

- `tc_string.h`:
    - implement char<->UTF-8 conversion (TBD: should this be part of `tc_terminal` instead?)
    - implement `tcstr_utf8_find_offset()`
    - add an option for custom `malloc()`, `realloc()`, `free()`, `memcpy()`, `memmove()`, ...
- `tc_terminal.h`:
    - add `TCTERM_{FG,BG}_DEFAULT` since the default color is distinct in POSIX ("`\e[39m`" or "`\e[0m`")
    - add proper UTF-8 support for non-"`.UTF-8`" locales
    - (opt?) add support for bold/bright in POSIX
    - add proper checks for terminal capability (specifically, colors) and don't output them if !isatty()
- `tc_history.h`:
    - read history from file or stream
    - multiline support
- `tc_editline.h`:
    - multiline support
    - callback(s) for tab key completion and hints
		- proposed event handler:
          ```c
          typedef struct TC_EditEvent {
              int type;
              TC_String* input;
              size_t byte_pos, char_pos;
          } TC_EditEvent;
          int (*)(TC_EditEvent* event);
          ```

## Screenshots:

Editline:\
![editline](screenshots/editline.png)
