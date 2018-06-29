/*
 * tc_terminal.h: Operating system terminal abstraction layer.
 *
 * DEPENDS:
 * VERSION: 0.0.3 (2016-09-11)
 * LICENSE: CC0 & Boost (dual-licensed)
 * AUTHOR: Tim Cas
 * URL: https://github.com/darkuranium/tclib
 *
 * VERSION HISTORY:
 * 0.0.3    added TC_MALLOC and friends
 * 0.0.2    made the library compile with a C++ compiler
 * 0.0.1    initial public release
 *
 * TODOs:
 * - add TCTERM_{FG,BG}_DEFAULT
 * - add proper UTF-8 support
 * - (opt?) add support for bold/bright in POSIX
 * - add proper checks for terminal colors
 */

#ifndef TC_TERMINAL_H_
#define TC_TERMINAL_H_

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCTERM_FG_RED       0x01
#define TCTERM_FG_GREEN     0x02
#define TCTERM_FG_BLUE      0x04
#define TCTERM_FG_INTENSE   0x08
#define TCTERM_BG_RED       0x10
#define TCTERM_BG_GREEN     0x20
#define TCTERM_BG_BLUE      0x40
#define TCTERM_BG_INTENSE   0x80

#define TCTERM_FG_DEFAULT   0x0100
#define TCTERM_BG_DEFAULT   0x1000

/*
#define TCTERM_FG_STARTING
#define TCTERM_BG_STARTING
 */

/* extras */
#define TCTERM_FG_BLACK     0x00
#define TCTERM_FG_MAGENTA   (TCTERM_FG_RED | TCTERM_FG_BLUE)
#define TCTERM_FG_YELLOW    (TCTERM_FG_RED | TCTERM_FG_GREEN)
#define TCTERM_FG_CYAN      (TCTERM_FG_GREEN | TCTERM_FG_BLUE)
#define TCTERM_FG_WHITE     (TCTERM_FG_RED | TCTERM_FG_GREEN | TCTERM_FG_BLUE)
#define TCTERM_BG_BLACK     0x00
#define TCTERM_BG_MAGENTA   (TCTERM_BG_RED | TCTERM_BG_BLUE)
#define TCTERM_BG_YELLOW    (TCTERM_BG_RED | TCTERM_BG_GREEN)
#define TCTERM_BG_CYAN      (TCTERM_BG_GREEN | TCTERM_BG_BLUE)
#define TCTERM_BG_WHITE     (TCTERM_BG_RED | TCTERM_BG_GREEN | TCTERM_BG_BLUE)

#define TCTERM_MKI_(TYPE,R,G,B,I)    (((R) ? TCTERM_##TYPE##_RED : 0) | ((G) ? TCTERM_##TYPE##_GREEN : 0) | ((B) ? TCTERM_##TYPE##_BLUE : 0) | ((I) ? TCTERM_##TYPE##_INTENSE : 0))

#define TCTERM_FGI(R,G,B,I) TCTERM_MKI_(FG,R,G,B,I)
#define TCTERM_FG(R,G,B)    TCTERM_FGI(R,G,B,0)
#define TCTERM_BGI(R,G,B,I) TCTERM_MKI_(BG,R,G,B,I)
#define TCTERM_BG(R,G,B)    TCTERM_BGI(R,G,B,0)

#define TCTERM_STDIN    0
#define TCTERM_STDOUT   1
#define TCTERM_STDERR   2

#define TCTERM_EOF -1

enum TC_TermKeys
{
    TCTERM_KEY_UP = 256,
    TCTERM_KEY_DOWN,
    TCTERM_KEY_RIGHT,
    TCTERM_KEY_LEFT,

    TCTERM_KEY_INSERT,
    TCTERM_KEY_DELETE,
    TCTERM_KEY_HOME,
    TCTERM_KEY_END,
    TCTERM_KEY_PAGE_UP,
    TCTERM_KEY_PAGE_DOWN,

    TCTERM_KEY_BACKSPACE
};

int tcterm_init_stdio(void);
int tcterm_deinit(void);

int tcterm_set_mode_raw(void);
int tcterm_set_mode_default(void);

int tcterm_is_tty_file(FILE* file);

int tcterm_set_attr(int attr);
int tcterm_set_attr_default(void);

int tcterm_get_cursor_pos(int* x, int* y);
int tcterm_set_cursor_pos(int x, int y);

/*int tcterm_get_cursor_vis(int* visible);*/
int tcterm_set_cursor_vis(int visible);

int tcterm_clear_to_eol(int stream);

int tcterm_getc(void);

int tcterm_fprint(int stream, const char* str, int len);
int tcterm_fprintln(int stream, const char* str, int len);
int tcterm_print(const char* str, int len);
int tcterm_println(const char* str, int len);

int tcterm_vfprintf(int stream, const char* format, va_list args);
int tcterm_fprintf(int stream, const char* format, ...);
int tcterm_vprintf(const char* format, va_list args);
int tcterm_printf(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* TC_TERMINAL_H_ */

#ifdef TC_TERMINAL_IMPLEMENTATION
#undef TC_TERMINAL_IMPLEMENTATION

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else /* POSIX */
#include <termios.h>
#include <unistd.h> /* currently for *_FILENO */
#endif

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
#ifndef TC_FREE
#define TC_FREE(ptr)            free(ptr)
#endif /* TC_FREE */

static struct
{
    int initcount;
#ifdef _WIN32
    HANDLE wh[3];
    WORD defattr;
    DWORD defmode, mode;
    CONSOLE_CURSOR_INFO cinfo;
#else /* POSIX */
    int fh[3];
    struct termios deftc, tc;
    int hascol, isutf;
#endif
} tcterm__ctx;

#ifdef _WIN32
static int tcterm__init_handles(HANDLE win, HANDLE wout, HANDLE werr)
{
    tcterm__ctx.wh[TCTERM_STDIN] = win;
    tcterm__ctx.wh[TCTERM_STDOUT] = wout;
    tcterm__ctx.wh[TCTERM_STDERR] = werr;

    CONSOLE_SCREEN_BUFFER_INFO info;
    BOOL ok;
    ok = GetConsoleScreenBufferInfo(wout, &info);
    if(!ok) return 0;
    tcterm__ctx.defattr = info.wAttributes;
    ok = GetConsoleMode(win, &tcterm__ctx.defmode);
    tcterm__ctx.mode = tcterm__ctx.defmode;
    if(!ok) return 0;
    ok = GetConsoleCursorInfo(wout, &tcterm__ctx.cinfo);
    if(!ok) return 0;
    return 1;
}
/*int tcterm_init_files(FILE* fin, FILE* fout, FILE* ferr)
{
    return tcterm__init_handles(
        fin ? (HANDLE)_get_osfhandle(_fileno(fin)) : INVALID_HANDLE_VALUE,
        fout ? (HANDLE)_get_osfhandle(_fileno(fout)) : INVALID_HANDLE_VALUE,
        ferr ? (HANDLE)_get_osfhandle(_fileno(ferr)) : INVALID_HANDLE_VALUE
    );
}*/
#else /* POSIX */
static int tcterm__init_fds(int fin, int fout, int ferr)
{
    tcterm__ctx.fh[TCTERM_STDIN] = fin;
    tcterm__ctx.fh[TCTERM_STDOUT] = fout;
    tcterm__ctx.fh[TCTERM_STDERR] = ferr;

    if(tcgetattr(fin, &tcterm__ctx.deftc))
        return 0;
    tcterm__ctx.tc = tcterm__ctx.deftc;

    tcterm__ctx.hascol = 0;
    if(isatty(fout))
    {
        const char* term = getenv("TERM");
        if(!strcmp(term, "xterm"))
            tcterm__ctx.hascol = TCTERM_FG_WHITE | TCTERM_BG_WHITE | TCTERM_FG_INTENSE | TCTERM_BG_INTENSE;
    }

    tcterm__ctx.isutf = 1; /* $$$ TODO: Remove assumption! $$$ */

    return 1;
}
#endif
int tcterm_init_stdio(void)
{
    if(tcterm__ctx.initcount++) return 1;
#ifdef _WIN32
    return tcterm__init_handles(GetStdHandle(STD_INPUT_HANDLE), GetStdHandle(STD_OUTPUT_HANDLE), GetStdHandle(STD_ERROR_HANDLE));
#else /* POSIX */
    /* TODO: fileno(stdout), etc ... in case user does freopen(stdout)? */
    return tcterm__init_fds(STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);
#endif
}
int tcterm_deinit(void)
{
    if(--tcterm__ctx.initcount) return 1;
#ifdef _WIN32
    BOOL ok = 1;
    ok &= SetConsoleCursorInfo(tcterm__ctx.wh[TCTERM_STDOUT], &tcterm__ctx.cinfo);
    ok &= SetConsoleMode(tcterm__ctx.wh[TCTERM_STDIN], tcterm__ctx.defmode);
    ok &= SetConsoleTextAttribute(tcterm__ctx.wh[TCTERM_STDOUT], tcterm__ctx.defattr);
    return ok;
#else /* POSIX */
    int ok = 1;
    ok &= tcterm_set_attr_default();
    ok &= !tcsetattr(tcterm__ctx.fh[TCTERM_STDIN], TCSAFLUSH, &tcterm__ctx.deftc);
    return ok;
#endif
}

#ifdef _WIN32
static int tcterm__set_mode(DWORD mode)
{
    tcterm__ctx.mode = mode;
    return SetConsoleMode(tcterm__ctx.wh[TCTERM_STDIN], mode);
}
#else /* POSIX */
static int tcterm__set_mode(tcflag_t mode)
{
    tcterm__ctx.tc.c_lflag = mode;

    /* TODO: cbreak mode, keypad? */

    /*tcterm__ctx.tc.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                            | INLCR | IGNCR | ICRNL | IXON);
    tcterm__ctx.tc.c_oflag &= ~OPOST;
    tcterm__ctx.tc.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tcterm__ctx.tc.c_cflag &= ~(CSIZE | PARENB);
    tcterm__ctx.tc.c_cflag |= CS8;*/

    /* TODO: TCSADRAIN? */
    return !tcsetattr(tcterm__ctx.fh[TCTERM_STDIN], TCSAFLUSH, &tcterm__ctx.tc);
}
#endif
int tcterm_set_mode_raw(void)
{
#ifdef _WIN32
    return tcterm__set_mode(tcterm__ctx.mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
#else /* POSIX */
    return tcterm__set_mode(tcterm__ctx.tc.c_lflag & ~(ECHO | ICANON));
#endif
}
int tcterm_set_mode_default(void)
{
#ifdef _WIN32
    return tcterm__set_mode(tcterm__ctx.defmode);
#else /* POSIX */
    return tcterm__set_mode(tcterm__ctx.deftc.c_lflag);
#endif
}

int tcterm_is_tty_file(FILE* file)
{
#ifdef _WIN32
    return _isatty(_fileno(file));
#else /* POSIX */
    return isatty(fileno(file));
#endif
}

int tcterm_set_attr(int attr)
{
#ifdef _WIN32
    WORD wattr = 0;
    if(attr & TCTERM_FG_DEFAULT)
        wattr |= tcterm__ctx.defattr & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    else
    {
        if(attr & TCTERM_FG_RED) wattr |= FOREGROUND_RED;
        if(attr & TCTERM_FG_GREEN) wattr |= FOREGROUND_GREEN;
        if(attr & TCTERM_FG_BLUE) wattr |= FOREGROUND_BLUE;
        if(attr & TCTERM_FG_INTENSE) wattr |= FOREGROUND_INTENSITY;
    }
    if(attr & TCTERM_BG_DEFAULT)
        wattr |= tcterm__ctx.defattr & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
    else
    {
        if(attr & TCTERM_BG_RED) wattr |= BACKGROUND_RED;
        if(attr & TCTERM_BG_GREEN) wattr |= BACKGROUND_GREEN;
        if(attr & TCTERM_BG_BLUE) wattr |= BACKGROUND_BLUE;
        if(attr & TCTERM_BG_INTENSE) wattr |= BACKGROUND_INTENSITY;
    }
    BOOL ok = 1;
    ok &= SetConsoleTextAttribute(tcterm__ctx.wh[TCTERM_STDOUT], wattr);
    ok &= SetConsoleTextAttribute(tcterm__ctx.wh[TCTERM_STDERR], wattr);
    return ok;
#else /* POSIX */
    if(!tcterm__ctx.hascol) return 2;
    int ok = 1;
    ok &= tcterm_print("\033[0", -1) > 0;
    if(!(attr & TCTERM_FG_DEFAULT))
        ok &= tcterm_printf(";%d", (attr & TCTERM_FG_INTENSE) ? 90 : 30) > 0;
    if(!(attr & TCTERM_BG_DEFAULT))
        ok &= tcterm_printf(";%d", (attr & TCTERM_BG_INTENSE) ? 100 : 40) > 0;
    ok &= tcterm_print("m", -1);
    return ok;
#endif
}
int tcterm_set_attr_default(void)
{
#ifdef _WIN32
    BOOL ok = 1;
    ok &= SetConsoleTextAttribute(tcterm__ctx.wh[TCTERM_STDOUT], tcterm__ctx.defattr);
    ok &= SetConsoleTextAttribute(tcterm__ctx.wh[TCTERM_STDERR], tcterm__ctx.defattr);
    return ok;
#else /* POSIX */
    if(!tcterm__ctx.hascol) return 2;
    return tcterm_print("\033[0m", 4) > 0;
#endif
}

int tcterm_get_cursor_pos(int* x, int* y)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    BOOL ret = GetConsoleScreenBufferInfo(tcterm__ctx.wh[TCTERM_STDOUT], &info);
    if(ret)
    {
        *x = info.dwCursorPosition.X;
        *y = info.dwCursorPosition.Y;
    }
    return ret;
#else /* POSIX */
    struct termios tmp = tcterm__ctx.tc;
    tmp.c_cflag &= ~CREAD; /* TODO: make robust (does this do what it was supposed to ...?) */
    tmp.c_lflag &= ~(ECHO | ICANON); /* $$$ TODO: set $$$ */

    *x = *y = -1;

    /* TODO: TCSANOW? */
    if(tcsetattr(tcterm__ctx.fh[TCTERM_STDIN], TCSADRAIN, &tmp))
        return 0;

    int ok = tcterm_print("\033[6n", 4) > 0;
    if(ok)
    {
        char chr[2];
        int num = fscanf(stdin, "\033[%d;%d%1[R]", y, x, chr);
        if(num < 0) perror("E");
        if(num != 3)
            ok = 0;
        else
        {
            *x -= 1;
            *y -= 1;
        }
    }

    /* TODO: TCSANOW? */
    return !tcsetattr(tcterm__ctx.fh[TCTERM_STDIN], TCSADRAIN, &tcterm__ctx.tc) && ok;
#endif
}
int tcterm_set_cursor_pos(int x, int y)
{
#ifdef _WIN32
    COORD cpos;
    cpos.X = x;
    cpos.Y = y;
    return SetConsoleCursorPosition(tcterm__ctx.wh[TCTERM_STDOUT], cpos);
#else /* POSIX */
    return tcterm_printf("\033[%d;%dH", y + 1, x + 1, -1) > 0;
#endif
}

/*int tcterm_get_cursor_vis(int* size, int* visible)
{
    CONSOLE_CURSOR_INFO cinfo;
    BOOL ret = GetConsoleCursorInfo(tcterm__ctx.wh[TCTERM_STDOUT], &cinfo);
    if(ret)
    {
        *size = cinfo.dwSize;
        *visible = cinfo.bVisible;
    }
    return ret;
}*/
int tcterm_set_cursor_vis(int visible)
{
#ifdef _WIN32
    CONSOLE_CURSOR_INFO cinfo;
    /*cinfo.dwSize = size;*/
    cinfo.bVisible = visible;
    return SetConsoleCursorInfo(tcterm__ctx.wh[TCTERM_STDOUT], &cinfo);
#else /* POSIX */
    return tcterm_printf("\033[?25%c", visible ? 'h' : 'l') > 0;
#endif
}

int tcterm_clear_to_eol(int stream)
{
    if(stream > TCTERM_STDERR) return -1;
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    BOOL ret = GetConsoleScreenBufferInfo(tcterm__ctx.wh[TCTERM_STDOUT], &info);
    if(!ret) return 0;

    DWORD nwritten;
    BOOL ok = 1;
    ok &= FillConsoleOutputCharacterW(tcterm__ctx.wh[stream], L' ', info.dwSize.X - info.dwCursorPosition.X, info.dwCursorPosition, &nwritten);
    ok &= FillConsoleOutputAttribute(tcterm__ctx.wh[stream], info.wAttributes,  info.dwSize.X - info.dwCursorPosition.X, info.dwCursorPosition, &nwritten);
    return ok;
#else /* POSIX */
    return tcterm_print("\033[K", 3);
#endif
}

#ifdef _WIN32
#else /* POSIX */
static int tcterm__get_int(int* ret, FILE* file)
{
    int c;
    int num = 0;
    for(;;)
    {
        c = getc(file);
        if(c == EOF)
        {
            /* TODO SPLICE */
            /*return NULL;*/
            c = TCTERM_EOF;
            break;
        }
        else if(!('0' <= c && c <= '9'))
            break;

        num *= 10;
        num += c - '0';
    }
    *ret = num;
    return c;
}
#endif

int tcterm_getc(void)
{
#ifdef _WIN32
    static int wlen = 0;
    static WCHAR wbuf[2];

    static int upos = 0;
    static int ulen = 0;
    static char ubuf[4];

    INPUT_RECORD input;
    DWORD numread;

    if(upos >= ulen)
    {
        while(!wlen) /* while we don't have a UTF-16 unit or pair */
        {
            WaitForSingleObject(tcterm__ctx.wh[TCTERM_STDIN], INFINITE);
            if(!ReadConsoleInputW(tcterm__ctx.wh[TCTERM_STDIN], &input, 1, &numread) || !numread)
                return TCTERM_EOF;

            switch(input.EventType)
            {
                case KEY_EVENT:
                    if(!input.Event.KeyEvent.bKeyDown)
                        break;
                    if(input.Event.KeyEvent.uChar.UnicodeChar)
                    {
                        if(input.Event.KeyEvent.uChar.UnicodeChar == '\b')
                            return TCTERM_KEY_BACKSPACE;
                        if((input.Event.KeyEvent.uChar.UnicodeChar & 0xFC00) == 0xD800)
                            wbuf[0] = input.Event.KeyEvent.uChar.UnicodeChar;
                        else if((input.Event.KeyEvent.uChar.UnicodeChar & 0xFC00) == 0xDC00)
                        {
                            wbuf[1] = input.Event.KeyEvent.uChar.UnicodeChar;
                            wlen = 2;
                        }
                        else
                        {
                            wbuf[0] = input.Event.KeyEvent.uChar.UnicodeChar;
                            wlen = 1;
                        }
                    }
                    else switch(input.Event.KeyEvent.wVirtualKeyCode)
                    {
                    case VK_UP:     return TCTERM_KEY_UP;
                    case VK_DOWN:   return TCTERM_KEY_DOWN;
                    case VK_RIGHT:  return TCTERM_KEY_RIGHT;
                    case VK_LEFT:   return TCTERM_KEY_LEFT;

                    case VK_INSERT: return TCTERM_KEY_INSERT;
                    case VK_DELETE: return TCTERM_KEY_DELETE;
                    case VK_HOME:   return TCTERM_KEY_HOME;
                    case VK_END:    return TCTERM_KEY_END;
                    case VK_PRIOR:  return TCTERM_KEY_PAGE_UP;
                    case VK_NEXT:   return TCTERM_KEY_PAGE_DOWN;
                    }
                    break;
            }
        }

        ulen = WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen, ubuf, sizeof(ubuf), NULL, NULL);
        upos = 0;
        wlen = 0;

        if(!ulen)
            return TCTERM_EOF;
    }

    return (unsigned char)ubuf[upos++];
#else /* POSIX */ /* $$$ TODO: UTF-8 $$$ */
    int num;
    int c;

    for(;;)
    {
        c = getc(stdin);
        if(c == TCTERM_EOF) return TCTERM_EOF; /* splice? */

        switch(c)
        {
        case '\033':
            c = getc(stdin);
            if(c == TCTERM_EOF) return TCTERM_EOF; /* splice? */

            switch(c)
            {
            case '[':
                c = tcterm__get_int(&num, stdin);
                if(c == TCTERM_EOF) return TCTERM_EOF; /* splice? */

                switch(c)
                {
                case 'A': return TCTERM_KEY_UP;
                case 'B': return TCTERM_KEY_DOWN;

                case 'C': return TCTERM_KEY_RIGHT;
                case 'D': return TCTERM_KEY_LEFT;

                case '~':
                    switch(num)
                    {
                    case 2: return TCTERM_KEY_INSERT;
                    case 3: return TCTERM_KEY_DELETE;
                    case 5: return TCTERM_KEY_PAGE_UP;
                    case 6: return TCTERM_KEY_PAGE_DOWN;
                    }
                    break;
                }
                break;
            case 'O':
                c = getc(stdin);
                if(c == TCTERM_EOF) return TCTERM_EOF; /* splice? */

                switch(c)
                {
                case 'H': return TCTERM_KEY_HOME;
                case 'F': return TCTERM_KEY_END;
                default: break; /* splice? */
                }
                break;
            default: /* splice? */
                break;
            }
            break;
        case '\b':
        case '\x7F':
            return TCTERM_KEY_BACKSPACE;
        default:
            if(c < 256)
                return c;
        }
    }

    return c;
#endif
}

int tcterm_fprint(int stream, const char* str, int len)
{
    if(stream > TCTERM_STDERR) return -1;
    if(len < 0) len = strlen(str);
    if(!len) return 0;

#ifdef _WIN32
    wchar_t* wstr = TC__VOID_CAST(wchar_t*,TC_MALLOC((len + 1) * sizeof(wchar_t)));

    int wlen = MultiByteToWideChar(CP_UTF8, 0, str, len, wstr, len + 1);

    if(!wlen)
    {
        TC_FREE(wstr);
        return -1;
    }

    DWORD written;
    BOOL ok = WriteConsoleW(tcterm__ctx.wh[stream], wstr, wlen, &written, NULL);
    TC_FREE(wstr);
    if(!ok) return -1;

    return written;
#else /* POSIX */
    /* $$$ TODO: UTF-8 $$$ */
    return write(tcterm__ctx.fh[stream], str, len);
#endif
}
int tcterm_fprintln(int stream, const char* str, int len)
{
    int ret1 = tcterm_fprint(stream, str, len);
    if(ret1 < 0) return -1;
    int ret2 = tcterm_fprint(stream, "\n", 1);
    if(ret2 < 0) return -1;
    return ret1 + ret2;
}
int tcterm_print(const char* str, int len)
{
    return tcterm_fprint(TCTERM_STDOUT, str, len);
}
int tcterm_println(const char* str, int len)
{
    return tcterm_fprintln(TCTERM_STDOUT, str, len);
}

int tcterm_vfprintf(int stream, const char* format, va_list args)
{
    va_list argcpy;
    va_copy(argcpy, args);
#ifdef _WIN32
    int rlen = _vscprintf(format, argcpy);
#else /* POSIX */
    int rlen = vsnprintf(NULL, 0, format, argcpy);
#endif
    va_end(argcpy);

    if(rlen < 0) return -1;

    char* buf = TC__VOID_CAST(char*,TC_MALLOC(rlen + 1));
    int alen = vsnprintf(buf, rlen + 1, format, args);

    if(rlen != alen)
    {
        TC_FREE(buf);
        return -1;
    }

    alen = tcterm_fprint(stream, buf, alen);

    TC_FREE(buf);
    return alen;
}
int tcterm_fprintf(int stream, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = tcterm_vfprintf(stream, format, args);
    va_end(args);
    return ret;
}
int tcterm_vprintf(const char* format, va_list args)
{
    return tcterm_vfprintf(TCTERM_STDOUT, format, args);
}
int tcterm_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = tcterm_vprintf(format, args);
    va_end(args);
    return ret;
}

#endif /* TC_TERMINAL_IMPLEMENTATION */
