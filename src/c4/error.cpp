#include "c4/error.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "c4/log.hpp"

#if defined(C4_XBOX) || (defined(C4_WIN) && defined(C4_MSVC))
#   include "c4/windows.hpp"
#elif defined(C4_PS4)
#   include <libdbg.h>
#elif defined(C4_UNIX) || defined(C4_LINUX)
#   include <sys/stat.h>
#   include <cstring>
#   include <fcntl.h>
#   include <unistd.h>
#endif

#if defined(C4_EXCEPTIONS_ENABLED) && defined(C4_ERROR_THROWS_EXCEPTION)
#   include <exception>
#endif

//-----------------------------------------------------------------------------
C4_BEGIN_NAMESPACE(c4)

static error_flags         s_error_flags = ON_ERROR_DEFAULTS;
static error_callback_type s_error_callback = nullptr;

//-----------------------------------------------------------------------------

error_flags get_error_flags()
{
    return s_error_flags;
}
void set_error_flags(error_flags flags)
{
    s_error_flags = flags;
}

error_callback_type get_error_callback()
{
    return s_error_callback;
}
/** Set the function which is called when an error occurs. */
void set_error_callback(error_callback_type cb)
{
    s_error_callback = cb;
}

//-----------------------------------------------------------------------------

void handle_error(srcloc where, const char *fmt, ...)
{
    char buf[1024]; // FIXME. //sstream< c4::string > ss;
    size_t msglen = 0;
    if(s_error_flags & (ON_ERROR_LOG|ON_ERROR_CALLBACK))
    {
        va_list args;
        va_start(args, fmt);
        int ilen = vsnprintf(buf, sizeof(buf), fmt, args); // ss.vprintf(fmt, args);
        va_end(args);
        msglen = ilen < (int)sizeof(buf) ? ilen : sizeof(buf)-1;
    }

    if(s_error_flags & ON_ERROR_LOG)
    {
        C4_LOGF_ERR("\n");
#if defined(C4_ERROR_SHOWS_FILELINE) && defined(C4_ERROR_SHOWS_FUNC)
        C4_LOGF_ERR("ERROR: %s:%d: %s\n", where.file, where.line, buf/*ss.c_strp()*/);
        C4_LOGF_ERR("ERROR: %s:%d: here: %s\n", where.file, where.line, where.func);
#elif defined(C4_ERROR_SHOWS_FILELINE)
        C4_LOGF_ERR("ERROR: %s:%d: %s\n", where.file, where.line, buf/*ss.c_strp()*/);
#elif ! defined(C4_ERROR_SHOWS_FUNC)
        C4_LOGF_ERR("ERROR: %s\n", buf/*ss.c_strp()*/);
#endif
        //c4::log.flush();
    }

    if(s_error_flags & ON_ERROR_CALLBACK)
    {
        if(s_error_callback)
        {
            s_error_callback(buf, msglen/*ss.c_strp(), ss.tellp()*/);
        }
    }

    if(s_error_flags & ON_ERROR_ABORT)
    {
        abort();
    }

    if(s_error_flags & ON_ERROR_THROW)
    {
#if defined(C4_EXCEPTIONS_ENABLED) && defined(C4_ERROR_THROWS_EXCEPTION)
        throw Exception(ss.c_str());
#else
        abort();
#endif
    }
}

//-----------------------------------------------------------------------------

void handle_warning(srcloc where, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[1024]; //sstream<c4::string> ss;
    vsnprintf(buf, sizeof(buf), fmt, args);//ss.vprintf(fmt, args);
    va_end(args);
    C4_LOGF_WARN("\n");
#if defined(C4_ERROR_SHOWS_FILELINE) && defined(C4_ERROR_SHOWS_FUNC)
    C4_LOGF_WARN("WARNING: %s:%d: %s\n", where.file, where.line, buf/*ss.c_strp()*/);
    C4_LOGF_WARN("WARNING: %s:%d: here: %s\n", where.file, where.line, where.func);
#elif defined(C4_ERROR_SHOWS_FILELINE)
    C4_LOGF_WARN("WARNING: %s:%d: %s\n", where.file, where.line, buf/*ss.c_strp()*/);
#elif ! defined(C4_ERROR_SHOWS_FUNC)
    C4_LOGF_WARN("WARNING: %s\n", buf/*ss.c_strp()*/);
#endif
    //c4::log.flush();
}

//-----------------------------------------------------------------------------
#ifndef NDEBUG
bool is_debugger_attached()
{
#if defined(C4_PS4)
    return (sceDbgIsDebuggerAttached() != 0);
#elif defined(C4_XBOX) || (defined(C4_WIN) && defined(C4_MSVC))
    return IsDebuggerPresent();
#elif defined(C4_UNIX) || defined(C4_LINUX)
    static bool first_call = true;
    static bool first_call_result = false;
    if(first_call)
    {
        first_call = false;
        //! @see http://stackoverflow.com/questions/3596781/how-to-detect-if-the-current-process-is-being-run-by-gdb
        //! (this answer: http://stackoverflow.com/a/24969863/3968589 )
        char buf[1024] = "";

        int status_fd = open("/proc/self/status", O_RDONLY);
        if (status_fd == -1)
        {
            return 0;
        }

        ssize_t num_read = ::read(status_fd, buf, sizeof(buf));

        if (num_read > 0)
        {
            static const char TracerPid[] = "TracerPid:";
            char *tracer_pid;

            if(num_read < 1024)
            {
                buf[num_read] = 0;
            }
            tracer_pid    = strstr(buf, TracerPid);
            if (tracer_pid)
            {
                first_call_result = !!atoi(tracer_pid + sizeof(TracerPid) - 1);
            }
        }
    }
    return first_call_result;
#else
    C4_NOT_IMPLEMENTED();
    return false;
#endif
} // is_debugger_attached()
#endif // NDEBUG

C4_END_NAMESPACE(c4)