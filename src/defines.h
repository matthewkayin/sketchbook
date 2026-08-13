#pragma once

// App name
#define SBK_APP_NAME "Sketchbook"

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define SBK_PLATFORM_WINDOWS 1
    #ifndef _WIN64
        #error "64-bit is required"
    #endif
    #define SBK_PLATFORM_STR "Windows"
#elif defined(__linux__) || defined(__gnu_linux__)
    #define SBK_PLATFORM_LINUX 1
    #define SBK_PLATFORM_STR "Linux"
#elif __APPLE__
    #define SBK_PLATFORM_MACOS 1
    #define SBK_PLATFORM_STR "MacOS"
#endif

// Build string
#ifdef SBK_DEBUG
    #define SBK_BUILD_STR "Debug"
#else
    #define SBK_BUILD_STR "Release"
#endif

// Window size
#define SBK_SCREEN_WIDTH 1280
#define SBK_SCREEN_HEIGHT 720

// Util macro
#define array_length(array) (sizeof(array) / sizeof(array[0]))
