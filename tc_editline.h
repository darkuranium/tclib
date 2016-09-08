/*
 * tc_editline.h: Terminal line editing support.
 *
 * DEPENDS: tc_history tc_terminal | tc_string
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
 * - multiline support
 * - callback(s) for tab key completion and hints
 *  typedef struct TC_EditEvent {
 *      int type;
 *      TC_String* input;
 *      size_t byte_pos, char_pos;
 *  } TC_EditEvent;
 *  int (*)(TC_EditEvent* event);
 */

#ifndef TC_EDITLINE_H_
#define TC_EDITLINE_H_

#include "tc_string.h"

#ifdef __cplusplus
extern "C" {
#endif

int tcedit_attach_stdio(int histlen);
int tcedit_detach(void);
TC_String* tcedit_readline(const char* prompt, int promptlen, int (*echo)(TC_String* str, size_t hpos));

#ifdef __cplusplus
}
#endif

#endif /* TC_EDITLINE_H_ */

#ifdef TC_EDITLINE_IMPLEMENTATION
#undef TC_EDITLINE_IMPLEMENTATION

#include "tc_history.h"
#include "tc_terminal.h"

static TC_History tcedit__hist;
int tcedit_attach_stdio(int histlen)
{
    if(!tcterm_init_stdio())
        return 0;
    if(!tchist_init(&tcedit__hist, histlen))
        return 0;
    tcterm_set_mode_raw();

    return 1;
}
int tcedit_detach(void)
{
    tchist_deinit(&tcedit__hist);
    int ret = tcterm_deinit();
    return ret;
}
TC_String* tcedit_readline(const char* prompt, int promptlen, int (*echo)(TC_String* str, size_t hpos))
{
    tchist_cmd_vmove_full(&tcedit__hist, +1);
    TC_String* str;

    if(prompt)
        tcterm_print(prompt, promptlen);

    int cx, cy, tx, ty;
    tcterm_get_cursor_pos(&cx, &cy);
    size_t hpos;

    int done = 0;
    int c;
    char cc;
    while(!done)
    {
        c = tcterm_getc();

        switch(c)
        {
        case TCTERM_EOF: return NULL;
        case '\r':
        case '\n':
            done = 1;
            break;

        case TCTERM_KEY_UP: tchist_cmd_vmove(&tcedit__hist, -1); break;
        case TCTERM_KEY_DOWN: tchist_cmd_vmove(&tcedit__hist, +1); break;
        case TCTERM_KEY_LEFT: tchist_cmd_hmove(&tcedit__hist, -1); break;
        case TCTERM_KEY_RIGHT: tchist_cmd_hmove(&tcedit__hist, +1); break;

        case TCTERM_KEY_INSERT: break; /* $$$ TODO INSERT $$$ */
        case TCTERM_KEY_DELETE: tchist_str_delete(&tcedit__hist, +1); break;
        case TCTERM_KEY_PAGE_UP: tchist_cmd_vmove_full(&tcedit__hist, -1); break;
        case TCTERM_KEY_PAGE_DOWN: tchist_cmd_vmove_full(&tcedit__hist, +1); break;
        case TCTERM_KEY_HOME: tchist_cmd_hmove_full(&tcedit__hist, -1); break;
        case TCTERM_KEY_END: tchist_cmd_hmove_full(&tcedit__hist, +1); break;

        case TCTERM_KEY_BACKSPACE: tchist_str_delete(&tcedit__hist, -1); break;

        default:
            if(c < 256)
            {
                cc = c;
                tchist_str_input(&tcedit__hist, &cc, 1, 1); break;
            }
            break;
        }
        tcterm_set_cursor_pos(cx, cy);
        tcterm_clear_to_eol(TCTERM_STDOUT);

        hpos = tchist_get_hpos(&tcedit__hist);

        str = tchist_get_string(&tcedit__hist);
        if(echo)
        {
            if(!echo(str, hpos))
                return NULL;
        }
        else
            tcterm_print(str->ptr, str->len);

        tx = cx; ty = cy;
        if(hpos)
            tx += tcstr_utf8_find_char(str, hpos);
        tcterm_set_cursor_pos(tx, ty);
    }
    tcterm_print("\n", 1);

    return tchist_exec(&tcedit__hist);
}

#endif /* TC_EDITLINE_IMPLEMENTATION */
