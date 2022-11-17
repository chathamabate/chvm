#ifndef CORE_LOG_H
#define CORE_LOG_H

#include <unistd.h>
#include <stdio.h>

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

// UC for unicode.

#define UC_STAR             "\u203B"
#define UC_UP_ARR           "\u21E7"   
#define UC_CIRCLE_DIV       "\u2298"
#define UC_SHIP_WHEEL       "\u2388"
#define UC_FISHEYE          "\u25C9"
#define UC_HALF_CIRCLE      "\u25D0"

#define UC_VERTICAL_LINE    "\u2503"
#define UC_HORIZ_LINE       "\u2501"

#define UC_TL_CORNER        "\u250F"
#define UC_TR_CORNER        "\u2513"
#define UC_BR_CORNER        "\u251B"
#define UC_BL_CORNER        "\u2517"

// EJ for emoji
#define EJ_CHECK            "\u2705"
#define EJ_CROSS            "\u274C"

#define EJ_ORANGE_CIRCLE    "\U0001F7E0"

#define CORE_LOG_PID_PREFIX "(%d) "

#define CORE_LOG_S_PREFIX "[" CC_BRIGHT_MAGENTA "S" CC_RESET "] " CC_MAGENTA 
#define CORE_LOG_W_PREFIX "[" CC_BRIGHT_BLUE "W" CC_RESET "] " CC_BLUE
#define CORE_LOG_N_PREFIX "[" CC_BOLD "N" CC_RESET "] "  

#define CORE_LOG_S(fs) CORE_LOG_PID_PREFIX CORE_LOG_S_PREFIX fs CC_RESET "\n"
#define CORE_LOG_W(fs) CORE_LOG_PID_PREFIX CORE_LOG_W_PREFIX fs CC_RESET "\n"
#define CORE_LOG_N(fs) CORE_LOG_PID_PREFIX CORE_LOG_N_PREFIX fs CC_RESET "\n"

#define CORE_LOG_LEVEL 2

// Here are the three different logging
// macros. (S for scream, W for warn, N for note)

#define S(fs) printf(CORE_LOG_S(fs), getpid())
#define W(fs) printf(CORE_LOG_W(fs), getpid())
#define N(fs) printf(CORE_LOG_N(fs), getpid())

#define Sf(fs, ...) printf(CORE_LOG_S(fs), getpid(), __VA_ARGS__)
#define Wf(fs, ...) printf(CORE_LOG_W(fs), getpid(), __VA_ARGS__)
#define Nf(fs, ...) printf(CORE_LOG_N(fs), getpid(), __VA_ARGS__)

#if CORE_LOG_LEVEL < 1
    #undef S
    #define S(fs)
    
    #undef Sf
    #define Sf(fs, ...)
#endif

#if CORE_LOG_LEVEL < 2
    #undef W
    #define W(fs)

    #undef Wf
    #define Wf(fs, ...)
#endif

#if CORE_LOG_LEVEL < 3
    #undef N
    #define N(fs)

    #undef Nf
    #define Nf(fs, ...)
#endif


#endif
