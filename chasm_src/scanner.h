#ifndef CHASM_SRC_SCANNER_H
#define CHASM_SRC_SCANNER_H


#include <stdint.h>
// This is a test.

// Here is a header update.

// What should the scanner do exactly?
// How should it work... always from a file???

// A char generator. Should return '\0' when out 
// of characters to receive or an error has occured.
// The function should take whatever generator object
// it comes from.
typedef char (*char_gen)(void *generator);

typedef enum {
    TT_CODE_LABEL, TT_DATA_LABEL,
    TT_CONST_INT, TT_CONST_HEX,
    TT_UR, TT_DR,
    TT_OR_EQ, TT_AND_EQ, TT_XOR_EQ, TT_LSHIFT_EQ, TT_RSHIFT_EQ,
    TT_PLUS_EQ, TT_MINUS_EQ, TT_TIMES_EQ, TT_DIV_EQ, TT_MOD_EQ,
    TT_EQ, TT_LT, TT_LT_EQ,
    TT_ASSIGN,
    TT_POP, TT_PUSH, TT_SP, TT_CMPL,
    TT_SYSCALL, TT_JMP, TT_CALL,
    TT_MEM, TT_RO,
    TT_IF, TT_RET, TT_EXIT, TT_BEGIN, TT_END, TT_PASTE, TT_REPEAT,
    TT_U, TT_D,
    TT_LBRACK, TT_RBRACK, TT_SEMI_COLON, TT_NOT,
    TT_STR
} chasm_token_type;

typedef struct {
    chasm_token_type type;

    // NOTE : When data is NULL, the token is a static constant and
    // does not need to be freed after use.
    // Otherwise the token must be freed, and it's data.
    void *data;
} chasm_token;

// Here are all the static singleton tokens.
extern const chasm_token 
    TOK_OR_EQ, TOK_AND_EQ, TOK_XOR_EQ, TOK_LSHFIT_EQ, TOK_RSHIFT_EQ,
    TOK_PLUS_EQ, TOK_MINUS_EQ, TOK_TIMES_EQ, TOK_DIV_EQ, TOK_MOD_EQ,
    TOK_EQ, TOK_LT, TOK_LT_EQ,
    TOK_ASSIGN,
    TOK_POP, TOK_PUSH, TOK_SP, TOK_CMPL,
    TOK_SYSCALL, TOK_JMP, TOK_CALL,
    TOK_MEM, TOK_RO, 
    TOK_IF, TOK_RET, TOK_EXIT, TOK_BEGIN, TOK_END, TOK_PASTE, TOK_REPEAT,
    TOK_U, TOK_D,
    TOK_LBRACK, TOK_RBRACK, TOK_SEMI_COLON, TOK_NOT;

typedef enum {
    CSS_READY,

    // When characters run out from the generator
    // while scanning for a token. ERROR
    CSS_EARLY_EXHAUSTION,

    // When incorrect characters are produced by
    // the generator. ERROR
    CSS_WRONG_SYNTAX,

    // When there is nothing left to scan.
    CSS_CLOSED
} chasm_scanner_state;

typedef struct {
    void *generator;
    char_gen next_char;

    // This is the lookahead character.
    char lah;

    // Number of the line the scanner is currently on.
    uint32_t line_number;

    uint8_t state;
} chasm_scanner;

chasm_scanner *new_chasm_scanner(void *g, char_gen nc);
#define delete_chasm_scanner(cs) free(cs)


chasm_token *next_token(chasm_scanner *cs);



#endif
