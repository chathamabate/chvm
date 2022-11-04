#ifndef CORE_LOG_H
#define CORE_LOG_H

#include <stdio.h>

#define CORE_LOG_LEVEL 3

// Here are the three different logging
// macros. (E for error, W for warn, N for note)

#define E(fs, ...) printf("E : " fs "\n", __VA_ARGS__)
#define W(fs, ...) printf("W : " fs "\n", __VA_ARGS__)
#define N(fs, ...) printf("N : " fs "\n", __VA_ARGS__)

// Log level 3 enables all log types.

#if CORE_LOG_LEVEL == 2
    #undef N
    #define N(fs, ...)
#elif CORE_LOG_LEVEL == 1
    #undef W
    #define W(fs, ...)

    #undef N
    #define N(fs, ...)
#elif CORE_LOG_LEVEL == 0
    #undef E
    #define E(fs, ...)

    #undef W
    #define W(fs, ...)

    #undef N
    #define N(fs, ...)
#endif

#define CC_RESET        "\x1b[0m"
#define CC_BOLD         "\x1b[1m"
#define CC_FAINT        "\x1b[2m"
#define CC_ITALIC       "\x1b[3m"
#define CC_UNDERLINED   "\x1b[4m"

#define CC_BLACK        "\x1b[30m"
#define CC_RED          "\x1b[31m"
#define CC_GREEN        "\x1b[32m"
#define CC_YELLOW       "\x1b[33m"
#define CC_BLUE         "\x1b[34m"
#define CC_MAGENTA      "\x1b[35m"
#define CC_CYAN         "\x1b[36m"
#define CC_WHITE        "\x1b[37m"

#define CC_BG_BLACK     "\x1b[40m"
#define CC_BG_RED       "\x1b[41m"
#define CC_BG_GREEN     "\x1b[42m"
#define CC_BG_YELLOW    "\x1b[43m"
#define CC_BG_BLUE      "\x1b[44m"
#define CC_BG_MAGENTA   "\x1b[45m"
#define CC_BG_CYAN      "\x1b[46m"
#define CC_BG_WHITE     "\x1b[47m"

#define CC_BRIGHT_BLACK         "\x1b[90m"
#define CC_BRIGHT_RED           "\x1b[91m"
#define CC_BRIGHT_GREEN         "\x1b[92m"
#define CC_BRIGHT_YELLOW        "\x1b[93m"
#define CC_BRIGHT_BLUE          "\x1b[94m"
#define CC_BRIGHT_MAGENTA       "\x1b[95m"
#define CC_BRIGHT_CYAN          "\x1b[96m"
#define CC_BRIGHT_WHITE         "\x1b[97m"

#define CC_BRIGHT_BG_BLACK      "\x1b[100m"
#define CC_BRIGHT_BG_RED        "\x1b[101m"
#define CC_BRIGHT_BG_GREEN      "\x1b[102m"
#define CC_BRIGHT_BG_YELLOW     "\x1b[103m"
#define CC_BRIGHT_BG_BLUE       "\x1b[104m"
#define CC_BRIGHT_BG_MAGENTA    "\x1b[105m"
#define CC_BRIGHT_BG_CYAN       "\x1b[106m"
#define CC_BRIGHT_BG_WHITE      "\x1b[107m"

#endif
