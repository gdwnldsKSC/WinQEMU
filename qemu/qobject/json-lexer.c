/*
 * JSON lexer
 *
 * Copyright IBM, Corp. 2009
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 *
 */

#include "qapi/qmp/qstring.h"
#include "qapi/qmp/qlist.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qint.h"
#include "qemu-common.h"
#include "qapi/qmp/json-lexer.h"

#define MAX_TOKEN_SIZE (64ULL << 20)

/*
 * \"([^\\\"]|(\\\"\\'\\\\\\/\\b\\f\\n\\r\\t\\u[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]))*\"
 * '([^\\']|(\\\"\\'\\\\\\/\\b\\f\\n\\r\\t\\u[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]))*'
 * 0|([1-9][0-9]*(.[0-9]+)?([eE]([-+])?[0-9]+))
 * [{}\[\],:]
 * [a-z]+
 *
 */

enum json_lexer_state {
    IN_ERROR = 0,
    IN_DQ_UCODE3,
    IN_DQ_UCODE2,
    IN_DQ_UCODE1,
    IN_DQ_UCODE0,
    IN_DQ_STRING_ESCAPE,
    IN_DQ_STRING,
    IN_SQ_UCODE3,
    IN_SQ_UCODE2,
    IN_SQ_UCODE1,
    IN_SQ_UCODE0,
    IN_SQ_STRING_ESCAPE,
    IN_SQ_STRING,
    IN_ZERO,
    IN_DIGITS,
    IN_DIGIT,
    IN_EXP_E,
    IN_MANTISSA,
    IN_MANTISSA_DIGITS,
    IN_NONZERO_NUMBER,
    IN_NEG_NONZERO_NUMBER,
    IN_KEYWORD,
    IN_ESCAPE,
    IN_ESCAPE_L,
    IN_ESCAPE_LL,
    IN_ESCAPE_I,
    IN_ESCAPE_I6,
    IN_ESCAPE_I64,
    IN_WHITESPACE,
    IN_START,
};

#ifndef _MSC_VER
#define TERMINAL(state)     [0 ... 0x7F] = (state)
#define DIGITS(v)           ['0' ... '9'] = (v)
#define DIGITS_1_9(v)       ['1' ... '9'] = (v)
#define HEX_LOWER(v)        ['a' ... 'f'] = (v)
#define HEX_UPPER(v)        ['A' ... 'F'] = (v)
#define LOWER_ALPHA(v)      ['a' ... 'z'] = (v)
#define STRING_BODY_LO(v)   [1 ... 0xBF] = (v)
#define STRING_BODY_HI(v)   [0xC2 ... 0xF4] = (v)
#else
#define FILL_2(a, v)    [(a)] = (v), [(a)+1] = (v)
#define FILL_4(a, v)    FILL_2(a, v), FILL_2((a)+2, v)
#define FILL_8(a, v)    FILL_4(a, v), FILL_4((a)+4, v)
#define FILL_16(a, v)   FILL_8(a, v), FILL_8((a)+8, v)
#define FILL_32(a, v)   FILL_16(a, v), FILL_16((a)+16, v)
#define FILL_64(a, v)   FILL_32(a, v), FILL_32((a)+32, v)
#define FILL_128(a, v)  FILL_64(a, v), FILL_64((a)+64, v)
#define TERMINAL(state)     FILL_128(0, (state))                    /* [0x00 ... 0x7F] */
#define DIGITS(v)           FILL_8('0', (v)), FILL_2('8', (v))      /* ['0' ... '9']   */
#define DIGITS_1_9(v)       FILL_8('1', (v)), ['9'] = (v)           /* ['1' ... '9']   */
#define HEX_LOWER(v)        FILL_4('a', (v)), FILL_2('e', (v))      /* ['a' ... 'f']   */
#define HEX_UPPER(v)        FILL_4('A', (v)), FILL_2('E', (v))      /* ['A' ... 'F']   */
#define LOWER_ALPHA(v)      FILL_16('a', (v)), FILL_8('q', (v)), FILL_2('y', (v))  /* ['a' ... 'z'] */
#define STRING_BODY_LO(v)   FILL_128(1, (v)), FILL_32(0x81, (v)), FILL_16(0xA1, (v)), \
                            FILL_8(0xB1, (v)), FILL_4(0xB9, (v)), FILL_2(0xBD, (v)), [0xBF] = (v)  /* [0x01 ... 0xBF] */
#define STRING_BODY_HI(v)   FILL_32(0xC2, (v)), FILL_16(0xE2, (v)), FILL_2(0xF2, (v)), [0xF4] = (v)  /* [0xC2 ... 0xF4] */
#endif

/* Return whether TERMINAL is a terminal state and the transition to it
   from OLD_STATE required lookahead.  This happens whenever the table
   below uses the TERMINAL macro.  */
#define TERMINAL_NEEDED_LOOKAHEAD(old_state, terminal) \
            (json_lexer[(old_state)][0] == (terminal))

static const uint8_t json_lexer[][256] =  {
    /* double quote string */
    [IN_DQ_UCODE3] = {
        DIGITS(IN_DQ_STRING),
        HEX_LOWER(IN_DQ_STRING),
        HEX_UPPER(IN_DQ_STRING),
    },
    [IN_DQ_UCODE2] = {
        DIGITS(IN_DQ_UCODE3),
        HEX_LOWER(IN_DQ_UCODE3),
        HEX_UPPER(IN_DQ_UCODE3),
    },
    [IN_DQ_UCODE1] = {
        DIGITS(IN_DQ_UCODE2),
        HEX_LOWER(IN_DQ_UCODE2),
        HEX_UPPER(IN_DQ_UCODE2),
    },
    [IN_DQ_UCODE0] = {
        DIGITS(IN_DQ_UCODE1),
        HEX_LOWER(IN_DQ_UCODE1),
        HEX_UPPER(IN_DQ_UCODE1),
    },
    [IN_DQ_STRING_ESCAPE] = {
        ['b'] = IN_DQ_STRING,
        ['f'] =  IN_DQ_STRING,
        ['n'] =  IN_DQ_STRING,
        ['r'] =  IN_DQ_STRING,
        ['t'] =  IN_DQ_STRING,
        ['/'] = IN_DQ_STRING,
        ['\\'] = IN_DQ_STRING,
        ['\''] = IN_DQ_STRING,
        ['\"'] = IN_DQ_STRING,
        ['u'] = IN_DQ_UCODE0,
    },
    [IN_DQ_STRING] = {
        STRING_BODY_LO(IN_DQ_STRING),
        STRING_BODY_HI(IN_DQ_STRING),
        ['\\'] = IN_DQ_STRING_ESCAPE,
        ['"'] = JSON_STRING,
    },

    /* single quote string */
    [IN_SQ_UCODE3] = {
        DIGITS(IN_SQ_STRING),
        HEX_LOWER(IN_SQ_STRING),
        HEX_UPPER(IN_SQ_STRING),
    },
    [IN_SQ_UCODE2] = {
        DIGITS(IN_SQ_UCODE3),
        HEX_LOWER(IN_SQ_UCODE3),
        HEX_UPPER(IN_SQ_UCODE3),
    },
    [IN_SQ_UCODE1] = {
        DIGITS(IN_SQ_UCODE2),
        HEX_LOWER(IN_SQ_UCODE2),
        HEX_UPPER(IN_SQ_UCODE2),
    },
    [IN_SQ_UCODE0] = {
        DIGITS(IN_SQ_UCODE1),
        HEX_LOWER(IN_SQ_UCODE1),
        HEX_UPPER(IN_SQ_UCODE1),
    },
    [IN_SQ_STRING_ESCAPE] = {
        ['b'] = IN_SQ_STRING,
        ['f'] =  IN_SQ_STRING,
        ['n'] =  IN_SQ_STRING,
        ['r'] =  IN_SQ_STRING,
        ['t'] =  IN_SQ_STRING,
        ['/'] = IN_DQ_STRING,
        ['\\'] = IN_DQ_STRING,
        ['\''] = IN_SQ_STRING,
        ['\"'] = IN_SQ_STRING,
        ['u'] = IN_SQ_UCODE0,
    },
    [IN_SQ_STRING] = {
        STRING_BODY_LO(IN_SQ_STRING),
        STRING_BODY_HI(IN_SQ_STRING),
        ['\\'] = IN_SQ_STRING_ESCAPE,
        ['\''] = JSON_STRING,
    },

    /* Zero */
    [IN_ZERO] = {
        TERMINAL(JSON_INTEGER),
        DIGITS(IN_ERROR),
        ['.'] = IN_MANTISSA,
    },

    /* Float */
    [IN_DIGITS] = {
        TERMINAL(JSON_FLOAT),
        DIGITS(IN_DIGITS),
    },

    [IN_DIGIT] = {
        DIGITS(IN_DIGITS),
    },

    [IN_EXP_E] = {
        ['-'] = IN_DIGIT,
        ['+'] = IN_DIGIT,
        DIGITS(IN_DIGITS),
    },

    [IN_MANTISSA_DIGITS] = {
        TERMINAL(JSON_FLOAT),
        DIGITS(IN_MANTISSA_DIGITS),
        ['e'] = IN_EXP_E,
        ['E'] = IN_EXP_E,
    },

    [IN_MANTISSA] = {
        DIGITS(IN_MANTISSA_DIGITS),
    },

    /* Number */
    [IN_NONZERO_NUMBER] = {
        TERMINAL(JSON_INTEGER),
        DIGITS(IN_NONZERO_NUMBER),
        ['e'] = IN_EXP_E,
        ['E'] = IN_EXP_E,
        ['.'] = IN_MANTISSA,
    },

    [IN_NEG_NONZERO_NUMBER] = {
        ['0'] = IN_ZERO,
        DIGITS_1_9(IN_NONZERO_NUMBER),
    },

    /* keywords */
    [IN_KEYWORD] = {
        TERMINAL(JSON_KEYWORD),
        LOWER_ALPHA(IN_KEYWORD),
    },

    /* whitespace */
    [IN_WHITESPACE] = {
        TERMINAL(JSON_SKIP),
        [' '] = IN_WHITESPACE,
        ['\t'] = IN_WHITESPACE,
        ['\r'] = IN_WHITESPACE,
        ['\n'] = IN_WHITESPACE,
    },        

    /* escape */
    [IN_ESCAPE_LL] = {
        ['d'] = JSON_ESCAPE,
    },

    [IN_ESCAPE_L] = {
        ['d'] = JSON_ESCAPE,
        ['l'] = IN_ESCAPE_LL,
    },

    [IN_ESCAPE_I64] = {
        ['d'] = JSON_ESCAPE,
    },

    [IN_ESCAPE_I6] = {
        ['4'] = IN_ESCAPE_I64,
    },

    [IN_ESCAPE_I] = {
        ['6'] = IN_ESCAPE_I6,
    },

    [IN_ESCAPE] = {
        ['d'] = JSON_ESCAPE,
        ['i'] = JSON_ESCAPE,
        ['p'] = JSON_ESCAPE,
        ['s'] = JSON_ESCAPE,
        ['f'] = JSON_ESCAPE,
        ['l'] = IN_ESCAPE_L,
        ['I'] = IN_ESCAPE_I,
    },

    /* top level rule */
    [IN_START] = {
        ['"'] = IN_DQ_STRING,
        ['\''] = IN_SQ_STRING,
        ['0'] = IN_ZERO,
        DIGITS_1_9(IN_NONZERO_NUMBER),
        ['-'] = IN_NEG_NONZERO_NUMBER,
        ['{'] = JSON_OPERATOR,
        ['}'] = JSON_OPERATOR,
        ['['] = JSON_OPERATOR,
        [']'] = JSON_OPERATOR,
        [','] = JSON_OPERATOR,
        [':'] = JSON_OPERATOR,
        LOWER_ALPHA(IN_KEYWORD),
        ['%'] = IN_ESCAPE,
        [' '] = IN_WHITESPACE,
        ['\t'] = IN_WHITESPACE,
        ['\r'] = IN_WHITESPACE,
        ['\n'] = IN_WHITESPACE,
    },
};

void json_lexer_init(JSONLexer *lexer, JSONLexerEmitter func)
{
    lexer->emit = func;
    lexer->state = IN_START;
    lexer->token = qstring_new();
    lexer->x = lexer->y = 0;
}

static int json_lexer_feed_char(JSONLexer *lexer, char ch, bool flush)
{
    int char_consumed, new_state;

    lexer->x++;
    if (ch == '\n') {
        lexer->x = 0;
        lexer->y++;
    }

    do {
        new_state = json_lexer[lexer->state][(uint8_t)ch];
        char_consumed = !TERMINAL_NEEDED_LOOKAHEAD(lexer->state, new_state);
        if (char_consumed) {
            qstring_append_chr(lexer->token, ch);
        }

        switch (new_state) {
        case JSON_OPERATOR:
        case JSON_ESCAPE:
        case JSON_INTEGER:
        case JSON_FLOAT:
        case JSON_KEYWORD:
        case JSON_STRING:
            lexer->emit(lexer, lexer->token, new_state, lexer->x, lexer->y);
            /* fall through */
        case JSON_SKIP:
            QDECREF(lexer->token);
            lexer->token = qstring_new();
            new_state = IN_START;
            break;
        case IN_ERROR:
            /* XXX: To avoid having previous bad input leaving the parser in an
             * unresponsive state where we consume unpredictable amounts of
             * subsequent "good" input, percolate this error state up to the
             * tokenizer/parser by forcing a NULL object to be emitted, then
             * reset state.
             *
             * Also note that this handling is required for reliable channel
             * negotiation between QMP and the guest agent, since chr(0xFF)
             * is placed at the beginning of certain events to ensure proper
             * delivery when the channel is in an unknown state. chr(0xFF) is
             * never a valid ASCII/UTF-8 sequence, so this should reliably
             * induce an error/flush state.
             */
            lexer->emit(lexer, lexer->token, JSON_ERROR, lexer->x, lexer->y);
            QDECREF(lexer->token);
            lexer->token = qstring_new();
            new_state = IN_START;
            lexer->state = new_state;
            return 0;
        default:
            break;
        }
        lexer->state = new_state;
    } while (!char_consumed && !flush);

    /* Do not let a single token grow to an arbitrarily large size,
     * this is a security consideration.
     */
    if (lexer->token->length > MAX_TOKEN_SIZE) {
        lexer->emit(lexer, lexer->token, lexer->state, lexer->x, lexer->y);
        QDECREF(lexer->token);
        lexer->token = qstring_new();
        lexer->state = IN_START;
    }

    return 0;
}

int json_lexer_feed(JSONLexer *lexer, const char *buffer, size_t size)
{
    size_t i;

    for (i = 0; i < size; i++) {
        int err;

        err = json_lexer_feed_char(lexer, buffer[i], false);
        if (err < 0) {
            return err;
        }
    }

    return 0;
}

int json_lexer_flush(JSONLexer *lexer)
{
    return lexer->state == IN_START ? 0 : json_lexer_feed_char(lexer, 0, true);
}

void json_lexer_destroy(JSONLexer *lexer)
{
    QDECREF(lexer->token);
}
