#include "scanner.h"
// This is also a test.

#include <stdlib.h>

#define SINGLETON(tok, tt) \
const chasm_token tok = { \
    .type = (tt), \
    .data = NULL \
}

// All singleton tokens defined.
SINGLETON(TOK_OR_EQ, TT_OR_EQ);
SINGLETON(TOK_AND_EQ, TT_AND_EQ);
SINGLETON(TOK_XOR_EQ, TT_XOR_EQ);
SINGLETON(TOK_LSHIFT_EQ, TT_LSHIFT_EQ);
SINGLETON(TOK_RSHIFT_EQ, TT_RSHIFT_EQ);
SINGLETON(TOK_PLUS_EQ, TT_PLUS_EQ);
SINGLETON(TOK_MINUS_EQ, TT_MINUS_EQ);
SINGLETON(TOK_TIMES_EQ, TT_TIMES_EQ);
SINGLETON(TOK_DIV_EQ, TT_DIV_EQ);
SINGLETON(TOK_EQ, TT_EQ);
SINGLETON(TOK_LT, TT_LT);
SINGLETON(TOK_LT_EQ, TT_LT_EQ);
SINGLETON(TOK_ASSIGN, TT_ASSIGN);
SINGLETON(TOK_POP, TT_POP);
SINGLETON(TOK_PUSH, TT_PUSH);
SINGLETON(TOK_SP, TT_SP);
SINGLETON(TOK_CMPL, TT_CMPL);
SINGLETON(TOK_SYSCALL, TT_SYSCALL);
SINGLETON(TOK_JMP, TT_JMP);
SINGLETON(TOK_CALL, TT_CALL);
SINGLETON(TOK_MEM, TT_MEM);
SINGLETON(TOK_RO, TT_RO);
SINGLETON(TOK_IF, TT_IF);
SINGLETON(TOK_RET, TT_RET);
SINGLETON(TOK_EXIT, TT_EXIT);
SINGLETON(TOK_BEGIN, TT_BEGIN);
SINGLETON(TOK_END, TT_END);
SINGLETON(TOK_PASTE, TT_PASTE);
SINGLETON(TOK_REPEAT, TT_REPEAT);
SINGLETON(TOK_U, TT_U);
SINGLETON(TOK_D, TT_D);
SINGLETON(TOK_LBRACK, TT_LBRACK);
SINGLETON(TOK_RBRACK, TT_RBRACK);
SINGLETON(TOK_SEMI_COLON, TT_SEMI_COLON);
SINGLETON(TOK_NOT, TT_NOT);

chasm_scanner *new_chasm_scanner(void *g, char_gen nc) {
    chasm_scanner *cs = malloc(sizeof(chasm_scanner));

    cs->generator = g;
    cs->next_char = nc;

    // Next character should always be loaded into the look ahead.
    cs->lah = nc(g);

    if (cs->lah == '\0') {
        cs->state = CSS_CLOSED;
    } else {
        cs->state = CSS_READY;
    }

    return cs;
}

