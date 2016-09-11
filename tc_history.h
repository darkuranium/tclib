/*
 * tc_history.h: Simple terminal history handling.
 *
 * DEPENDS: tc_string
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
 * - read history from file or stream
 * - multiline support
 */

#ifndef TC_HISTORY_H_
#define TC_HISTORY_H_

#include "tc_string.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TC__HistoryEntry;
typedef struct TC_History
{
    int maxlen;
    size_t mem, head, tail;
    struct TC__HistoryEntry* entries;

    size_t vpos, hpos;
} TC_History;

TC_History* tchist_init(TC_History* hist, int maxlen);
void tchist_deinit(TC_History* hist);

TC_String* tchist_get_string(TC_History* hist);

size_t tchist_get_hpos(TC_History* hist);
void tchist_cmd_vmove_full(TC_History* hist, int down);
void tchist_cmd_hmove_full(TC_History* hist, int right);
void tchist_cmd_vmove(TC_History* hist, int down);
void tchist_cmd_hmove(TC_History* hist, int right);

void tchist_str_input(TC_History* hist, const char* str, int len, int insert);
void tchist_str_delete(TC_History* hist, int len);
void tchist_str_clear(TC_History* hist);
TC_String* tchist_exec(TC_History* hist);

#ifdef __cplusplus
}
#endif

#endif /* TC_HISTORY_H_ */

#ifdef TC_HISTORY_IMPLEMENTATION
#undef TC_HISTORY_IMPLEMENTATION
#include <stdlib.h>

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

typedef struct TC__HistoryEntry
{
    TC_String orig, edit;
} TC__HistoryEntry;

static size_t tchist__get_tail_idx(TC_History* hist)
{
    return (hist->tail + hist->mem - 1) % hist->mem;
}
static size_t tchist__get_head_idx(TC_History* hist)
{
    return hist->head;
}
static size_t tchist__get_prev_idx(TC_History* hist, size_t idx)
{
    if(idx == hist->head)
        return idx;
    return (idx + hist->mem - 1) % hist->mem;
}
static size_t tchist__get_next_idx(TC_History* hist, size_t idx)
{
    if((idx + 1) % hist->mem == hist->tail)
        return idx;
    return (idx + 1) % hist->mem;
}

TC_History* tchist_init(TC_History* hist, int maxlen)
{
#ifdef __cplusplus
    /* goddamnit for making me do this because you absolutely need -Wextra! (you know who you are) */
    static const TC__HistoryEntry eentry = TC__HistoryEntry();
#else
    static const TC__HistoryEntry eentry;
#endif

    if(!hist) return NULL;

    hist->maxlen = maxlen;
    hist->mem = maxlen >= 0 ? maxlen + 1 : 1;
    hist->head = 0;
    hist->tail = 1;
    hist->vpos = tchist__get_tail_idx(hist);
    hist->hpos = 0;

    hist->entries = TC__VOID_CAST(TC__HistoryEntry*,malloc(hist->mem * sizeof(*hist->entries)));

    size_t i;
    for(i = 0; i < hist->mem; i++)
        hist->entries[i] = eentry;

    return hist;
}
void tchist_deinit(TC_History* hist)
{
    if(!hist) return;

    size_t i;
    for(i = 0; i < hist->mem; i++)
    {
        tcstr_deinit(&hist->entries[i].orig);
        tcstr_deinit(&hist->entries[i].edit);
    }
    free(hist->entries);
}

TC_String* tchist_get_string(TC_History* hist)
{
    return &hist->entries[hist->vpos].edit;
}

size_t tchist_get_hpos(TC_History* hist)
{
    return hist->hpos;
}
void tchist_cmd_vmove_full(TC_History* hist, int down)
{
    if(down > 0)
        hist->vpos = tchist__get_tail_idx(hist);
    else if(down < 0)
        hist->vpos = tchist__get_head_idx(hist);
    tchist_cmd_hmove_full(hist, +1);
}
void tchist_cmd_hmove_full(TC_History* hist, int right)
{
    if(right > 0)
        hist->hpos = hist->entries[hist->vpos].edit.len;
    else if(right < 0)
        hist->hpos = 0;
}
void tchist_cmd_vmove(TC_History* hist, int down)
{
    while(down > 0)
    {
        hist->vpos = tchist__get_next_idx(hist, hist->vpos);
        down--;
    }
    while(down < 0)
    {
        hist->vpos = tchist__get_prev_idx(hist, hist->vpos);
        down++;
    }
    tchist_cmd_hmove_full(hist, +1);
}
void tchist_cmd_hmove(TC_History* hist, int right)
{
    while(right > 0)
    {
        hist->hpos = tcstr_utf8_next_off(&hist->entries[hist->vpos].edit, hist->hpos);
        right--;
    }
    while(right < 0)
    {
        hist->hpos = tcstr_utf8_prev_off(&hist->entries[hist->vpos].edit, hist->hpos);
        right++;
    }
}
void tchist_str_input(TC_History* hist, const char* str, int len, int insert)
{
    if(len < 0) len = strlen(str);
    tcstr_splices(&hist->entries[hist->vpos].edit, hist->hpos, insert ? 0 : len, str, len);
    hist->hpos += len;
}
void tchist_str_delete(TC_History* hist, int len)
{
    TC_String* str = tchist_get_string(hist);
    size_t npos;
    while(len > 0) /* delete */
    {
        npos = tcstr_utf8_next_off(str, hist->hpos);
        tcstr_splices(str, hist->hpos, npos - hist->hpos, NULL, 0);
        len--;
    }
    while(len < 0) /* backspace */
    {
        npos = tcstr_utf8_prev_off(str, hist->hpos);
        tcstr_splices(str, npos, hist->hpos - npos, NULL, 0);
        hist->hpos = npos;
        len++;
    }
}
void tchist_str_clear(TC_History* hist)
{
    tcstr_reinit(&hist->entries[hist->vpos].edit, NULL);
}
TC_String* tchist_exec(TC_History* hist)
{
    static char emptystr[] = "";
    static TC_String empty = { 0, emptystr };
    TC__HistoryEntry* tentry = &hist->entries[tchist__get_tail_idx(hist)];
    TC__HistoryEntry* entry = &hist->entries[hist->vpos];

    TC_String* str = tchist_get_string(hist);
    if(!str->len)
    {
        tcstr_reinit(&entry->edit, &entry->orig);
        tchist_cmd_hmove_full(hist, -1);
        return &empty;
    }

    if(entry == tentry)
        tcstr_reinit(&entry->orig, &entry->edit);
    else
    {
        tcstr_reinit(&tentry->orig, &entry->edit);
        tcstr_reinit(&tentry->edit, &entry->edit);

        tcstr_reinit(&entry->edit, &entry->orig);
    }

    hist->tail = (hist->tail + 1) % hist->mem;

    tentry = &hist->entries[tchist__get_tail_idx(hist)];
    tcstr_reinits(&tentry->orig, NULL, 0);
    tcstr_reinits(&tentry->edit, NULL, 0);

    if(hist->tail == hist->head)
        hist->head = (hist->head + 1) % hist->mem;

    tchist_cmd_hmove_full(hist, -1);

    return tchist_get_string(hist);
}

#endif /* TC_HISTORY_IMPLEMENTATION */
