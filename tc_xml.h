/*
 * tc_xml.h: XML file parser.
 *
 * DEPENDS:
 * VERSION: 0.1.0 (2024-08-01)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Čas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.1.0    initial public release
 *
 * TODOS:
 * - Ensure full compliance with XML spec (for non-validating parsers)
 * - Introduce a node-based / DOM-like API
 *   - Add an XPath-like parser to it (but might be a separate lib)
 * - [maybe] Introduce various helper flags (like TCXML_CONCAT_CDATA)
 * - Full C++-compilation-mode support
 */
#ifndef TC_XML_H_
#define TC_XML_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct tcxml_string
{
    size_t len;
    char* ptr;
} tcxml_string_t;

typedef struct tcxml_error
{
    size_t offset, line, column;
    const char* message;
} tcxml_error_t;

#define TCXML_ERROR_IS_OK(error)    !(error).message

// Utility functions; used internally, but provided here as they may be useful.

// Convert UTF-32 to UTF-8, returning the number of characters written into `utf8` (or 0 on error).
size_t tcxml_utf8_from_utf32(char utf8[4], uint32_t utf32);
// Convert UTF-8 to UTF-32, returning the number of characters read from `utf8` (or 0 on error).
// `utf32` may be NULL, in which case this simply counts bytes that make up 1 UTF-32 character.
size_t tcxml_utf32_from_utf8(uint32_t* utf32, const char* utf8, size_t utf8len);

typedef struct tcxml_sax_callbacks
{
    // start of parse
    void (*start)(void* udata);
    // end of parse
    void (*end)(void* udata);

    // <?xml ...?>
    void (*xml_decl)(tcxml_string_t version, tcxml_string_t encoding, bool* standalone, void* udata);
    // cdata
    void (*cdata)(tcxml_string_t data, void* udata);
    // basic text
    void (*text)(tcxml_string_t text, size_t body_head, size_t body_tail, void* udata);
    // element start, e.g. <foo>
    void (*element_start)(tcxml_string_t tag, tcxml_string_t* attrs, size_t nattrs, void* udata);
    // element end, e.g. </foo>
    void (*element_end)(tcxml_string_t tag, void* udata);
    // processing instruction, e.g. <?foo ... ?>
    void (*processing_instruction)(tcxml_string_t target, tcxml_string_t body, void* udata);
    // comment, e.g. <!-- ... -->
    void (*comment)(tcxml_string_t text, void* udata);

    // unknown entity reference, e.g. &foo;
    // note that this is *not* triggered for &amp; &lt; &gt; &apos; &quot;
    // should return the replacement text in `replacement` (defaults to `&<ref>;`, i.e. no-op), and a boolean on whether we want to continue parse (true), or abort with error (false)
    bool (*unknown_entity_reference)(tcxml_string_t* replacement, tcxml_string_t ref, void* udata);

    // whitespace that can be ignored, e.g. <x>[here]<y/>[here]</x>
    //void (*ignorable_whitespace)(tcxml_string_t text, void* udata);

    // handle missing start & end elements; `stack` can be inspected & modified to help resolve the problem
    // should return whether we should continue parsing (true), or abort (false)
    //bool (*error_element_start_missing)(char* tag, char** stack, size_t* stack_top, void* udata);
    //bool (*error_element_end_missing)(char* tag, char** stack, size_t* stack_top, void* udata);
} tcxml_sax_callbacks_t;

/// A streaming parser context, in order to reuse memory allocations.
typedef struct tcxml_sax_buffers
{
    // allocated strings, this is reused between callback calls
    struct
    {
        size_t maxlen;  // used so that we know how many subarrays to free
        size_t mem, len;
        struct
        {
            size_t mem, len;
            char* ptr;
        }* ptr;
    } data_buf;

    // text buffer, contains already-converted text (e.g. `they&apos;re` => `they're`)
    struct
    {
        size_t mem, len;
        char* ptr;
    } text_buf;

    // element attribute list, passed to callback
    struct
    {
        size_t mem, len;
        tcxml_string_t* ptr;
    } attrs;

    // element tag name stack
    /*struct
    {
        size_t mem, len;
        tcxml_string_t* ptr;
    } stack;*/
} tcxml_sax_buffers_t;

tcxml_sax_buffers_t* tcxml_sax_buffers_init(tcxml_sax_buffers_t* bufs);
void tcxml_sax_buffers_deinit(const tcxml_sax_buffers_t* bufs);

/// `bufs` is optional. If NULL, will allocate a temporary context.
tcxml_error_t tcxml_sax_process(tcxml_sax_buffers_t* bufs, const char* src, const tcxml_sax_callbacks_t* cbs, void* udata);

/*
#define TCXML_CONCAT_CDATA                  0x01    // concatenate neighbouring cdata, e.g. `<![CDATA[foo]]><![CDATA[bar]]>` into `foobar`
#define TCXML_ALLOW_HTML_LIKE_ATTRIBS       0x02    // allow HTML-like attributes (`<x foo=bar baz/>` --- no quotes and attribs without values)
#define TCXML_ALLOW_NONCOMPLIANT_COMMENTS   0x04    // allow non-compliant comments, i.e. comments containing `--`
#define TCXML_INSERT_MISSING_START          0x08    // insert missing start elements (this prevents `error_element_start_missing` from being called)
#define TCXML_INSERT_MISSING_END            0x10    // insert missing end elements (this prevents `error_element_end_missing` from being called)

#define TCXML_NODE_XML_DECL                 0   // <?xml ...?>
#define TCXML_NODE_CDATA                    1   // <![CDATA[...]]>
#define TCXML_NODE_TEXT                     2   // plain text
#define TCXML_NODE_ELEMENT                  3   // <foo>...</foo> <bar/>
#define TCXML_NODE_PROCESSING_INSTRUCTION   4   // <?foo ...?>
#define TCXML_NODE_COMMENT                  5   // <!-- ... -->
*/

typedef struct tcxml_node
{
    uint8_t type;
    // VALID IN: all
    char* whitespace_pre;   // preceding ignorable whitespace
    char* whitespace_post;  // following ignorable whitespace
    // VALID IN: xml_decl (always "xml"), element, processing_instruction
    char* tag;
    // VALID IN: cdata, text, processing_instruction, comment
    tcxml_string_t text;    // text contents
    // VALID IN: xml_decl, element
    tcxml_string_t const* args;
    // VALID IN: element
    size_t nchildren;
    struct tcxml_node* children;
} tcxml_node_t;

#endif /* TC_XML_H_ */


#ifdef TC_XML_IMPLEMENTATION
#undef TC_XML_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <assert.h>
//#include <errno.h>  // for ERANGE, as triggered by strtoul

#ifdef __cplusplus
    #if defined(_MSC_VER) || defined(__GNUC__)
        #define restrict __restrict
    #else
        #define restrict
    #endif
//#define TC__RESTRICT(T)     T restrict
#endif

size_t tcxml_utf8_from_utf32(char utf8[4], uint32_t utf32)
{
    if(utf32 <= 0x7F)
    {
        utf8[0] = utf32;                        // 0xxxxxxx
        return 1;
    }
    if(utf32 <= 0x7FF)
    {
        utf8[0] = 0xC0 | (utf32 >> 6);          // 110xxxxx
        utf8[1] = 0x80 | (utf32 & 0x3F);        // 10xxxxxx
        return 2;
    }
    if(utf32 <= 0xFFFF)
    {
        utf8[0] = 0xE0 | (utf32 >> 12);         // 1110xxxx
        utf8[1] = 0x80 | ((utf32 >> 6) & 0x3F); // 10xxxxxx
        utf8[2] = 0x80 | (utf32 & 0x3F);        // 10xxxxxx
        return 3;
    }
    if(utf32 <= 0x10FFFF)
    {
        utf8[0] = 0xF0 | (utf32 >> 18);         // 11110xxx
        utf8[1] = 0x80 | ((utf32 >> 12) & 0x3F);// 10xxxxxx
        utf8[1] = 0x80 | ((utf32 >> 6) & 0x3F); // 10xxxxxx
        utf8[2] = 0x80 | (utf32 & 0x3F);        // 10xxxxxx
        return 4;
    }
    return 0;   // invalid
}

// TODO performance: Optimize
size_t tcxml_utf32_from_utf8(uint32_t* utf32, const char* utf8, size_t utf8len)
{
    // to simplify logic below (avoid `if` conditions for NULL)
    uint32_t utf32_tmp;
    if(!utf32) utf32 = &utf32_tmp;

    *utf32 = UINT32_MAX;    // for invalid UTF-8, we'll simply store an invalid UTF-32 character
    if(!utf8len)
        return 0;

    if(!(utf8[0] & 0x80))           // 0xxxxxxx
    {
        *utf32 = utf8[0];
        return 1;
    }
    else if(utf8len < 2)
        return 0;   // truncated UTF-8

    if((utf8[0] & 0xE0) == 0xC0     // 110xxxxx
    && (utf8[1] & 0xC0) == 0x80)    // 10xxxxxx
    {
        *utf32 = (utf8[0] & 0x1F) << 6
               | (utf8[1] & 0x3F);
        return 2;
    }
    else if(utf8len < 3)
        return 0;   // truncated UTF-8

    if((utf8[0] & 0xF0) == 0xE0     // 1110xxxx
    && (utf8[1] & 0xC0) == 0x80     // 10xxxxxx
    && (utf8[2] & 0xC0) == 0x80)    // 10xxxxxx
    {
        *utf32 = (utf8[0] & 0x0F) << 12
               | (utf8[1] & 0x3F) << 6
               | (utf8[2] & 0x3F);
        return 3;
    }
    else if(utf8len < 4)
        return 0;

    if((utf8[0] & 0xF8) == 0xF0     // 11110xxx
    && (utf8[1] & 0xC0) == 0x80     // 10xxxxxx
    && (utf8[2] & 0xC0) == 0x80     // 10xxxxxx
    && (utf8[3] & 0xC0) == 0x80)    // 10xxxxxx
    {
        *utf32 = (utf8[0] & 0x0F) << 18
               | (utf8[1] & 0x3F) << 12
               | (utf8[2] & 0x3F) << 6
               | (utf8[3] & 0x3F);
        return 4;
    }
    //could use `else if(utf8len < 5)` if we wanted to extend UTF-8 support to up to 0x7FFFFFFF / 6 bytes (but we don't)

    return 0;   // invalid UTF-8
}


tcxml_sax_buffers_t* tcxml_sax_buffers_init(tcxml_sax_buffers_t* bufs)
{
    if(!bufs) return NULL;
    *bufs = (tcxml_sax_buffers_t){ {0} };
    return bufs;
}
void tcxml_sax_buffers_deinit(const tcxml_sax_buffers_t* bufs)
{
    if(!bufs) return;
    tcxml_sax_buffers_t mbufs = *bufs;

    for(size_t i = 0; i < mbufs.data_buf.maxlen; i++)
        free(mbufs.data_buf.ptr[i].ptr);
    free(mbufs.data_buf.ptr);

    free(mbufs.text_buf.ptr);
    free(mbufs.attrs.ptr);
    //free(mbufs.stack.ptr);
}


// https://www.w3.org/TR/xml/#sec-documents
struct tcxml_parse_context_
{
    tcxml_sax_buffers_t* bufs;
    const tcxml_sax_callbacks_t* cbs;
    void* udata;

    tcxml_error_t error;
    // non-`const` for convenience (to avoid tons of casts for tcxml_string_t), but actually never modified
    char* str;
    char* ptr;

    // a temporary used by various functions
    tcxml_string_t capture;
};

static size_t tcxml_next_pow_2_(size_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
#if SIZE_MAX >= UINT64_MAX
    v |= v >> 32;
#endif
    v++;
    return v;
}

static void tcxml_arr_ensuremem_impl_(void** ptr, size_t size, size_t* mem, size_t len)
{
    if(*mem < len)
    {
        *mem = len < 8 ? 8 : tcxml_next_pow_2_(len);
        *ptr = realloc(*ptr, *mem * size);
    }
}
//#define TCXML_ARR_ENSUREMEM_(arr) tcxml_arr_ensuremem_impl_(&(arr)->ptr, sizeof(*(arr)->ptr), &(arr)->mem, (arr)->len)

#define TCXML_ARR_APPENDN_(arr, n)  (tcxml_arr_ensuremem_impl_((void**)&(arr)->ptr, sizeof(*(arr)->ptr), &(arr)->mem, ((arr)->len += (n))), &(arr)->ptr[(arr)->len - (n)])
//#define TCXML_ARR_PUSH_(arr, item)  (*TCXML_ARR_APPENDN_(arr, 1) = (item))
//#define TCXML_ARR_POP_(arr)         (assert((arr)->len), (arr)->ptr[--(arr)->len])


static void tcxml_data_reset_(tcxml_sax_buffers_t* restrict bufs)
{
    bufs->data_buf.len = 0;
}
static tcxml_string_t tcxml_data_push_(tcxml_sax_buffers_t* restrict bufs, tcxml_string_t str)
{
    size_t index = bufs->data_buf.len;

    // reserve a data slot
    TCXML_ARR_APPENDN_(&bufs->data_buf, 1);
    if(bufs->data_buf.maxlen < bufs->data_buf.len)
    {
        assert(bufs->data_buf.maxlen + 1 == bufs->data_buf.len);
        bufs->data_buf.maxlen = bufs->data_buf.len;
        // first time reaching this far, ensure value is initialized
        bufs->data_buf.ptr[index].mem = 0;   // (`len` is reset below anyway)
        bufs->data_buf.ptr[index].ptr = NULL;
    }

    // populate the slot (note that we need to do this even if str.ptr == NULL, so that popn_ works correctly)
    bufs->data_buf.ptr[index].len = 0;

    if(!str.ptr)
        return (tcxml_string_t){ 0, NULL };

    TCXML_ARR_APPENDN_(&bufs->data_buf.ptr[index], str.len + 1);
    memcpy(bufs->data_buf.ptr[index].ptr, str.ptr, str.len);
    bufs->data_buf.ptr[index].ptr[str.len] = 0;
    return (tcxml_string_t){
        str.len,
        bufs->data_buf.ptr[index].ptr,
    };
}
static tcxml_string_t tcxml_data_pushpp_(tcxml_sax_buffers_t* restrict bufs, const char* head, const char* tail)
{
    return tcxml_data_push_(bufs, (tcxml_string_t){ tail - head, (char*)head });
}
static tcxml_string_t tcxml_data_pushpn_(tcxml_sax_buffers_t* restrict bufs, const char* str, size_t len)
{
    return tcxml_data_push_(bufs, (tcxml_string_t){ len, (char*)str });
}
// Guaranteed to *not* free memory.
static void tcxml_data_popn_(tcxml_sax_buffers_t* restrict bufs, size_t n)
{
    assert(bufs->data_buf.len >= n);
    bufs->data_buf.len -= n;
}


static void tcxml_text_reset_(tcxml_sax_buffers_t* restrict bufs)
{
    bufs->text_buf.len = 0;
}
static void tcxml_text_append_(tcxml_sax_buffers_t* restrict bufs, tcxml_string_t str, bool normalize_eol)
{
    size_t olen = bufs->text_buf.len;
    char* ptr = TCXML_ARR_APPENDN_(&bufs->text_buf, str.len);
    if(str.len)
    {
        if(!normalize_eol)
            memcpy(ptr, str.ptr, str.len);
        else
        {
            // normalize line endings as we copy; this may reduce the string length, so we fix that up at the end
            size_t pos = 0;
            for(;;)
            {
                const char* cr = memchr(&str.ptr[pos], '\r', str.len - pos);
                if(!cr)
                    break;
                // found a CR, first simply copy over everything until that
                size_t nleading = (cr - str.ptr) - pos;
                memcpy(&ptr[olen], &str.ptr[pos], nleading);
                olen += nleading;

                // emit a LF and adjust `pos` to one beyond it (so that next search doesn't find the same thing)
                ptr[olen++] = '\n';
                pos = cr - str.ptr + 1;
                // check for CRLF
                if(pos < str.len && str.ptr[pos] == '\n')
                    ++pos;  // skip LF in CRLF (since this should count as *one* newline)
            }
            // copy any final (trailing) data, after last CR
            memcpy(&ptr[olen], &str.ptr[pos], str.len - pos);
            olen += str.len - pos;

            // finally, adjust the length to actual length
            assert(olen <= bufs->text_buf.len && "Output length out of bounds");
            bufs->text_buf.len = olen;
        }
    }
}
static void tcxml_text_appendpp_(tcxml_sax_buffers_t* restrict bufs, const char* head, const char* tail, bool normalize_eol)
{
    tcxml_text_append_(bufs, (tcxml_string_t){ tail - head, (char*)head }, normalize_eol);
}
static void tcxml_text_appendpn_(tcxml_sax_buffers_t* restrict bufs, const char* str, size_t len, bool normalize_eol)
{
    tcxml_text_append_(bufs, (tcxml_string_t){ len, (char*)str }, normalize_eol);
}
static tcxml_string_t tcxml_text_finish_(tcxml_sax_buffers_t* restrict bufs)
{
    tcxml_string_t str = {bufs->text_buf.len, bufs->text_buf.ptr};
    *TCXML_ARR_APPENDN_(&bufs->text_buf, 1) = 0;
    return str;
}


// compare `str1` with a 0-terminated `str2`
static int tcxml_string_cmpz_(tcxml_string_t str1, const char* str2)
{
    size_t str2_len = strlen(str2);
    if(str1.len < str2_len)
        return -1;
    if(str1.len > str2_len)
        return +1;
    return memcmp(str1.ptr, str2, str1.len);
}

static bool tcxml_starts_with_(const char* str, const char* start)
{
    return !memcmp(str, start, strlen(start));
}

static void tcxml_advance_line_col_(const char* head, const char* tail, tcxml_error_t* error)
{
    while(head < tail)
    {
        size_t u8chars;
        switch(head[0])
        {
        case '\r':
            if(head + 1 < tail && head[1] == '\n')
                ++head; // skip over next `head`
            /* (fallthrough) */
        case '\n':
            ++error->line;
            error->column = 0;
            ++head;
            break;
        default:
            u8chars = tcxml_utf32_from_utf8(NULL, head, tail - head);
            if(!u8chars) u8chars = 1;   // in case of invalid UTF-8, we assume 1 char
            error->column += u8chars;
            head += u8chars;
            break;
        }
    }
}

static tcxml_error_t tcxml_make_error_(struct tcxml_parse_context_* restrict ctx, const char* message)
{
    tcxml_error_t error = {
        .offset = ctx->ptr - ctx->str,
        .message = message,
    };
    //tcxml_advance_line_col_(ctx->str + ctx->bom_skip, ctx->ptr, &error);
    return error;
}

static bool tcxml_match_(const char* str, struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_starts_with_(ctx->ptr, str))
        return false;
    ctx->ptr += strlen(str);
    return true;
}

#define TCXML_WSPACE_CHARS_ " \t\r\n"
static size_t tcxml_measure_wspace_left_(tcxml_string_t str)
{
    size_t i;
    for(i = 0; i < str.len; i++)
        if(!strchr(TCXML_WSPACE_CHARS_, str.ptr[i]))
            break;
    return i;
}
static size_t tcxml_measure_wspace_right_(tcxml_string_t str)
{
    size_t i;
    for(i = str.len; i > 0; i--)
        if(!strchr(TCXML_WSPACE_CHARS_, str.ptr[i-1]))
            break;
    return str.len - i;
}

#define TCXML_ERROR_(MESSAGE)       (ctx->error = tcxml_make_error_(ctx, MESSAGE), false)
#define TCXML_CAPTURE_(HEAD,TAIL)   (ctx->capture = (tcxml_string_t){ (TAIL) - (HEAD), (HEAD) }, true)

static bool tcxml_px_prolog_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_px_Misc_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_px_CDSect_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_p_SDDecl_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_px_element_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_p_STag_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_p_ETag_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_px_content_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_p_EmptyElemTag_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_p_Reference_(struct tcxml_parse_context_* restrict ctx);
static bool tcxml_p_EncodingDecl_(struct tcxml_parse_context_* restrict ctx);

/*
document    ::= prolog element Misc*
*/
static bool tcxml_px_document_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_px_prolog_(ctx))
        return false;   // forward error
    if(!tcxml_px_element_(ctx))
        return false;   // forward error

    char* ptr;
    do
    {
        ptr = ctx->ptr;
    }
    while(tcxml_px_Misc_(ctx));
    ctx->ptr = ptr; // undo last, partial tcxml_p_Misc_

    return true;    // no capture
}

/*
==SKIP==
Char    ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]
    -- NOTE: simplified to allow all Unicode except #x0 --
*/

/*
S	   ::=   	(#x20 | #x9 | #xD | #xA)+
 */
static bool tcxml_p_S_(struct tcxml_parse_context_* restrict ctx)
{
    char* head = ctx->ptr;
    ctx->ptr += strspn(ctx->ptr, " \t\r\n");
    if(head == ctx->ptr)
        return TCXML_ERROR_("Expected whitespace");
    return TCXML_CAPTURE_(head, ctx->ptr);
}


/*
NameStartChar   ::= ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] |
                    [#xF8-#x2FF] | [#x370-#x37D] | [#x37F-#x1FFF] |
                    [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF] |
                    [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] |
                    [#x10000-#xEFFFF]
NameChar        ::= NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] |
                    [#x203F-#x2040]
Name            ::= NameStartChar (NameChar)*
*/
static bool tcxml_p_NameStartChar_(struct tcxml_parse_context_* restrict ctx)
{
    // $$$ TODO: Handle UTF-8
    if(*ctx->ptr == ':'
    || ('A' <= *ctx->ptr && *ctx->ptr <= 'Z')
    || *ctx->ptr == '_'
    || ('a' <= *ctx->ptr && *ctx->ptr <= 'z'))
    {
        char* head = ctx->ptr++;
        return TCXML_CAPTURE_(head, ctx->ptr);
    }
    return TCXML_ERROR_("Expected start of XML name character");
}
static bool tcxml_p_NameChar_(struct tcxml_parse_context_* restrict ctx)
{
    if(tcxml_p_NameStartChar_(ctx))
        return true;
    // $$$ TODO: Handle UTF-8
    if(*ctx->ptr == '-'
    || *ctx->ptr == '.'
    || ('0' <= *ctx->ptr && *ctx->ptr <= '9'))
    {
        char* head = ctx->ptr++;
        return TCXML_CAPTURE_(head, ctx->ptr);
    }
    return TCXML_ERROR_("Expected XML name character");
}
static bool tcxml_p_Name_(struct tcxml_parse_context_* restrict ctx)
{
    char* head = ctx->ptr;
    if(!tcxml_p_NameStartChar_(ctx))
        return TCXML_ERROR_("Expected XML name");
    while(tcxml_p_NameChar_(ctx)) { /* nothing to do */ }
    return TCXML_CAPTURE_(head, ctx->ptr);
}
/*
==SKIP==
Names           ::= Name (#x20 Name)*
Nmtoken         ::= (NameChar)+
Nmtokens        ::= Nmtoken (#20 Nmtoken)*

EntityValue     ::= '"' ([^%&"] | PEReference | Reference)* '"' |
                    "'" ([^%&'] | PEReference | Reference)* "'"
*/

/*
AttValue	    ::= '"' ([^<&"] | Reference)* '"' |
                    "'" ([^<&'] | Reference)* "'"
*/
static bool tcxml_p_AttValue_(struct tcxml_parse_context_* restrict ctx)
{
    char quot = *ctx->ptr;
    if(quot != '"' && quot != '\'')
        return TCXML_ERROR_("Expected `\"` or `'` to start attribute value");
    ++ctx->ptr; // quot

    tcxml_text_reset_(ctx->bufs);
    while(*ctx->ptr && *ctx->ptr != quot)
    {
        size_t span = strcspn(ctx->ptr, quot == '"' ? "<&\"" : "<&'");
        if(span)
        {
            tcxml_text_appendpn_(ctx->bufs, ctx->ptr, span, true);
            ctx->ptr += span;
        }
        else if(!tcxml_p_Reference_(ctx))
            return TCXML_ERROR_("Invalid attribute value contents");
    }

    if(*ctx->ptr != quot)
        return TCXML_ERROR_("Expected end of attribute value quoted string");
    ++ctx->ptr; // quot

    return true;    // forward capture
}

/*
SystemLiteral   ::= ('"' [^"]* '"') | ("'" [^']* "'")
PubidLiteral    ::= '"' PubidChar* '"' | "'" (PubidChar - "'")* "'"
PubidChar       ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%]
*/

/*
CharData        ::= [^<&]* - ([^<&]* ']]>' [^<&]*)
    -- NOTE: simplified to [^<&]* --
*/
static bool tcxml_p_CharData_(struct tcxml_parse_context_* restrict ctx)
{
    char* head = ctx->ptr;
    ctx->ptr += strcspn(ctx->ptr, "<&");
    tcxml_text_appendpp_(ctx->bufs, head, ctx->ptr, false);
    return true;    // CharData always succeeds, as it can be empty
}

/*
Comment         ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
*/
static bool tcxml_px_Comment_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_match_("<!--", ctx))
        return TCXML_ERROR_("Expected start of a comment");

    char* chead = ctx->ptr;
    while(ctx->ptr[0] && (ctx->ptr[0] != '-' || ctx->ptr[1] != '-'))
        ++ctx->ptr;
    char* ctail = ctx->ptr;

    if(!tcxml_match_("-->", ctx))
        return TCXML_ERROR_("Expected end of a comment");

    if(ctx->cbs->comment)
    {
        tcxml_string_t text = tcxml_data_pushpp_(ctx->bufs, chead, ctail);
        ctx->cbs->comment(text, ctx->udata);
        tcxml_data_popn_(ctx->bufs, 1);
    }

    return true;
}

/*
PI          ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
PITarget    ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l'))
*/
static bool tcxml_p_PITarget_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_p_Name_(ctx))
        return TCXML_ERROR_("Expected processing instruction target name");
    if(ctx->capture.len == 3
    && strchr("Xx", ctx->capture.ptr[0])
    && strchr("Mm", ctx->capture.ptr[1])
    && strchr("Ll", ctx->capture.ptr[2]))
        return TCXML_ERROR_("Processing instruction target `xml` is reserved");
    return true;    // return Name
}
static bool tcxml_px_PI_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_match_("<?", ctx))
        return TCXML_ERROR_("Expected start of processing instruction");
    if(!tcxml_p_PITarget_(ctx))
        return false;   // forward error
    tcxml_string_t target = ctx->capture;

    tcxml_string_t body;
    if(tcxml_p_S_(ctx))
    {
        char* bhead = ctx->ptr;
        while(ctx->ptr[0] && (ctx->ptr[0] != '?' || ctx->ptr[1] != '>'))
            ++ctx->ptr;
        char* btail = ctx->ptr;
        body = (tcxml_string_t){ btail - bhead, bhead };
    }
    else
        body = (tcxml_string_t){ 0, NULL }; // no body => no capture

    if(!tcxml_match_("?>", ctx))
        return TCXML_ERROR_("Expected end of processing instruction");

    if(ctx->cbs->processing_instruction)
    {
        target = tcxml_data_push_(ctx->bufs, target);
        body = tcxml_data_push_(ctx->bufs, body);
        ctx->cbs->processing_instruction(target, body, ctx->udata);
        tcxml_data_popn_(ctx->bufs, 2);
    }

    return true;
}

/*
CDSect      ::= CDStart CData CDEnd
CDStart     ::= '<![CDATA['
CData       ::= (Char* - (Char* ']]>' Char*))
CDEnd       ::= ']]>'
*/
static bool tcxml_p_CDStart_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_match_("<![CDATA[", ctx))
        return TCXML_ERROR_("Expected CDATA start");
    return true;
}
static bool tcxml_p_CData_(struct tcxml_parse_context_* restrict ctx)
{
    char* dhead = ctx->ptr;
    while(ctx->ptr[0] && (ctx->ptr[0] != ']' || ctx->ptr[1] != ']' || ctx->ptr[2] != '>'))
        ++ctx->ptr;
    char* dtail = ctx->ptr;
    tcxml_text_appendpp_(ctx->bufs, dhead, dtail, true);
    return true;    // result in text_buf
}
static bool tcxml_p_CDEnd_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_match_("]]>", ctx))
        return TCXML_ERROR_("Expected CDATA end");
    return true;
}
static bool tcxml_px_CDSect_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_p_CDStart_(ctx))
        return false;   // forward error

    tcxml_text_reset_(ctx->bufs);
    if(!tcxml_p_CData_(ctx))
        return false;   // forward error

    if(!tcxml_p_CDEnd_(ctx))
        return false;   // forward error

    if(ctx->cbs->cdata)
    {
        tcxml_string_t cdata = tcxml_text_finish_(ctx->bufs);
        ctx->cbs->cdata(cdata, ctx->udata);
    }

    return true;
}

/*
prolog      ::= XMLDecl? Misc* (doctypedecl Misc*)?
XMLDecl     ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'
VersionInfo ::= S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"')
Eq          ::= S? '=' S?
VersionNum  ::= '1.' [0-9]+
Misc        ::= Comment | PI | S
    -- NOTE: simplified by removing doctypedecl --
*/
static bool tcxml_p_Eq_(struct tcxml_parse_context_* restrict ctx)
{
    tcxml_p_S_(ctx);
    if(!tcxml_match_("=", ctx))
        return TCXML_ERROR_("Expected '=' sign");
    tcxml_p_S_(ctx);
    return true;    // ctx->capture irrelevant
}
static bool tcxml_p_VersionNum_(struct tcxml_parse_context_* restrict ctx)
{
    char* vhead = ctx->ptr;
    if(!tcxml_match_("1.", ctx))
        return TCXML_ERROR_("Invalid XML version: expected '1.x'");
    char* head = ctx->ptr;
    ctx->ptr += strspn(ctx->ptr, "0123456789");
    if(head == ctx->ptr)
        return TCXML_ERROR_("Invalid XML version: expected digits after '1.'");
    return TCXML_CAPTURE_(vhead, ctx->ptr);
}
static bool tcxml_p_VersionInfo_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_p_S_(ctx))
        return false;   // forward error
    if(!tcxml_match_("version", ctx))
        return TCXML_ERROR_("Expected `version` declaration attribute");
    if(!tcxml_p_Eq_(ctx))
        return false;   // forward error

    char quot = *ctx->ptr;
    if(quot != '\'' && quot != '"')
        return TCXML_ERROR_("Expected start of quoted string");
    ++ctx->ptr; // quot

    if(!tcxml_p_VersionNum_(ctx))
        return false;   // forward error

    if(*ctx->ptr != quot)
        return TCXML_ERROR_("Expected end of quoted string");
    ++ctx->ptr; // quot

    return true;    // forward `capture` from VersionNum
}
static bool tcxml_px_XMLDecl_(struct tcxml_parse_context_* restrict ctx)
{
    char* ptr;

    if(!tcxml_match_("<?xml", ctx))
        return TCXML_ERROR_("Expected XML declaration");

    if(!tcxml_p_VersionInfo_(ctx))
        return false;   // forward error
    tcxml_string_t version = ctx->capture;

    tcxml_string_t encoding;
    ptr = ctx->ptr;
    if(tcxml_p_EncodingDecl_(ctx))
        encoding = ctx->capture;
    else
    {
        encoding = (tcxml_string_t){ 0, NULL };
        ctx->ptr = ptr;
    }

    int standalone;
    ptr = ctx->ptr;
    if(tcxml_p_SDDecl_(ctx))
        standalone = ctx->capture.ptr[0] == 'y';
    else
    {
        standalone = -1;
        ctx->ptr = ptr;
    }

    tcxml_p_S_(ctx);

    if(!tcxml_match_("?>", ctx))
        return TCXML_ERROR_("Expected end of XML declaration");

    if(ctx->cbs->xml_decl)
    {
        version = tcxml_data_push_(ctx->bufs, version);
        encoding = tcxml_data_push_(ctx->bufs, encoding);
        bool standalone_bool = standalone;
        ctx->cbs->xml_decl(version, encoding, standalone != -1 ? &standalone_bool : NULL, ctx->udata);
        tcxml_data_popn_(ctx->bufs, 2);
    }

    return true;
}
static bool tcxml_px_prolog_(struct tcxml_parse_context_* restrict ctx)
{
    char* ptr;

    ptr = ctx->ptr;
    if(!tcxml_px_XMLDecl_(ctx))
        ctx->ptr = ptr;

    do
    {
        ptr = ctx->ptr;
    }
    while(tcxml_px_Misc_(ctx));
    ctx->ptr = ptr; // undo last, partial tcxml_px_Misc_

    return true;
}
static bool tcxml_px_Misc_(struct tcxml_parse_context_* restrict ctx)
{
    char* ptr = ctx->ptr;

    if(tcxml_px_Comment_(ctx))
        return true;
    ctx->ptr = ptr;

    if(tcxml_px_PI_(ctx))
        return true;
    ctx->ptr = ptr;

    if(tcxml_p_S_(ctx))
    {
        /*if(ctx->cbs->ignorable_whitespace)
        {
            tcxml_string_t text = tcxml_data_push_(ctx, ctx->capture);
            ctx->cbs->ignorable_whitespace(text, ctx->udata);
            tcxml_data_popn_(ctx, 1);
        }*/
        return true;
    }
    //ctx->ptr = ptr;

    return TCXML_ERROR_("Expected comment, processing instruction, or whitespace");
}

/*
SDDecl          ::= S 'standalone' Eq (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"'))
*/
static bool tcxml_p_SDDecl_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_p_S_(ctx))
        return false;   // forward error
    if(!tcxml_match_("standalone", ctx))
        return TCXML_ERROR_("Expected `standalone=...` declaration attribute");
    if(!tcxml_p_Eq_(ctx))
        return false;   // forward error

    char quot = *ctx->ptr;
    if(quot != '\'' && quot != '"')
        return TCXML_ERROR_("Expected start of quoted string");
    ++ctx->ptr; // quot

    char* head = ctx->ptr;
    if(!tcxml_match_("yes", ctx)
    && !tcxml_match_("no", ctx))
        return TCXML_ERROR_("Expected 'yes' or 'no' for `standalone=...` declaration attribute");
    char* tail = ctx->ptr;

    if(*ctx->ptr != quot)
        return TCXML_ERROR_("Expected end of quoted string");
    ++ctx->ptr; // quot

    return TCXML_CAPTURE_(head,tail);
}


/*
element         ::= EmptyElemTag |
                    STag content ETag
*/
static bool tcxml_px_element_(struct tcxml_parse_context_* restrict ctx)
{
    char* ptr;

    ptr = ctx->ptr;
    if(tcxml_p_EmptyElemTag_(ctx))
    {
        if(ctx->cbs->element_start || ctx->cbs->element_end)
        {
            assert((ctx->bufs->attrs.len & 1) == 0 && "Expected an even number of attribute elements");

            tcxml_string_t tag = tcxml_data_push_(ctx->bufs, ctx->capture);
            if(ctx->cbs->element_start)
                ctx->cbs->element_start(tag, ctx->bufs->attrs.ptr, ctx->bufs->attrs.len / 2, ctx->udata);
            if(ctx->cbs->element_end)
                ctx->cbs->element_end(tag, ctx->udata);
            tcxml_data_popn_(ctx->bufs, 1);
        }
        tcxml_data_reset_(ctx->bufs); // (optional)
        return true;    // don't care about capture (we've already invoked events)
    }
    ctx->ptr = ptr;

    if(!tcxml_p_STag_(ctx))
        return false;   // forward error
    tcxml_string_t start_tag = ctx->capture;

    // TODO: adjust stack

    if(ctx->cbs->element_start)
    {
        assert((ctx->bufs->attrs.len & 1) == 0 && "Expected an even number of attribute elements");

        tcxml_string_t tag = tcxml_data_push_(ctx->bufs, start_tag);
        ctx->cbs->element_start(tag, ctx->bufs->attrs.ptr, ctx->bufs->attrs.len / 2, ctx->udata);
        tcxml_data_popn_(ctx->bufs, 1);
    }

    tcxml_data_reset_(ctx->bufs); // (optional)

    tcxml_text_reset_(ctx->bufs);
    if(!tcxml_px_content_(ctx))
        return false;   // forward error

    if(!tcxml_p_ETag_(ctx))
        return false;   // forward error

    if(start_tag.len != ctx->capture.len || memcmp(start_tag.ptr, ctx->capture.ptr, start_tag.len))
        return TCXML_ERROR_("Mismatched element start/end tags");

    if(ctx->cbs->element_end)
    {
        tcxml_string_t tag = tcxml_data_push_(ctx->bufs, ctx->capture);
        ctx->cbs->element_end(tag, ctx->udata);
        tcxml_data_popn_(ctx->bufs, 1);
    }

    return true;    // return nothing
}

/*
STag            ::= '<' Name (S Attribute)* S? '>'
Attribute       ::= Name Eq AttValue
*/
static bool tcxml_p_Attribute_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_p_Name_(ctx))
        return TCXML_ERROR_("Expected attribute name");
    tcxml_string_t name = tcxml_data_push_(ctx->bufs, ctx->capture);

    if(!tcxml_p_Eq_(ctx))
        return false;   // forward error

    if(!tcxml_p_AttValue_(ctx))
        return false;   // forward error
    tcxml_string_t value = tcxml_data_pushpn_(ctx->bufs, ctx->bufs->text_buf.ptr, ctx->bufs->text_buf.len);

    tcxml_string_t* attr = TCXML_ARR_APPENDN_(&ctx->bufs->attrs, 2);
    attr[0] = name;
    attr[1] = value;

    return true;
}
static bool tcxml_p_STag_(struct tcxml_parse_context_* restrict ctx)
{
    char* ptr;

    if(!tcxml_match_("<", ctx))
        return TCXML_ERROR_("Expected '<' at start of element");

    if(!tcxml_p_Name_(ctx))
        return TCXML_ERROR_("Expected XML element tag");
    tcxml_string_t name = ctx->capture;

    ctx->bufs->attrs.len = 0; // reset # of attributes
    for(;;)
    {
        ptr = ctx->ptr;

        if(!tcxml_p_S_(ctx))
            break;

        if(!tcxml_p_Attribute_(ctx))
            break;
    }
    ctx->ptr = ptr; // restore ctx->ptr to just before `S Attribute`

    tcxml_p_S_(ctx);

    if(!tcxml_match_(">", ctx))
        return TCXML_ERROR_("Expected '>' to end element");

    ctx->capture = name;
    return true;    // return name in capture
}

/*
ETag            ::= '</' Name S? '>'
*/
static bool tcxml_p_ETag_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_match_("</", ctx))
        return TCXML_ERROR_("Expected </ for element end");

    if(!tcxml_p_Name_(ctx))
        return TCXML_ERROR_("Expected XML element tag");

    tcxml_p_S_(ctx);

    if(!tcxml_match_(">", ctx))
        return TCXML_ERROR_("Expected > to end element");

    return true;    // forward Name in capture
}

/*
content         ::= CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*
*/
static bool tcxml_px_content_(struct tcxml_parse_context_* restrict ctx)
{
    tcxml_text_reset_(ctx->bufs);
    tcxml_p_CharData_(ctx);
    if(ctx->bufs->text_buf.len && ctx->cbs->text)
    {
        size_t body_head = tcxml_measure_wspace_left_((tcxml_string_t){ ctx->bufs->text_buf.len, ctx->bufs->text_buf.ptr });
        size_t body_tail = body_head != ctx->bufs->text_buf.len ? ctx->bufs->text_buf.len - tcxml_measure_wspace_right_((tcxml_string_t){ ctx->bufs->text_buf.len, ctx->bufs->text_buf.ptr }) : body_head;
        ctx->cbs->text(tcxml_text_finish_(ctx->bufs), body_head, body_tail, ctx->udata);
    }

    for(;;)
    {
        char* ptr = ctx->ptr;

        if(tcxml_px_element_(ctx))
            goto alt_parse_ok;
        ctx->ptr = ptr;

        if(tcxml_p_Reference_(ctx))
            goto alt_parse_ok;
        ctx->ptr = ptr;

        if(tcxml_px_CDSect_(ctx))
            goto alt_parse_ok;
        ctx->ptr = ptr;

        if(tcxml_px_PI_(ctx))
            goto alt_parse_ok;
        ctx->ptr = ptr;

        if(tcxml_px_Comment_(ctx))
            goto alt_parse_ok;
        ctx->ptr = ptr;

        break;  // none of these options were valid

alt_parse_ok:
        tcxml_text_reset_(ctx->bufs);
        tcxml_p_CharData_(ctx);
        if(ctx->bufs->text_buf.len && ctx->cbs->text)
        {
            size_t body_head = tcxml_measure_wspace_left_((tcxml_string_t){ ctx->bufs->text_buf.len, ctx->bufs->text_buf.ptr });
            size_t body_tail = body_head != ctx->bufs->text_buf.len ? ctx->bufs->text_buf.len - tcxml_measure_wspace_right_((tcxml_string_t){ ctx->bufs->text_buf.len, ctx->bufs->text_buf.ptr }) : body_head;
            ctx->cbs->text(tcxml_text_finish_(ctx->bufs), body_head, body_tail, ctx->udata);
        }
    }

    return true;    // always ok, as `content` can be empty
}

/*
EmptyElemTag    ::= '<' Name (S Attribute)* S? '/>'
*/
static bool tcxml_p_EmptyElemTag_(struct tcxml_parse_context_* restrict ctx)
{
    char* ptr;

    if(!tcxml_match_("<", ctx))
        return TCXML_ERROR_("Expected '<' for start of element tag");

    if(!tcxml_p_Name_(ctx))
        return TCXML_ERROR_("Expected XML element tag");
    tcxml_string_t name = ctx->capture;

    ctx->bufs->attrs.len = 0; // reset # of attributes
    for(;;)
    {
        ptr = ctx->ptr;

        if(!tcxml_p_S_(ctx))
            break;

        if(!tcxml_p_Attribute_(ctx))
            break;
    }
    ctx->ptr = ptr; // restore ctx->ptr to just before `S Attribute`

    tcxml_p_S_(ctx);

    if(!tcxml_match_("/>", ctx))
        return TCXML_ERROR_("Expected '/>' to end empty element tag");

    ctx->capture = name;
    return true;    // return name in capture
}

/*
CharRef         ::= '&#' [0-9]+ ';' |
                    '&#x' [0-9a-fA-F]+ ';'
*/
static bool tcxml_p_CharRef_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_match_("&#", ctx))
        return TCXML_ERROR_("Expected '&#' for start of character reference");

    bool is_hex = tcxml_match_("x", ctx);

    char* head = ctx->ptr;
    ctx->ptr += strspn(ctx->ptr, &"ABCDEFabcdef0123456789"[is_hex ? 0 : sizeof("ABCDEFabcdef") - 1]);
    if(ctx->ptr == head)
        return TCXML_ERROR_("Expected hexadecimal digits for character reference");

    // get a \0-terminated copy of the number
    tcxml_string_t num = tcxml_data_push_(ctx->bufs, (tcxml_string_t){ ctx->ptr - head, head });
    uint32_t code_point = strtoul(num.ptr, NULL, is_hex ? 16 : 10);
    tcxml_data_popn_(ctx->bufs, 1);

    // and finally, append the actual number
    size_t tlen = ctx->bufs->text_buf.len;
    char* utf8 = TCXML_ARR_APPENDN_(&ctx->bufs->text_buf, 4);             // 1: reserve 4 bytes (UTF-8 length)
    size_t nbytes = tcxml_utf8_from_utf32(utf8, code_point);        // 2: convert
    ctx->bufs->text_buf.len = tlen + nbytes;                              // 3: adjust length (dropping any unused bytes)
    if(!nbytes)
        return TCXML_ERROR_("Invalid Unicode code point in character reference");

    if(!tcxml_match_(";", ctx))
        return TCXML_ERROR_("Expected ';' to end character reference");

    return true;    // result in text_buf
}

/*
Reference       ::= EntityRef | CharRef
EntityRef       ::= '&' Name ';'
*/
static bool tcxml_p_EntityRef_(struct tcxml_parse_context_* restrict ctx)
{
    char* head = ctx->ptr;

    if(!tcxml_match_("&", ctx))
        return TCXML_ERROR_("Expected start of entity reference");

    if(!tcxml_p_Name_(ctx))
        return TCXML_ERROR_("Expected name of entity reference");

    if(!tcxml_match_(";", ctx))
        return TCXML_ERROR_("Expected end of entity reference");

    char* tail = ctx->ptr;

    tcxml_string_t replacement;

    // TODO performance: Use switch statement, perfect hashing, or similar to quickly identify known strings.
    if(!tcxml_string_cmpz_(ctx->capture, "amp"))
        replacement = (tcxml_string_t){ 1, "&" };
    else if(!tcxml_string_cmpz_(ctx->capture, "lt"))
        replacement = (tcxml_string_t){ 1, "<" };
    else if(!tcxml_string_cmpz_(ctx->capture, "gt"))
        replacement = (tcxml_string_t){ 1, ">" };
    else if(!tcxml_string_cmpz_(ctx->capture, "apos"))
        replacement = (tcxml_string_t){ 1, "'" };
    else if(!tcxml_string_cmpz_(ctx->capture, "quot"))
        replacement = (tcxml_string_t){ 1, "\"" };
    else if(ctx->cbs->unknown_entity_reference)
    {
        // default replacement is simply the entire entity reference, e.g. &foo;
        replacement = tcxml_data_push_(ctx->bufs, (tcxml_string_t){ tail - head, head });
        tcxml_string_t ref = tcxml_data_push_(ctx->bufs, ctx->capture);
        bool ok = ctx->cbs->unknown_entity_reference(&replacement, ref, ctx->udata);
        tcxml_data_popn_(ctx->bufs, 2);
        if(!ok)
            return TCXML_ERROR_("Unknown entity reference (extended)");
    }
    else
        return TCXML_ERROR_("Unknown entity reference");

    // NOTE: We've (potentially) used tcxml_data_popn_ above, but we know it's safe to use the "dangling buffer" replacement here,
    // as popn_ never frees memory.
    tcxml_text_append_(ctx->bufs, replacement, false);

    return true;    // result in text_buf
}
static bool tcxml_p_Reference_(struct tcxml_parse_context_* restrict ctx)
{
    char* ptr = ctx->ptr;

    if(tcxml_p_EntityRef_(ctx))
        return true;    // forward return
    ctx->ptr = ptr;

    if(tcxml_p_CharRef_(ctx))
        return true;    // forward return

    return TCXML_ERROR_("Expected &...; reference");
}

/*
EncodingDecl    ::= S 'encoding' Eq ('"' EncName '"' | "'" EncName "'" )
EncName         ::= [A-Za-z] ([A-Za-z0-9._] | '-')*
*/
static bool tcxml_p_EncName_(struct tcxml_parse_context_* restrict ctx)
{
    char* head = ctx->ptr;

    if(!('A' <= *ctx->ptr && *ctx->ptr <= 'Z')
    && !('a' <= *ctx->ptr && *ctx->ptr <= 'z'))
        return TCXML_ERROR_("Expected start of encoding name");
    ++ctx->ptr;

    while(  ('A' <= *ctx->ptr && *ctx->ptr <= 'Z')
        ||  ('a' <= *ctx->ptr && *ctx->ptr <= 'z')
        ||  ('0' <= *ctx->ptr && *ctx->ptr <= '9')
        ||  *ctx->ptr == '.'
        ||  *ctx->ptr == '_'
        ||  *ctx->ptr == '-' )
        ++ctx->ptr;

    return TCXML_CAPTURE_(head,ctx->ptr);
}
static bool tcxml_p_EncodingDecl_(struct tcxml_parse_context_* restrict ctx)
{
    if(!tcxml_p_S_(ctx))
        return false;   // forward error
    if(!tcxml_match_("encoding", ctx))
        return TCXML_ERROR_("Expected `encoding=...` declaration attribute");
    if(!tcxml_p_Eq_(ctx))
        return false;   // forward error

    char quot = *ctx->ptr;
    if(quot != '\'' && quot != '"')
        return TCXML_ERROR_("Expected start of quoted string");
    ++ctx->ptr; // quot

    if(!tcxml_p_EncName_(ctx))
        return false;   // forward error

    if(*ctx->ptr != quot)
        return TCXML_ERROR_("Expected end of quoted string");
    ++ctx->ptr; // quot

    return true;    // return EncName in capture
}

tcxml_error_t tcxml_sax_process(tcxml_sax_buffers_t* bufs, const char* src, const tcxml_sax_callbacks_t* cbs, void* udata)
{
    tcxml_sax_buffers_t bufs_tmp;
    if(!bufs)
        bufs = tcxml_sax_buffers_init(&bufs_tmp);
    struct tcxml_parse_context_ ctx = {
        .bufs = bufs,
        .cbs = cbs,
        .udata = udata,

        .str = (char*)src,
        .ptr = (char*)src,
    };

    if(cbs->start)
        cbs->start(udata);

    bool has_bom = tcxml_starts_with_(ctx.ptr, "\xEF\xBB\xBF");
    if(has_bom)
        ctx.ptr += 3;   // skip UTF-8 BOM

    if(tcxml_px_document_(&ctx))
    {
        if(*ctx.ptr == 0)
            ctx.error.message = NULL;                                                 // document parsed OK, clear error message
        else
            ctx.error = tcxml_make_error_(&ctx, "Expected end-of-file"); // we have some trailing data that wasn't parsed
    }

    // compute error offset if we had an error
    if(ctx.error.message)
    {
        ctx.error.line = ctx.error.column = 0;
        tcxml_advance_line_col_(ctx.str + (has_bom ? 3 : 0), ctx.str + ctx.error.offset, &ctx.error);
    }
    else if(cbs->end)
        cbs->end(udata);

    if(bufs == &bufs_tmp)
        tcxml_sax_buffers_deinit(&bufs_tmp);
    return ctx.error;
}

#endif /* TC_XML_IMPLEMENTATION */
