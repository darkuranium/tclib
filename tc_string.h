/*
 * tc_string.h: TC_String implementation & handling.
 *
 * DEPENDS:
 * VERSION: 0.0.2 (2016-09-08)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Cas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.0.2    made the library compile with a C++ compiler
 * 0.0.1    initial public release
 *
 * TODOs:
 * - add tests
 * - implement char<->UTF-8 conversion (TBD: should this be part of the console lib instead?)
 * - implement tcstr_utf8_find_offset
 * - add an option for custom malloc(), realloc(), free(), memcpy(), memmove()
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
/*size_t tcstr_utf8_find_offset(const TC_String* str, size_t chr);*/
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
        str->ptr = TC__VOID_CAST(char*,malloc(src->len + 1));
        memcpy(str->ptr, src->ptr, src->len);
        str->ptr[src->len] = 0;
    }
    else
    {
        str->len = 0;
        str->ptr = TC__VOID_CAST(char*,malloc(str->len + 1));
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
        str->ptr = TC__VOID_CAST(char*,malloc(len + 1));
        memcpy(str->ptr, ptr, len);
        str->ptr[len] = 0;
    }
    else if(len >= 0)
    {
        str->len = len;
        str->ptr = TC__VOID_CAST(char*,malloc(len + 1));
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
        free(str->ptr);
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
    memmove(str->ptr + pos + src->len, str->ptr + pos + del, nlen - pos - src->len);
    if(nlen > str->len)
        str->ptr = TC__VOID_CAST(char*,realloc(str->ptr, nlen + 1));
    // 01234---789%
    memmove(str->ptr + pos, src->ptr, src->len);
    if(nlen < str->len)
        str->ptr = TC__VOID_CAST(char*,realloc(str->ptr, nlen + 1));
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
/*static size_t mhINT_utf8_find_offset(const MH_String* str, size_t uidx)
{
    size_t i;
    for(i = 0; i < str->len; i++)
    {
        if(!uidx)
            break;
        if(mhINT_utf8_is_sync(str->ptr[i]))
            uidx--;
    }
    return i;
}*/
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
