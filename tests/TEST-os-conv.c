#include <locale.h>
#include <iconv.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <stdint.h>

#include <uchar.h>

static uint16_t swap16(uint16_t x)
{
    return (x >> 8) | (x << 8);
}
static uint32_t swap32(uint32_t x)
{
    return (((uint32_t)swap16(x)) << 16) | swap16(x >> 16);
}

void dump_wstr(const wchar_t* wbuf, int num)
{
    if(num < 0) num = wcslen(wbuf);

    printf("%d\n", num);
    int i;
    for(i = 0; i < num; i++)
        printf("%8X ", wbuf[i]);
    printf("\n");
}
size_t len_str16(const char16_t* buf16)
{
    size_t len;
    for(len = 0; buf16[len]; len++) {}
    return len;
}
void dump_str16(const char16_t* buf16, int num)
{
    if(num < 0) num = len_str16(buf16);

    printf("%d\n", num);
    int i;
    for(i = 0; i < num; i++)
        printf("%4X ", buf16[i]);
    printf("\n");
}
void print_towcs(const char* buf, int end)
{
    printf("=== mbsrtowcs(3) ===\n");

    if(end < 0) end = strlen(buf);

    wchar_t wbuf[512];

    const char* ptr = buf;

    mbstate_t state = { };
    //size_t num = mbrtowc(wbuf, buf, end, &state);
    size_t num = mbsrtowcs(wbuf, &ptr, end, &state);

    dump_wstr(wbuf, num);
}
void print_iconv(const char* buf, int end)
{
    printf("=== iconv(3) ===\n");

    if(end < 0) end = strlen(buf);

    iconv_t icp = iconv_open("UTF-32", "");
    if(!icp)
    {
        perror("Unable to iconv_open()");
        return;
    }

    wchar_t wbuf[512];
    wchar_t* wptr = wbuf;
    size_t wleft = sizeof(wbuf);

    const char* ptr = buf;
    size_t left = end;
    size_t num = iconv(icp, (char**)&ptr, &left, (char**)&wptr, &wleft);

    if(num == (size_t)-1)
    {
        printf("Near `%s`\n", ptr);
        perror("Error Converting");
        return;
    }

    size_t i;
    for(i = 1; i <= wptr - wbuf; i++)
        wbuf[i] = swap32(wbuf[i]);

    dump_wstr(wbuf + 1, wptr - wbuf - 1);

    iconv_close(icp);
}
void print_char16(const char* buf, int end)
{
    printf("=== mbrtoc16(3) ===\n");

#ifndef __STDC_UTF_16__
    printf("(__STDC_UTF_16__ is not defined; aborting)\n");
    return;
#endif
    if(end < 0) end = strlen(buf);

    char16_t buf16[512];
    size_t len16 = 0;

    mbstate_t state = {};

    int i = 0;
    while(i < end)
    {
        size_t ret = mbrtoc16(&buf16[len16], &buf[i], end - i, &state);
        while(ret == (size_t)-3)
        {
            len16++;
            ret = mbrtoc16(&buf16[len16], &buf[i], end - i, &state);
        }
        if(ret == (size_t)-2)
        {
            fprintf(stderr, "Warning: Incomplete character!\n");
            i = end;
        }
        else if(ret == (size_t)-1)
        {
            perror("Error Converting");
            return;
        }
        else
        {
            len16++;
            i += ret;
        }
    }
    buf16[len16] = 0;

    dump_str16(buf16, len16);
}
void print_char32(const char* buf, int end)
{
    printf("=== mbrtoc32(3) ===\n");

#ifndef __STDC_UTF_32__
    printf("(__STDC_UTF_32__ is not defined; aborting)\n");
    return;
#endif
    if(end < 0) end = strlen(buf);

    char32_t buf32[512];
    size_t len32 = 0;

    mbstate_t state = {};

    int i = 0;
    while(i < end)
    {
        size_t ret = mbrtoc32(&buf32[len32], &buf[i], end - i, &state);
        while(ret == (size_t)-3)
        {
            len32++;
            ret = mbrtoc32(&buf32[len32], &buf[i], end - i, &state);
        }
        if(ret == (size_t)-2)
        {
            fprintf(stderr, "Warning: Incomplete character!\n");
            i = end;
        }
        else if(ret == (size_t)-1)
        {
            perror("Error Converting");
            return;
        }
        else
        {
            len32++;
            i += ret;
        }
    }
    buf32[len32] = 0;

    dump_wstr((const wchar_t*)buf32, len32);
}

int main(void)
{
    locale_t oloc = uselocale((locale_t)0);
    locale_t nloc = newlocale(LC_CTYPE_MASK, "", (locale_t)0);
    locale_t xloc = uselocale(nloc);

    setlocale(LC_CTYPE, "");

    char buf[512] = "s" "\xC8\xA9\xAF" "t";
    //fgets(buf, sizeof(buf), stdin);

    size_t end = strcspn(buf, "\r\n");
    buf[end] = 0;

    print_towcs(buf, end);
    print_iconv(buf, end);
    print_char16(buf, end);
    print_char32(buf, end);

    return 0;
}
