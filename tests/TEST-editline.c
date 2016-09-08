#define TC_STRING_IMPLEMENTATION
#define TC_TERMINAL_IMPLEMENTATION
#define TC_HISTORY_IMPLEMENTATION
#define TC_EDITLINE_IMPLEMENTATION
#include "../tc_string.h"
#include "../tc_terminal.h"
#include "../tc_history.h"
#include "../tc_editline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void vdeinit(void)
{
    //tcterm_deinit();
    tcedit_detach();
}

static int do_echo(TC_String* str, size_t hpos)
{
    tcterm_set_attr(TCTERM_FG_WHITE);

    size_t head;
    size_t tail;
    size_t ident;
    head = strspn(str->ptr, " ");
    tcterm_print(str->ptr, head);
    do
    {
        tail = head + strcspn(str->ptr + head, " ");

        if(tail - head == 3 && !memcmp(str->ptr + head, "var", 3))
            tcterm_set_attr(TCTERM_FG_CYAN);
        else
        {
            ident = head + strspn(str->ptr + head, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_");
            if(ident - head)
            {
                tcterm_set_attr(TCTERM_FG_RED);
                tcterm_print(str->ptr + head, ident - head);
                head = ident;
            }
            ident = head + strspn(str->ptr + head, "012345678.");
            if(ident - head)
            {
                tcterm_set_attr(TCTERM_FG_MAGENTA);
                tcterm_print(str->ptr + head, ident - head);
                head = ident;
            }
            tcterm_set_attr(TCTERM_FG_WHITE);
        }

        tcterm_print(str->ptr + head, tail - head);

        tcterm_set_attr(TCTERM_FG_WHITE);

        head = tail + strspn(str->ptr + tail, " ");
        tcterm_print(str->ptr + tail, head - tail);
    }
    while(str->ptr[tail]);
    tcterm_print(str->ptr + head, tail - head);

    tcterm_set_attr(TCTERM_FG_WHITE);
    return 1;
}

/*#include <locale.h>*/
int main(void)
{
    // converting input to UTF-8:
    // Windows: WideCharToMultiByte(CP_UTF8, 0, $input, $input.len or -1, $output, $output.len, NULL, NULL);
    // POSIX:
    //  - detect if UTF-8 via `$isutf = setlocale(LC_CTYPE, ""); <check $isutf for `.UTF-8`>`
    //  - if yes: nothing left to do!
    //  - if no:
    //      - setlocale(LC_CTYPE, "")
    //      - try (in order):
    //          - mbrtoc32() (check __STDC_UTF_32__)
    //          - mbrtoc16() (check __STDC_UTF_16__)
    //          - mbrtowc (check __STDC_ISO_10646__)
    //          - (now what; use mbrtowc and hope for the best?)
    //      - convert back via inverse function


    /*char* prev_lc = setlocale(LC_CTYPE, NULL);
    printf("PLC: %s\n", prev_lc);
    char* curr_lc = setlocale(LC_CTYPE, "");
    printf("CLC: %s\n", curr_lc);
    setlocale(LC_CTYPE, prev_lc);*/

    /*int init = tcterm_init_stdio();
    int cx=0, cy=0;
    int ci = tcterm_get_cursor_pos(&cx, &cy);
    int deinit = tcterm_deinit();

    printf("C %d | %d => %d,%d | %d\n", init, ci, cx, cy, deinit);*/

    tcedit_attach_stdio(512);
    atexit(vdeinit);

    TC_String* line;
    for(;;)
    {
        tcterm_set_attr(TCTERM_FG_YELLOW | TCTERM_FG_INTENSE);
        line = tcedit_readline(">>> ", -1, do_echo);
        if(!line) break;
        if(!line->len) continue;

        tcterm_printf("LINE `%.*s`\n", (int)line->len, line->ptr);
        if(!strcmp(line->ptr, "exit") || !strcmp(line->ptr, "quit"))
            break;
    }

    /*char buf[256];

    tcterm_init_stdio();
    atexit(vdeinit);

    tcterm_set_attr(TCTERM_FG_WHITE);
    tcterm_writeln(TCTERM_STDOUT, "%%%", -1);

    int ci, cx, cy;
    ci = tcterm_get_cursor_pos(&cx, &cy);

    snprintf(buf, sizeof(buf), "%d %d %d\n", ci, cx, cy);
    tcterm_write(TCTERM_STDOUT, buf, -1);

    tcterm_set_cursor_pos(cx + 3, cy);

    tcterm_set_attr(TCTERM_FG_RED);

    ci = tcterm_get_cursor_pos(&cx, &cy);
    snprintf(buf, sizeof(buf), "%d %d %d\n", ci, cx, cy);
    tcterm_write(TCTERM_STDOUT, buf, -1);
    tcterm_set_cursor_vis(25, 1);
    getchar();

    int csize, visible;
    ci = tcterm_get_cursor_vis(&csize, &visible);
    printf("CSIZE: %d %d\n", ci, csize);
    tcterm_set_cursor_vis(100, 1);
    printf("ISATTY: %d %d %d\n", tcterm_is_tty_file(stdin), tcterm_is_tty_file(stdout), tcterm_is_tty_file(stderr));

    tcterm_set_cursor_pos(cx + 3, cy);
    tcterm_clear_to_eol(TCTERM_STDOUT, '=');

    getchar();

    fprintf(stderr, "Stderr test!\n");

    getchar();*/

    return 0;
}
