#define TC_XML_IMPLEMENTATION
#include "../tc_xml.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
static void set_utf8_io(void)
{
    //SetConsoleCP(CP_UTF8);    // (don't need this, and apparently it's broken for CP_UTF8 anyhow)
    SetConsoleOutputCP(CP_UTF8);
}
#else
static void set_utf8_io(void)
{
}
#endif

// colors
#define C_RESET     "\33[0m"
#define C_TAG       "\33[35m"
#define C_NAME      "\33[33m"
#define C_QUOTED    "\33[32m"
#define C_CDATA     "\33[4;34m"
#define C_COMMENT   "\33[90m"
#define C_PI        "\33[31m"
#define C_PI_DATA   "\33[34m"
#define C_TEXT      "\33[4;36m"
#define C_WSPACE    "\33[0m"//"\33[41m"
#define C_DEBUG     "\33[90m"

static void cb_start(void* udata)
{
    printf(C_DEBUG "---------- SOF ----------" C_RESET "\n");
}
static void cb_end(void* udata)
{
    printf("\n" C_DEBUG "---------- EOF ----------" C_RESET "\n");
}

static void cb_xml_decl(tcxml_string_t version, tcxml_string_t encoding, bool* standalone, void* udata)
{
    printf("<?" C_PI "xml" C_RESET " version=\"" C_QUOTED "%s" C_RESET "\" encoding=\"" C_QUOTED "%s" C_RESET "\"", version.ptr, encoding.ptr);
    if(standalone)
        printf(" standalone=\"" C_QUOTED "%s" C_RESET "\"", *standalone ? "yes" : "no");
    printf("?>\n");
}
static void cb_cdata(tcxml_string_t data, void* udata)
{
    printf("<![CDATA[" C_CDATA "%s" C_RESET "]]>", data.ptr);
}
static void cb_text(tcxml_string_t text, size_t body_head, size_t body_tail, void* udata)
{
    printf(C_WSPACE "%.*s" C_RESET C_TEXT "%.*s" C_RESET C_WSPACE "%.*s" C_RESET,
        (unsigned int)body_head, text.ptr,
        (unsigned int)(body_tail - body_head), text.ptr + body_head,
        (unsigned int)(text.len - body_tail), text.ptr + body_tail
    );
}
static void cb_element_start(tcxml_string_t tag, tcxml_string_t* attrs, size_t nattrs, void* udata)
{
    printf("<" C_TAG "%s" C_RESET, tag.ptr);
    for(size_t i = 0; i < nattrs; i++)
        printf(" " C_NAME "%s" C_RESET "=\"" C_QUOTED "%s" C_RESET "\"", attrs[2*i+0].ptr, attrs[2*i+1].ptr);
    printf(">");
}
static void cb_element_end(tcxml_string_t tag, void* udata)
{
    printf("</" C_TAG "%s" C_RESET ">", tag.ptr);
}
static void cb_processing_instruction(tcxml_string_t target, tcxml_string_t text, void* udata)
{
    printf("<?" C_PI "%s" C_RESET " " C_PI_DATA "%s" C_RESET "?>", target.ptr, text.ptr);
}
static void cb_comment(tcxml_string_t text, void* udata)
{
    printf("<!--" C_COMMENT "%s" C_RESET "-->", text.ptr);
}

static bool cb_unknown_entity_reference(tcxml_string_t* replacement, tcxml_string_t ref, void* udata)
{
    //printf("{EREF %s}", ref.ptr);
    *replacement = (tcxml_string_t){ 3, "-R-" };
    return true;
}

/*static void cb_ignorable_whitespace(tcxml_string_t text, void* udata)
{
    printf("%s", text.ptr);
}*/

void usage(FILE* file, int ecode)
{
    fprintf(file, "Usage: tcxml <xmlfile>\n");
    exit(ecode);
}
int main(int argc, char** argv)
{
    set_utf8_io();
    system("");

    if(argc < 2)
        usage(stderr, 2);

    const char* fname = argv[1];

    static const tcxml_sax_callbacks_t callbacks = {
        .start = cb_start,
        .end = cb_end,

        .xml_decl = cb_xml_decl,
        .cdata = cb_cdata,
        .text = cb_text,
        .element_start = cb_element_start,
        .element_end = cb_element_end,
        .processing_instruction = cb_processing_instruction,
        .comment = cb_comment,

        .unknown_entity_reference = cb_unknown_entity_reference,

        //.ignorable_whitespace = cb_ignorable_whitespace,
    };

    FILE* file = fopen(fname, "rb");
    assert(file);

    static char src[1U*1024U*1024U];
    size_t len = fread(src, 1, sizeof(src) - 1U, file);
    src[len] = 0;

    tcxml_error_t error = tcxml_sax_process(NULL, src, &callbacks, NULL);
    fflush(stdout);
    if(!TCXML_ERROR_IS_OK(error))
        fprintf(stderr, "Error [%u:%u]: %s\n", (unsigned int)error.line + 1, (unsigned int)error.column + 1, error.message);

    fclose(file);

    return 0;
}
