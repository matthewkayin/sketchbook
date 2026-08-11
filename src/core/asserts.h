#pragma once

#include "defines.h"

#ifdef SBK_DEBUG

// Defined in logger.cpp
void logger_report_assertion_failure(
    const char* expression,
    const char* message,
    const char* file,
    int line);

#if _MSC_VER
    #include <intrin.h>
    #define sbk_debug_break() __debugbreak()
#else
    #define sbk_debug_break() __builtin_trap()
#endif

#define SBK_ASSERT(expr)                                                       \
    {                                                                          \
        if (expr) {                                                            \
        } else {                                                               \
            logger_report_assertion_failure(#expr, "", __FILE__, __LINE__);    \
            sbk_debug_break();                                                 \
        }                                                                      \
    }

#define SBK_ASSERT_MESSAGE(expr, message)                                        \
    {                                                                            \
        if (expr) {                                                              \
        } else {                                                                 \
            logger_report_assertion_failure(#expr, message, __FILE__, __LINE__); \
            sbk_debug_break();                                                   \
        }                                                                        \
    }

#else

#define SBK_ASSERT(expr)
#define SBK_ASSERT_MESSAGE(expr, message)

#endif
