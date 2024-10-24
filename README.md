# TCLibs

**License: Dual-licensed [CC0](https://creativecommons.org/publicdomain/zero/1.0/) and [Boost](http://www.boost.org/users/license.html)** (pick one)

A set of single-file public domain libraries done in the spirit of the [stb libraries](https://github.com/nothings/stb).

Note that some of the libraries have dependencies. For a quick reference see the "`DEPENDS:`" line near the top of each file.

The following libraries are currently available.

| Library            | Version | Description                                                                   | Dependencies                       |
|:-------------------|:--------|:------------------------------------------------------------------------------|:-----------------------------------|
| tc_string.h        | -.-.-   | String utility functions. *(may be removed if merged)*                        |                                    |
| tc_terminal.h      | -.-.-   | System terminal (console) abstraction layer (colors, cursor, ...).            |                                    |
| tc_history.h       | -.-.-   | Line history handling; independent of system terminal.                        | tc_string                          |
| tc_editline.h      | -.-.-   | Terminal line input with history handling.                                    | tc_terminal tc_history / tc_string |
| tc_random.h        | -.-.-   | Random number generation. *(very unstable API)*                               |                                    |
| tc_hash.h          | -.-.-   | Cryptographic hash function library.                                          |                                    |
| tc_texture_load.h  | -.-.-   | Texture loading (currently only DDS).                                         |                                    |
| tc_texture_codec.h | -.-.-   | Texture block (de)compression (currently only decompressors).                 |                                    |
| tc_thread.h        | -.-.-   | Threading &amp; atomics (atomics, threads, mutexes, condition variables, ...) |                                    |
| tc_vox.h           | 0.2.1   | [MagicaVoxel](https://ephtracy.github.io/) `*.vox` loading library.           |                                    |
| tc_xml.h           | 0.1.0   | XML parsing (for now only *mostly* compliant).                                |                                    |

Target OSes are Windows, Linux, FreeBSD and Mac OS X. Note that I do not currently have access to OS X, so the code might be buggier than usual.

## Planned and WiP Libraries:

- `tc_vfs.h` (WiP): Virtualized filesystem with directory+zip support (will either use [miniz](https://github.com/richgel999/miniz), or [because miniz has issues and is unmaintained] a custom `tc_[un]zip.h`).

## Screenshots:

Editline:
![editline](screenshots/editline.png)
