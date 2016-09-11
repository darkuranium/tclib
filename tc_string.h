/*
 * tc_string.h: TC_String implementation & handling.
 *
 * DEPENDS:
 * VERSION: 0.0.3 (2016-09-11)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Cas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.0.3    added TC_MALLOC, TC_MEMCPY and friends
 *          added tcstr_utf8_find_offset()
 * 0.0.2    made the library compile with a C++ compiler
 * 0.0.1    initial public release
 *
 *
 *
 * This library contains some common string handling functions.
 *
 * A single file should contain the following `#define` before including the header:
 *  ```c
 *  #define TC_STRING_IMPLEMENTATION
 *  #include "tc_string.h"
 *  ```
 *
 * It is primarily (but not exclusively) meant for internal use by other TCLib libraries;
 * as such, some of the features might seem out of place, while others might be missing.
 */

/* ========== API ==========
 *
 * SYNOPSIS:
 *  TC_String* tcstr_init(TC_String* str, const TC_String* src);
 *  TC_String* tcstr_inits(TC_String* str, const char* ptr, int len);
 * PARAMETERS:
 *  - str: string being initialized
 *  - src,ptr: source string being copied
 *  - len: length of source string (in bytes), or `-1` to determine automatically (via `strlen(ptr)`)
 * RETURN VALUE:
 *  `str` if successful, else `NULL`.
 * DESCRIPTION:
 *  Initialize `str` with the value from `src` or `ptr`/`len`.
 *
 *  Strings are always `0`-terminated for convenience, with the exception of null strings (those with `.ptr=NULL`).
 *
 *  - If `src != NULL`, then initialize `str` with its contents; else set `str` to the null string.
 *  - If `ptr != NULL`, then initialize `str` with its contents of length `len` or `strlen(ptr)`.
 *  - If `ptr == NULL`:
 *      - if `len >= 0`, allocate an uninitialized string of length `len`;
 *      - if `len < 0`, set `str` to the null string.
 *
 *
 * SYNOPSIS:
 *  void tcstr_deinit(TC_String* str);
 * PARAMETERS:
 *  - str: string being deinitialized
 * DESCRIPTION:
 *  Free the data contained by `str`.
 *
 *  It should not be used anymore, except with the `tcstr_init` family of functions.
 *
 *
 * SYNOPSIS:
 *  TC_String* tcstr_reinit(TC_String* str, const TC_String* src);
 *  TC_String* tcstr_reinits(TC_String* str, const char* ptr, int len);
 * PARAMETERS:
 *  Same as in `tcstr_init`.
 * RETURN VALUE:
 *  `str` if successful, else `NULL`.
 * DESCRIPTION:
 *  Reinitialize a string, freeing old data.
 *
 *  This is the same as calling `tcstr_deinit(str); tcstr_init(str, src);`
 *
 *
 * SYNOPSIS:
 *  TC_String* tcstr_splice(TC_String* str, size_t pos, size_t del, const TC_String* src);
 *  TC_String* tcstr_splices(TC_String* str, size_t pos, size_t del, const char* ptr, int len);
 * PARAMETERS:
 *  - str: string being spliced (in-place)
 *  - pos: starting position (byte offset) of the splice
 *  - del: number of characters to delete, in bytes
 *  - src, ptr, len: mostly the same as in `tcstr_init`; NULL values are the same as empty strings.
 * RETURN VALUE:
 *  `str` if successful, else `NULL`.
 * DESCRIPTION:
 *  `del` bytes are removed from `str` starting at `pos`, and then `src` or `ptr`/`len` is inserted at the same position.
 *
 *  Both `pos` and `del` are truncated to the end of `str` if necessary.
 * EXAMPLES:
 *  ```c
 *  // str = { .ptr="0123456789", .len=10 }
 *  //                    ^ pos
 *  //                       ^ pos+del
 *  tcstr_splice(&str, 5, 3, "abcde", 5);
 *  // str.ptr is now "01234abcde89"
 *  ```
 *  ```c
 *  // str = { .ptr="hello, world", .len=12 }
 *  tcstr_splice(&str, 2, 3, NULL, 0);
 *  // str.ptr is now "he, world"
 *  ```
 *  ```c
 *  // str = { .ptr="hello, world", .len=12 }
 *  tcstr_splice(&str, 7, 0, "abc ", -1);
 *  // str.ptr is now "hello, abc world"
 *  ```
 *
 *
 * SYNOPSIS:
 *  size_t tcstr_utf8_find_offset(const TC_String* str, size_t chr);
 *  size_t tcstr_utf8_find_char(const TC_String* str, size_t off);
 * PARAMETERS:
 *  - str: operand string
 *  - chr: code point offset
 *  - off: byte offset
 * RETURN VALUE:
 *  - `tcstr_utf8_find_offset` returns the byte offset for `chr`-th Unicode code point.
 *  - `tcstr_utf8_find_char` does the opposite, returning the number of code points leading up to a specific byte offset.
 * DESCRIPTION:
 *  Find the byte offset to the start of a specific Unicode code point, or vice-versa.
 *
 *
 * SYNOPSIS:
 *  size_t tcstr_utf8_prev_off(const TC_String* str, size_t off);
 *  size_t tcstr_utf8_next_off(const TC_String* str, size_t off);
 * PARAMETERS:
 *  - str: operand string
 *  - off: byte offset
 * RETURN VALUE:
 *  Start of the found character, 0 if no previous character exists, or `str->len` if no next character exists.
 * DESCRIPTION:
 *  Find the start byte offset of the next or previous Unicode character in the string.
 */

#ifndef TC_STRING_H_
#define TC_STRING_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TC_String
{
    size_t len;
    char* ptr;
} TC_String;

TC_String* tcstr_init(TC_String* str, const TC_String* src);
TC_String* tcstr_inits(TC_String* str, const char* ptr, int len);
void tcstr_deinit(TC_String* str);
TC_String* tcstr_reinit(TC_String* str, const TC_String* src);
TC_String* tcstr_reinits(TC_String* str, const char* ptr, int len);

TC_String* tcstr_splice(TC_String* str, size_t pos, size_t del, const TC_String* src);
TC_String* tcstr_splices(TC_String* str, size_t pos, size_t del, const char* ptr, int len);

/* unicode index -> string offset */
size_t tcstr_utf8_find_offset(const TC_String* str, size_t chr);
/* string offset -> unicode index */
size_t tcstr_utf8_find_char(const TC_String* str, size_t off);
size_t tcstr_utf8_prev_off(const TC_String* str, size_t off);
size_t tcstr_utf8_next_off(const TC_String* str, size_t off);

#ifdef __cplusplus
}
#endif

#endif /* TC_STRING_H_ */

#ifdef TC_STRING_IMPLEMENTATION
#undef TC_STRING_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>

#ifndef TC__STATIC_CAST
#ifdef __cplusplus
#define TC__STATIC_CAST(T,v) static_cast<T>(v)
#else
#define TC__STATIC_CAST(T,v) ((T)(v))
#endif
#endif /* TC__STATIC_CAST */

/* no cast done to preserve undefined function warnings in C */
#ifndef TC__VOID_CAST
#ifdef __cplusplus
#define TC__VOID_CAST(T,v)  TC__STATIC_CAST(T,v)
#else
#define TC__VOID_CAST(T,v)  (v)
#endif
#endif /* TC__VOID_CAST */

#ifndef TC_MALLOC
#define TC_MALLOC(size)         malloc(size)
#endif /* TC_MALLOC */
#ifndef TC_REALLOC
#define TC_REALLOC(ptr,size)    realloc(ptr, size)
#endif /* TC_REALLOC */
#ifndef TC_FREE
#define TC_FREE(ptr)            free(ptr)
#endif /* TC_FREE */

#ifndef TC_MEMCPY
#define TC_MEMCPY(dst,src,len)  memcpy(dst, src, len)
#endif /* TC_MEMCPY */
#ifndef TC_MEMMOVE
#define TC_MEMMOVE(dst,src,len) memmove(dst, src, len)
#endif /* TC_MEMMOVE */

TC_String* tcstr_init(TC_String* str, const TC_String* src)
{
    if(!str) return NULL;
    if(!src)
    {
        str->len = 0;
        str->ptr = NULL;
    }
    else if(src->ptr)
    {
        str->len = src->len;
        str->ptr = TC__VOID_CAST(char*,TC_MALLOC(src->len + 1));
        TC_MEMCPY(str->ptr, src->ptr, src->len);
        str->ptr[src->len] = 0;
    }
    else
    {
        str->len = 0;
        str->ptr = TC__VOID_CAST(char*,TC_MALLOC(str->len + 1));
        str->ptr[str->len] = 0;
    }
    return str;
}
TC_String* tcstr_inits(TC_String* str, const char* ptr, int len)
{
    if(!str) return NULL;
    if(ptr)
    {
        if(len < 0) len = strlen(ptr);

        str->len = len;
        str->ptr = TC__VOID_CAST(char*,TC_MALLOC(len + 1));
        TC_MEMCPY(str->ptr, ptr, len);
        str->ptr[len] = 0;
    }
    else if(len >= 0)
    {
        str->len = len;
        str->ptr = TC__VOID_CAST(char*,TC_MALLOC(len + 1));
        str->ptr[len] = 0;
    }
    else
    {
        str->len = 0;
        str->ptr = NULL;
    }
    return str;
}
void tcstr_deinit(TC_String* str)
{
    if(!str) return;
    if(str->ptr)
        TC_FREE(str->ptr);
}

TC_String* tcstr_reinit(TC_String* str, const TC_String* src)
{
    tcstr_deinit(str);
    return tcstr_init(str, src);
}
TC_String* tcstr_reinits(TC_String* str, const char* ptr, int len)
{
    tcstr_deinit(str);
    return tcstr_inits(str, ptr, len);
}

TC_String* tcstr_splice(TC_String* str, size_t pos, size_t del, const TC_String* src)
{
    if(pos > str->len)
        pos = str->len;
    if(pos + del > str->len)
        del = str->len - pos;

    size_t nlen = str->len + src->len - del;

    // 0123456789       10  str->len
    //      ^           5   pos
    //        ^         2   del
    // abc              3   src->len
    // 01234abc56789    13  mlen
    // 01234abc789      11  nlen

    // 0123456789-%
    //      ^ ^^    P DL
    TC_MEMMOVE(str->ptr + pos + src->len, str->ptr + pos + del, nlen - pos - src->len);
    if(nlen > str->len)
        str->ptr = TC__VOID_CAST(char*,TC_REALLOC(str->ptr, nlen + 1));
    // 01234---789%
    TC_MEMMOVE(str->ptr + pos, src->ptr, src->len);
    if(nlen < str->len)
        str->ptr = TC__VOID_CAST(char*,TC_REALLOC(str->ptr, nlen + 1));
    // 01234abc789%
    str->ptr[nlen] = 0;
    str->len = nlen;

    return str;
}
TC_String* tcstr_splices(TC_String* str, size_t pos, size_t del, const char* ptr, int len)
{
    TC_String src;
    if(len < 0) len = ptr ? strlen(ptr) : 0;
    src.len = len;
    src.ptr = (char*)ptr;
    return tcstr_splice(str, pos, del, &src);
}

static int tcstr__utf8_is_sync(char c)
{
    unsigned char uc = c;
    return !(uc & 0x80) || (uc & 0xC0) == 0xC0;
}
/* unicode index -> string offset */
size_t tcstr_utf8_find_offset(const TC_String* str, size_t uidx)
{
    size_t i;
    for(i = 0; i < str->len; i++)
    {
        if(!uidx)
            break;
        if(tcstr__utf8_is_sync(str->ptr[i]))
            uidx--;
    }
    return i;
}
/* string offset -> unicode index */
size_t tcstr_utf8_find_char(const TC_String* str, size_t off)
{
    size_t uidx = 0;
    size_t i;
    for(i = 0; i < off; i++)
        if(tcstr__utf8_is_sync(str->ptr[i]))
            uidx++;
    return uidx;
}
size_t tcstr_utf8_prev_off(const TC_String* str, size_t off)
{
    if(!off) return off;
    while(off--)
        if(tcstr__utf8_is_sync(str->ptr[off]))
            break;
    return off;
}
size_t tcstr_utf8_next_off(const TC_String* str, size_t off)
{
    if(off >= str->len)
        return off;

    while(++off <= str->len)
        if(tcstr__utf8_is_sync(str->ptr[off]))
            break;
    return off;
}
#endif /* TC_STRING_IMPLEMENTATION */
