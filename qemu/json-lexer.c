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

#include "qstring.h"
#include "qlist.h"
#include "qdict.h"
#include "qint.h"
#include "qemu-common.h"
#include "json-lexer.h"

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
#define TERMINAL(state) [0 ... 0x7F] = (state)
#else
#define TERMINAL(state) \
    [0]   = (state), \
    [1]   = (state), \
    [2]   = (state), \
    [3]   = (state), \
    [4]   = (state), \
    [5]   = (state), \
    [6]   = (state), \
    [7]   = (state), \
    [8]   = (state), \
    [9]   = (state), \
    [10]  = (state), \
    [11]  = (state), \
    [12]  = (state), \
    [13]  = (state), \
    [14]  = (state), \
    [15]  = (state), \
    [16]  = (state), \
    [17]  = (state), \
    [18]  = (state), \
    [19]  = (state), \
    [20]  = (state), \
    [21]  = (state), \
    [22]  = (state), \
    [23]  = (state), \
    [24]  = (state), \
    [25]  = (state), \
    [26]  = (state), \
    [27]  = (state), \
    [28]  = (state), \
    [29]  = (state), \
    [30]  = (state), \
    [31]  = (state), \
    [32]  = (state), \
    [33]  = (state), \
    [34]  = (state), \
    [35]  = (state), \
    [36]  = (state), \
    [37]  = (state), \
    [38]  = (state), \
    [39]  = (state), \
    [40]  = (state), \
    [41]  = (state), \
    [42]  = (state), \
    [43]  = (state), \
    [44]  = (state), \
    [45]  = (state), \
    [46]  = (state), \
    [47]  = (state), \
    [48]  = (state), \
    [49]  = (state), \
    [50]  = (state), \
    [51]  = (state), \
    [52]  = (state), \
    [53]  = (state), \
    [54]  = (state), \
    [55]  = (state), \
    [56]  = (state), \
    [57]  = (state), \
    [58]  = (state), \
    [59]  = (state), \
    [60]  = (state), \
    [61]  = (state), \
    [62]  = (state), \
    [63]  = (state), \
    [64]  = (state), \
    [65]  = (state), \
    [66]  = (state), \
    [67]  = (state), \
    [68]  = (state), \
    [69]  = (state), \
    [70]  = (state), \
    [71]  = (state), \
    [72]  = (state), \
    [73]  = (state), \
    [74]  = (state), \
    [75]  = (state), \
    [76]  = (state), \
    [77]  = (state), \
    [78]  = (state), \
    [79]  = (state), \
    [80]  = (state), \
    [81]  = (state), \
    [82]  = (state), \
    [83]  = (state), \
    [84]  = (state), \
    [85]  = (state), \
    [86]  = (state), \
    [87]  = (state), \
    [88]  = (state), \
    [89]  = (state), \
    [90]  = (state), \
    [91]  = (state), \
    [92]  = (state), \
    [93]  = (state), \
    [94]  = (state), \
    [95]  = (state), \
    [96]  = (state), \
    [97]  = (state), \
    [98]  = (state), \
    [99]  = (state), \
    [100] = (state), \
    [101] = (state), \
    [102] = (state), \
    [103] = (state), \
    [104] = (state), \
    [105] = (state), \
    [106] = (state), \
    [107] = (state), \
    [108] = (state), \
    [109] = (state), \
    [110] = (state), \
    [111] = (state), \
    [112] = (state), \
    [113] = (state), \
    [114] = (state), \
    [115] = (state), \
    [116] = (state), \
    [117] = (state), \
    [118] = (state), \
    [119] = (state), \
    [120] = (state), \
    [121] = (state), \
    [122] = (state), \
    [123] = (state), \
    [124] = (state), \
    [125] = (state), \
    [126] = (state), \
    [127] = (state)
#endif

/* Return whether TERMINAL is a terminal state and the transition to it
   from OLD_STATE required lookahead.  This happens whenever the table
   below uses the TERMINAL macro.  */
#define TERMINAL_NEEDED_LOOKAHEAD(old_state, terminal) \
            (json_lexer[(old_state)][0] == (terminal))

static const uint8_t json_lexer[][256] =  {
#ifndef _MSC_VER
    /* double quote string */
    [IN_DQ_UCODE3] = {
        ['0' ... '9'] = IN_DQ_STRING,
        ['a' ... 'f'] = IN_DQ_STRING,
        ['A' ... 'F'] = IN_DQ_STRING,
    },
    [IN_DQ_UCODE2] = {
        ['0' ... '9'] = IN_DQ_UCODE3,
        ['a' ... 'f'] = IN_DQ_UCODE3,
        ['A' ... 'F'] = IN_DQ_UCODE3,
    },
    [IN_DQ_UCODE1] = {
        ['0' ... '9'] = IN_DQ_UCODE2,
        ['a' ... 'f'] = IN_DQ_UCODE2,
        ['A' ... 'F'] = IN_DQ_UCODE2,
    },
    [IN_DQ_UCODE0] = {
        ['0' ... '9'] = IN_DQ_UCODE1,
        ['a' ... 'f'] = IN_DQ_UCODE1,
        ['A' ... 'F'] = IN_DQ_UCODE1,
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
        [1 ... 0xBF] = IN_DQ_STRING,
        [0xC2 ... 0xF4] = IN_DQ_STRING,
        ['\\'] = IN_DQ_STRING_ESCAPE,
        ['"'] = JSON_STRING,
    },

    /* single quote string */
    [IN_SQ_UCODE3] = {
        ['0' ... '9'] = IN_SQ_STRING,
        ['a' ... 'f'] = IN_SQ_STRING,
        ['A' ... 'F'] = IN_SQ_STRING,
    },
    [IN_SQ_UCODE2] = {
        ['0' ... '9'] = IN_SQ_UCODE3,
        ['a' ... 'f'] = IN_SQ_UCODE3,
        ['A' ... 'F'] = IN_SQ_UCODE3,
    },
    [IN_SQ_UCODE1] = {
        ['0' ... '9'] = IN_SQ_UCODE2,
        ['a' ... 'f'] = IN_SQ_UCODE2,
        ['A' ... 'F'] = IN_SQ_UCODE2,
    },
    [IN_SQ_UCODE0] = {
        ['0' ... '9'] = IN_SQ_UCODE1,
        ['a' ... 'f'] = IN_SQ_UCODE1,
        ['A' ... 'F'] = IN_SQ_UCODE1,
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
        [1 ... 0xBF] = IN_SQ_STRING,
        [0xC2 ... 0xF4] = IN_SQ_STRING,
        ['\\'] = IN_SQ_STRING_ESCAPE,
        ['\''] = JSON_STRING,
    },

    /* Zero */
    [IN_ZERO] = {
        TERMINAL(JSON_INTEGER),
        ['0' ... '9'] = IN_ERROR,
        ['.'] = IN_MANTISSA,
    },

    /* Float */
    [IN_DIGITS] = {
        TERMINAL(JSON_FLOAT),
        ['0' ... '9'] = IN_DIGITS,
    },

    [IN_DIGIT] = {
        ['0' ... '9'] = IN_DIGITS,
    },

    [IN_EXP_E] = {
        ['-'] = IN_DIGIT,
        ['+'] = IN_DIGIT,
        ['0' ... '9'] = IN_DIGITS,
    },

    [IN_MANTISSA_DIGITS] = {
        TERMINAL(JSON_FLOAT),
        ['0' ... '9'] = IN_MANTISSA_DIGITS,
        ['e'] = IN_EXP_E,
        ['E'] = IN_EXP_E,
    },

    [IN_MANTISSA] = {
        ['0' ... '9'] = IN_MANTISSA_DIGITS,
    },

    /* Number */
    [IN_NONZERO_NUMBER] = {
        TERMINAL(JSON_INTEGER),
        ['0' ... '9'] = IN_NONZERO_NUMBER,
        ['e'] = IN_EXP_E,
        ['E'] = IN_EXP_E,
        ['.'] = IN_MANTISSA,
    },

    [IN_NEG_NONZERO_NUMBER] = {
        ['0'] = IN_ZERO,
        ['1' ... '9'] = IN_NONZERO_NUMBER,
    },

    /* keywords */
    [IN_KEYWORD] = {
        TERMINAL(JSON_KEYWORD),
        ['a' ... 'z'] = IN_KEYWORD,
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
        ['1' ... '9'] = IN_NONZERO_NUMBER,
        ['-'] = IN_NEG_NONZERO_NUMBER,
        ['{'] = JSON_OPERATOR,
        ['}'] = JSON_OPERATOR,
        ['['] = JSON_OPERATOR,
        [']'] = JSON_OPERATOR,
        [','] = JSON_OPERATOR,
        [':'] = JSON_OPERATOR,
        ['a' ... 'z'] = IN_KEYWORD,
        ['%'] = IN_ESCAPE,
        [' '] = IN_WHITESPACE,
        ['\t'] = IN_WHITESPACE,
        ['\r'] = IN_WHITESPACE,
        ['\n'] = IN_WHITESPACE,
    },
#else
/* double quote string */
[IN_DQ_UCODE3] = {
    ['0'] = IN_DQ_STRING,
    ['1'] = IN_DQ_STRING,
    ['2'] = IN_DQ_STRING,
    ['3'] = IN_DQ_STRING,
    ['4'] = IN_DQ_STRING,
    ['5'] = IN_DQ_STRING,
    ['6'] = IN_DQ_STRING,
    ['7'] = IN_DQ_STRING,
    ['8'] = IN_DQ_STRING,
    ['9'] = IN_DQ_STRING,
    ['a'] = IN_DQ_STRING,
    ['b'] = IN_DQ_STRING,
    ['c'] = IN_DQ_STRING,
    ['d'] = IN_DQ_STRING,
    ['e'] = IN_DQ_STRING,
    ['f'] = IN_DQ_STRING,
    ['A'] = IN_DQ_STRING,
    ['B'] = IN_DQ_STRING,
    ['C'] = IN_DQ_STRING,
    ['D'] = IN_DQ_STRING,
    ['E'] = IN_DQ_STRING,
    ['F'] = IN_DQ_STRING,
},
[IN_DQ_UCODE2] = {
    ['0'] = IN_DQ_UCODE3,
    ['1'] = IN_DQ_UCODE3,
    ['2'] = IN_DQ_UCODE3,
    ['3'] = IN_DQ_UCODE3,
    ['4'] = IN_DQ_UCODE3,
    ['5'] = IN_DQ_UCODE3,
    ['6'] = IN_DQ_UCODE3,
    ['7'] = IN_DQ_UCODE3,
    ['8'] = IN_DQ_UCODE3,
    ['9'] = IN_DQ_UCODE3,
    ['a'] = IN_DQ_UCODE3,
    ['b'] = IN_DQ_UCODE3,
    ['c'] = IN_DQ_UCODE3,
    ['d'] = IN_DQ_UCODE3,
    ['e'] = IN_DQ_UCODE3,
    ['f'] = IN_DQ_UCODE3,
    ['A'] = IN_DQ_UCODE3,
    ['B'] = IN_DQ_UCODE3,
    ['C'] = IN_DQ_UCODE3,
    ['D'] = IN_DQ_UCODE3,
    ['E'] = IN_DQ_UCODE3,
    ['F'] = IN_DQ_UCODE3,
},
[IN_DQ_UCODE1] = {
    ['0'] = IN_DQ_UCODE2,
    ['1'] = IN_DQ_UCODE2,
    ['2'] = IN_DQ_UCODE2,
    ['3'] = IN_DQ_UCODE2,
    ['4'] = IN_DQ_UCODE2,
    ['5'] = IN_DQ_UCODE2,
    ['6'] = IN_DQ_UCODE2,
    ['7'] = IN_DQ_UCODE2,
    ['8'] = IN_DQ_UCODE2,
    ['9'] = IN_DQ_UCODE2,
    ['a'] = IN_DQ_UCODE2,
    ['b'] = IN_DQ_UCODE2,
    ['c'] = IN_DQ_UCODE2,
    ['d'] = IN_DQ_UCODE2,
    ['e'] = IN_DQ_UCODE2,
    ['f'] = IN_DQ_UCODE2,
    ['A'] = IN_DQ_UCODE2,
    ['B'] = IN_DQ_UCODE2,
    ['C'] = IN_DQ_UCODE2,
    ['D'] = IN_DQ_UCODE2,
    ['E'] = IN_DQ_UCODE2,
    ['F'] = IN_DQ_UCODE2,
},
[IN_DQ_UCODE0] = {
    ['0'] = IN_DQ_UCODE1,
    ['1'] = IN_DQ_UCODE1,
    ['2'] = IN_DQ_UCODE1,
    ['3'] = IN_DQ_UCODE1,
    ['4'] = IN_DQ_UCODE1,
    ['5'] = IN_DQ_UCODE1,
    ['6'] = IN_DQ_UCODE1,
    ['7'] = IN_DQ_UCODE1,
    ['8'] = IN_DQ_UCODE1,
    ['9'] = IN_DQ_UCODE1,
    ['a'] = IN_DQ_UCODE1,
    ['b'] = IN_DQ_UCODE1,
    ['c'] = IN_DQ_UCODE1,
    ['d'] = IN_DQ_UCODE1,
    ['e'] = IN_DQ_UCODE1,
    ['f'] = IN_DQ_UCODE1,
    ['A'] = IN_DQ_UCODE1,
    ['B'] = IN_DQ_UCODE1,
    ['C'] = IN_DQ_UCODE1,
    ['D'] = IN_DQ_UCODE1,
    ['E'] = IN_DQ_UCODE1,
    ['F'] = IN_DQ_UCODE1,
},
[IN_DQ_STRING_ESCAPE] = {
    ['b'] = IN_DQ_STRING,
    ['f'] = IN_DQ_STRING,
    ['n'] = IN_DQ_STRING,
    ['r'] = IN_DQ_STRING,
    ['t'] = IN_DQ_STRING,
    ['/'] = IN_DQ_STRING,
    ['\\'] = IN_DQ_STRING,
    ['\''] = IN_DQ_STRING,
    ['\"'] = IN_DQ_STRING,
    ['u'] = IN_DQ_UCODE0,
},
[IN_DQ_STRING] = {
        [0x01] = IN_DQ_STRING,
    [0x02] = IN_DQ_STRING,
    [0x03] = IN_DQ_STRING,
    [0x04] = IN_DQ_STRING,
    [0x05] = IN_DQ_STRING,
    [0x06] = IN_DQ_STRING,
    [0x07] = IN_DQ_STRING,
    [0x08] = IN_DQ_STRING,
    [0x09] = IN_DQ_STRING,
    [0x0A] = IN_DQ_STRING,
    [0x0B] = IN_DQ_STRING,
    [0x0C] = IN_DQ_STRING,
    [0x0D] = IN_DQ_STRING,
    [0x0E] = IN_DQ_STRING,
    [0x0F] = IN_DQ_STRING,
    [0x10] = IN_DQ_STRING,
    [0x11] = IN_DQ_STRING,
    [0x12] = IN_DQ_STRING,
    [0x13] = IN_DQ_STRING,
    [0x14] = IN_DQ_STRING,
    [0x15] = IN_DQ_STRING,
    [0x16] = IN_DQ_STRING,
    [0x17] = IN_DQ_STRING,
    [0x18] = IN_DQ_STRING,
    [0x19] = IN_DQ_STRING,
    [0x1A] = IN_DQ_STRING,
    [0x1B] = IN_DQ_STRING,
    [0x1C] = IN_DQ_STRING,
    [0x1D] = IN_DQ_STRING,
    [0x1E] = IN_DQ_STRING,
    [0x1F] = IN_DQ_STRING,
    [0x20] = IN_DQ_STRING,
    [0x21] = IN_DQ_STRING,
    [0x22] = IN_DQ_STRING,
    [0x23] = IN_DQ_STRING,
    [0x24] = IN_DQ_STRING,
    [0x25] = IN_DQ_STRING,
    [0x26] = IN_DQ_STRING,
    [0x27] = IN_DQ_STRING,
    [0x28] = IN_DQ_STRING,
    [0x29] = IN_DQ_STRING,
    [0x2A] = IN_DQ_STRING,
    [0x2B] = IN_DQ_STRING,
    [0x2C] = IN_DQ_STRING,
    [0x2D] = IN_DQ_STRING,
    [0x2E] = IN_DQ_STRING,
    [0x2F] = IN_DQ_STRING,
    [0x30] = IN_DQ_STRING,
    [0x31] = IN_DQ_STRING,
    [0x32] = IN_DQ_STRING,
    [0x33] = IN_DQ_STRING,
    [0x34] = IN_DQ_STRING,
    [0x35] = IN_DQ_STRING,
    [0x36] = IN_DQ_STRING,
    [0x37] = IN_DQ_STRING,
    [0x38] = IN_DQ_STRING,
    [0x39] = IN_DQ_STRING,
    [0x3A] = IN_DQ_STRING,
    [0x3B] = IN_DQ_STRING,
    [0x3C] = IN_DQ_STRING,
    [0x3D] = IN_DQ_STRING,
    [0x3E] = IN_DQ_STRING,
    [0x3F] = IN_DQ_STRING,
    [0x40] = IN_DQ_STRING,
    [0x41] = IN_DQ_STRING,
    [0x42] = IN_DQ_STRING,
    [0x43] = IN_DQ_STRING,
    [0x44] = IN_DQ_STRING,
    [0x45] = IN_DQ_STRING,
    [0x46] = IN_DQ_STRING,
    [0x47] = IN_DQ_STRING,
    [0x48] = IN_DQ_STRING,
    [0x49] = IN_DQ_STRING,
    [0x4A] = IN_DQ_STRING,
    [0x4B] = IN_DQ_STRING,
    [0x4C] = IN_DQ_STRING,
    [0x4D] = IN_DQ_STRING,
    [0x4E] = IN_DQ_STRING,
    [0x4F] = IN_DQ_STRING,
    [0x50] = IN_DQ_STRING,
    [0x51] = IN_DQ_STRING,
    [0x52] = IN_DQ_STRING,
    [0x53] = IN_DQ_STRING,
    [0x54] = IN_DQ_STRING,
    [0x55] = IN_DQ_STRING,
    [0x56] = IN_DQ_STRING,
    [0x57] = IN_DQ_STRING,
    [0x58] = IN_DQ_STRING,
    [0x59] = IN_DQ_STRING,
    [0x5A] = IN_DQ_STRING,
    [0x5B] = IN_DQ_STRING,
    [0x5C] = IN_DQ_STRING,
    [0x5D] = IN_DQ_STRING,
    [0x5E] = IN_DQ_STRING,
    [0x5F] = IN_DQ_STRING,
    [0x60] = IN_DQ_STRING,
    [0x61] = IN_DQ_STRING,
    [0x62] = IN_DQ_STRING,
    [0x63] = IN_DQ_STRING,
    [0x64] = IN_DQ_STRING,
    [0x65] = IN_DQ_STRING,
    [0x66] = IN_DQ_STRING,
    [0x67] = IN_DQ_STRING,
    [0x68] = IN_DQ_STRING,
    [0x69] = IN_DQ_STRING,
    [0x6A] = IN_DQ_STRING,
    [0x6B] = IN_DQ_STRING,
    [0x6C] = IN_DQ_STRING,
    [0x6D] = IN_DQ_STRING,
    [0x6E] = IN_DQ_STRING,
    [0x6F] = IN_DQ_STRING,
    [0x70] = IN_DQ_STRING,
    [0x71] = IN_DQ_STRING,
    [0x72] = IN_DQ_STRING,
    [0x73] = IN_DQ_STRING,
    [0x74] = IN_DQ_STRING,
    [0x75] = IN_DQ_STRING,
    [0x76] = IN_DQ_STRING,
    [0x77] = IN_DQ_STRING,
    [0x78] = IN_DQ_STRING,
    [0x79] = IN_DQ_STRING,
    [0x7A] = IN_DQ_STRING,
    [0x7B] = IN_DQ_STRING,
    [0x7C] = IN_DQ_STRING,
    [0x7D] = IN_DQ_STRING,
    [0x7E] = IN_DQ_STRING,
    [0x7F] = IN_DQ_STRING,
    [0x80] = IN_DQ_STRING,
    [0x81] = IN_DQ_STRING,
    [0x82] = IN_DQ_STRING,
    [0x83] = IN_DQ_STRING,
    [0x84] = IN_DQ_STRING,
    [0x85] = IN_DQ_STRING,
    [0x86] = IN_DQ_STRING,
    [0x87] = IN_DQ_STRING,
    [0x88] = IN_DQ_STRING,
    [0x89] = IN_DQ_STRING,
    [0x8A] = IN_DQ_STRING,
    [0x8B] = IN_DQ_STRING,
    [0x8C] = IN_DQ_STRING,
    [0x8D] = IN_DQ_STRING,
    [0x8E] = IN_DQ_STRING,
    [0x8F] = IN_DQ_STRING,
    [0x90] = IN_DQ_STRING,
    [0x91] = IN_DQ_STRING,
    [0x92] = IN_DQ_STRING,
    [0x93] = IN_DQ_STRING,
    [0x94] = IN_DQ_STRING,
    [0x95] = IN_DQ_STRING,
    [0x96] = IN_DQ_STRING,
    [0x97] = IN_DQ_STRING,
    [0x98] = IN_DQ_STRING,
    [0x99] = IN_DQ_STRING,
    [0x9A] = IN_DQ_STRING,
    [0x9B] = IN_DQ_STRING,
    [0x9C] = IN_DQ_STRING,
    [0x9D] = IN_DQ_STRING,
    [0x9E] = IN_DQ_STRING,
    [0x9F] = IN_DQ_STRING,
    [0xA0] = IN_DQ_STRING,
    [0xA1] = IN_DQ_STRING,
    [0xA2] = IN_DQ_STRING,
    [0xA3] = IN_DQ_STRING,
    [0xA4] = IN_DQ_STRING,
    [0xA5] = IN_DQ_STRING,
    [0xA6] = IN_DQ_STRING,
    [0xA7] = IN_DQ_STRING,
    [0xA8] = IN_DQ_STRING,
    [0xA9] = IN_DQ_STRING,
    [0xAA] = IN_DQ_STRING,
    [0xAB] = IN_DQ_STRING,
    [0xAC] = IN_DQ_STRING,
    [0xAD] = IN_DQ_STRING,
    [0xAE] = IN_DQ_STRING,
    [0xAF] = IN_DQ_STRING,
    [0xB0] = IN_DQ_STRING,
    [0xB1] = IN_DQ_STRING,
    [0xB2] = IN_DQ_STRING,
    [0xB3] = IN_DQ_STRING,
    [0xB4] = IN_DQ_STRING,
    [0xB5] = IN_DQ_STRING,
    [0xB6] = IN_DQ_STRING,
    [0xB7] = IN_DQ_STRING,
    [0xB8] = IN_DQ_STRING,
    [0xB9] = IN_DQ_STRING,
    [0xBA] = IN_DQ_STRING,
    [0xBB] = IN_DQ_STRING,
    [0xBC] = IN_DQ_STRING,
    [0xBD] = IN_DQ_STRING,
    [0xBE] = IN_DQ_STRING,
    [0xBF] = IN_DQ_STRING,
    [0xC2] = IN_DQ_STRING,
    [0xC3] = IN_DQ_STRING,
    [0xC4] = IN_DQ_STRING,
    [0xC5] = IN_DQ_STRING,
    [0xC6] = IN_DQ_STRING,
    [0xC7] = IN_DQ_STRING,
    [0xC8] = IN_DQ_STRING,
    [0xC9] = IN_DQ_STRING,
    [0xCA] = IN_DQ_STRING,
    [0xCB] = IN_DQ_STRING,
    [0xCC] = IN_DQ_STRING,
    [0xCD] = IN_DQ_STRING,
    [0xCE] = IN_DQ_STRING,
    [0xCF] = IN_DQ_STRING,
    [0xD0] = IN_DQ_STRING,
    [0xD1] = IN_DQ_STRING,
    [0xD2] = IN_DQ_STRING,
    [0xD3] = IN_DQ_STRING,
    [0xD4] = IN_DQ_STRING,
    [0xD5] = IN_DQ_STRING,
    [0xD6] = IN_DQ_STRING,
    [0xD7] = IN_DQ_STRING,
    [0xD8] = IN_DQ_STRING,
    [0xD9] = IN_DQ_STRING,
    [0xDA] = IN_DQ_STRING,
    [0xDB] = IN_DQ_STRING,
    [0xDC] = IN_DQ_STRING,
    [0xDD] = IN_DQ_STRING,
    [0xDE] = IN_DQ_STRING,
    [0xDF] = IN_DQ_STRING,
    [0xE0] = IN_DQ_STRING,
    [0xE1] = IN_DQ_STRING,
    [0xE2] = IN_DQ_STRING,
    [0xE3] = IN_DQ_STRING,
    [0xE4] = IN_DQ_STRING,
    [0xE5] = IN_DQ_STRING,
    [0xE6] = IN_DQ_STRING,
    [0xE7] = IN_DQ_STRING,
    [0xE8] = IN_DQ_STRING,
    [0xE9] = IN_DQ_STRING,
    [0xEA] = IN_DQ_STRING,
    [0xEB] = IN_DQ_STRING,
    [0xEC] = IN_DQ_STRING,
    [0xED] = IN_DQ_STRING,
    [0xEE] = IN_DQ_STRING,
    [0xEF] = IN_DQ_STRING,
    [0xF0] = IN_DQ_STRING,
    [0xF1] = IN_DQ_STRING,
    [0xF2] = IN_DQ_STRING,
    [0xF3] = IN_DQ_STRING,
    [0xF4] = IN_DQ_STRING,
	['\\'] = IN_DQ_STRING_ESCAPE,
	['"'] = JSON_STRING,
},

/* single quote string */
[IN_SQ_UCODE3] = {
    ['0'] = IN_SQ_STRING,
    ['1'] = IN_SQ_STRING,
    ['2'] = IN_SQ_STRING,
    ['3'] = IN_SQ_STRING,
    ['4'] = IN_SQ_STRING,
    ['5'] = IN_SQ_STRING,
    ['6'] = IN_SQ_STRING,
    ['7'] = IN_SQ_STRING,
    ['8'] = IN_SQ_STRING,
    ['9'] = IN_SQ_STRING,
    ['a'] = IN_SQ_STRING,
    ['b'] = IN_SQ_STRING,
    ['c'] = IN_SQ_STRING,
    ['d'] = IN_SQ_STRING,
    ['e'] = IN_SQ_STRING,
    ['f'] = IN_SQ_STRING,
    ['A'] = IN_SQ_STRING,
    ['B'] = IN_SQ_STRING,
    ['C'] = IN_SQ_STRING,
    ['D'] = IN_SQ_STRING,
    ['E'] = IN_SQ_STRING,
    ['F'] = IN_SQ_STRING,
},
[IN_SQ_UCODE2] = {
    ['0'] = IN_SQ_UCODE3,
    ['1'] = IN_SQ_UCODE3,
    ['2'] = IN_SQ_UCODE3,
    ['3'] = IN_SQ_UCODE3,
    ['4'] = IN_SQ_UCODE3,
    ['5'] = IN_SQ_UCODE3,
    ['6'] = IN_SQ_UCODE3,
    ['7'] = IN_SQ_UCODE3,
    ['8'] = IN_SQ_UCODE3,
    ['9'] = IN_SQ_UCODE3,
    ['a'] = IN_SQ_UCODE3,
    ['b'] = IN_SQ_UCODE3,
    ['c'] = IN_SQ_UCODE3,
    ['d'] = IN_SQ_UCODE3,
    ['e'] = IN_SQ_UCODE3,
    ['f'] = IN_SQ_UCODE3,
    ['A'] = IN_SQ_UCODE3,
    ['B'] = IN_SQ_UCODE3,
    ['C'] = IN_SQ_UCODE3,
    ['D'] = IN_SQ_UCODE3,
    ['E'] = IN_SQ_UCODE3,
    ['F'] = IN_SQ_UCODE3,
},
[IN_SQ_UCODE1] = {
    ['0'] = IN_SQ_UCODE2,
    ['1'] = IN_SQ_UCODE2,
    ['2'] = IN_SQ_UCODE2,
    ['3'] = IN_SQ_UCODE2,
    ['4'] = IN_SQ_UCODE2,
    ['5'] = IN_SQ_UCODE2,
    ['6'] = IN_SQ_UCODE2,
    ['7'] = IN_SQ_UCODE2,
    ['8'] = IN_SQ_UCODE2,
    ['9'] = IN_SQ_UCODE2,
    ['a'] = IN_SQ_UCODE2,
    ['b'] = IN_SQ_UCODE2,
    ['c'] = IN_SQ_UCODE2,
    ['d'] = IN_SQ_UCODE2,
    ['e'] = IN_SQ_UCODE2,
    ['f'] = IN_SQ_UCODE2,
    ['A'] = IN_SQ_UCODE2,
    ['B'] = IN_SQ_UCODE2,
    ['C'] = IN_SQ_UCODE2,
    ['D'] = IN_SQ_UCODE2,
    ['E'] = IN_SQ_UCODE2,
    ['F'] = IN_SQ_UCODE2,
},
[IN_SQ_UCODE0] = {
    ['0'] = IN_SQ_UCODE1,
    ['1'] = IN_SQ_UCODE1,
    ['2'] = IN_SQ_UCODE1,
    ['3'] = IN_SQ_UCODE1,
    ['4'] = IN_SQ_UCODE1,
    ['5'] = IN_SQ_UCODE1,
    ['6'] = IN_SQ_UCODE1,
    ['7'] = IN_SQ_UCODE1,
    ['8'] = IN_SQ_UCODE1,
    ['9'] = IN_SQ_UCODE1,
    ['a'] = IN_SQ_UCODE1,
    ['b'] = IN_SQ_UCODE1,
    ['c'] = IN_SQ_UCODE1,
    ['d'] = IN_SQ_UCODE1,
    ['e'] = IN_SQ_UCODE1,
    ['f'] = IN_SQ_UCODE1,
    ['A'] = IN_SQ_UCODE1,
    ['B'] = IN_SQ_UCODE1,
    ['C'] = IN_SQ_UCODE1,
    ['D'] = IN_SQ_UCODE1,
    ['E'] = IN_SQ_UCODE1,
    ['F'] = IN_SQ_UCODE1,
},
[IN_SQ_STRING_ESCAPE] = {
    ['b'] = IN_SQ_STRING,
    ['f'] = IN_SQ_STRING,
    ['n'] = IN_SQ_STRING,
    ['r'] = IN_SQ_STRING,
    ['t'] = IN_SQ_STRING,
    ['/'] = IN_DQ_STRING,
    ['\\'] = IN_DQ_STRING,
    ['\''] = IN_SQ_STRING,
    ['\"'] = IN_SQ_STRING,
    ['u'] = IN_SQ_UCODE0,
},
[IN_SQ_STRING] = {
        [0x01] = IN_SQ_STRING,
    [0x02] = IN_SQ_STRING,
    [0x03] = IN_SQ_STRING,
    [0x04] = IN_SQ_STRING,
    [0x05] = IN_SQ_STRING,
    [0x06] = IN_SQ_STRING,
    [0x07] = IN_SQ_STRING,
    [0x08] = IN_SQ_STRING,
    [0x09] = IN_SQ_STRING,
    [0x0A] = IN_SQ_STRING,
    [0x0B] = IN_SQ_STRING,
    [0x0C] = IN_SQ_STRING,
    [0x0D] = IN_SQ_STRING,
    [0x0E] = IN_SQ_STRING,
    [0x0F] = IN_SQ_STRING,
    [0x10] = IN_SQ_STRING,
    [0x11] = IN_SQ_STRING,
    [0x12] = IN_SQ_STRING,
    [0x13] = IN_SQ_STRING,
    [0x14] = IN_SQ_STRING,
    [0x15] = IN_SQ_STRING,
    [0x16] = IN_SQ_STRING,
    [0x17] = IN_SQ_STRING,
    [0x18] = IN_SQ_STRING,
    [0x19] = IN_SQ_STRING,
    [0x1A] = IN_SQ_STRING,
    [0x1B] = IN_SQ_STRING,
    [0x1C] = IN_SQ_STRING,
    [0x1D] = IN_SQ_STRING,
    [0x1E] = IN_SQ_STRING,
    [0x1F] = IN_SQ_STRING,
    [0x20] = IN_SQ_STRING,
    [0x21] = IN_SQ_STRING,
    [0x22] = IN_SQ_STRING,
    [0x23] = IN_SQ_STRING,
    [0x24] = IN_SQ_STRING,
    [0x25] = IN_SQ_STRING,
    [0x26] = IN_SQ_STRING,
    [0x27] = IN_SQ_STRING,
    [0x28] = IN_SQ_STRING,
    [0x29] = IN_SQ_STRING,
    [0x2A] = IN_SQ_STRING,
    [0x2B] = IN_SQ_STRING,
    [0x2C] = IN_SQ_STRING,
    [0x2D] = IN_SQ_STRING,
    [0x2E] = IN_SQ_STRING,
    [0x2F] = IN_SQ_STRING,
    [0x30] = IN_SQ_STRING,
    [0x31] = IN_SQ_STRING,
    [0x32] = IN_SQ_STRING,
    [0x33] = IN_SQ_STRING,
    [0x34] = IN_SQ_STRING,
    [0x35] = IN_SQ_STRING,
    [0x36] = IN_SQ_STRING,
    [0x37] = IN_SQ_STRING,
    [0x38] = IN_SQ_STRING,
    [0x39] = IN_SQ_STRING,
    [0x3A] = IN_SQ_STRING,
    [0x3B] = IN_SQ_STRING,
    [0x3C] = IN_SQ_STRING,
    [0x3D] = IN_SQ_STRING,
    [0x3E] = IN_SQ_STRING,
    [0x3F] = IN_SQ_STRING,
    [0x40] = IN_SQ_STRING,
    [0x41] = IN_SQ_STRING,
    [0x42] = IN_SQ_STRING,
    [0x43] = IN_SQ_STRING,
    [0x44] = IN_SQ_STRING,
    [0x45] = IN_SQ_STRING,
    [0x46] = IN_SQ_STRING,
    [0x47] = IN_SQ_STRING,
    [0x48] = IN_SQ_STRING,
    [0x49] = IN_SQ_STRING,
    [0x4A] = IN_SQ_STRING,
    [0x4B] = IN_SQ_STRING,
    [0x4C] = IN_SQ_STRING,
    [0x4D] = IN_SQ_STRING,
    [0x4E] = IN_SQ_STRING,
    [0x4F] = IN_SQ_STRING,
    [0x50] = IN_SQ_STRING,
    [0x51] = IN_SQ_STRING,
    [0x52] = IN_SQ_STRING,
    [0x53] = IN_SQ_STRING,
    [0x54] = IN_SQ_STRING,
    [0x55] = IN_SQ_STRING,
    [0x56] = IN_SQ_STRING,
    [0x57] = IN_SQ_STRING,
    [0x58] = IN_SQ_STRING,
    [0x59] = IN_SQ_STRING,
    [0x5A] = IN_SQ_STRING,
    [0x5B] = IN_SQ_STRING,
    [0x5C] = IN_SQ_STRING,
    [0x5D] = IN_SQ_STRING,
    [0x5E] = IN_SQ_STRING,
    [0x5F] = IN_SQ_STRING,
    [0x60] = IN_SQ_STRING,
    [0x61] = IN_SQ_STRING,
    [0x62] = IN_SQ_STRING,
    [0x63] = IN_SQ_STRING,
    [0x64] = IN_SQ_STRING,
    [0x65] = IN_SQ_STRING,
    [0x66] = IN_SQ_STRING,
    [0x67] = IN_SQ_STRING,
    [0x68] = IN_SQ_STRING,
    [0x69] = IN_SQ_STRING,
    [0x6A] = IN_SQ_STRING,
    [0x6B] = IN_SQ_STRING,
    [0x6C] = IN_SQ_STRING,
    [0x6D] = IN_SQ_STRING,
    [0x6E] = IN_SQ_STRING,
    [0x6F] = IN_SQ_STRING,
    [0x70] = IN_SQ_STRING,
    [0x71] = IN_SQ_STRING,
    [0x72] = IN_SQ_STRING,
    [0x73] = IN_SQ_STRING,
    [0x74] = IN_SQ_STRING,
    [0x75] = IN_SQ_STRING,
    [0x76] = IN_SQ_STRING,
    [0x77] = IN_SQ_STRING,
    [0x78] = IN_SQ_STRING,
    [0x79] = IN_SQ_STRING,
    [0x7A] = IN_SQ_STRING,
    [0x7B] = IN_SQ_STRING,
    [0x7C] = IN_SQ_STRING,
    [0x7D] = IN_SQ_STRING,
    [0x7E] = IN_SQ_STRING,
    [0x7F] = IN_SQ_STRING,
    [0x80] = IN_SQ_STRING,
    [0x81] = IN_SQ_STRING,
    [0x82] = IN_SQ_STRING,
    [0x83] = IN_SQ_STRING,
    [0x84] = IN_SQ_STRING,
    [0x85] = IN_SQ_STRING,
    [0x86] = IN_SQ_STRING,
    [0x87] = IN_SQ_STRING,
    [0x88] = IN_SQ_STRING,
    [0x89] = IN_SQ_STRING,
    [0x8A] = IN_SQ_STRING,
    [0x8B] = IN_SQ_STRING,
    [0x8C] = IN_SQ_STRING,
    [0x8D] = IN_SQ_STRING,
    [0x8E] = IN_SQ_STRING,
    [0x8F] = IN_SQ_STRING,
    [0x90] = IN_SQ_STRING,
    [0x91] = IN_SQ_STRING,
    [0x92] = IN_SQ_STRING,
    [0x93] = IN_SQ_STRING,
    [0x94] = IN_SQ_STRING,
    [0x95] = IN_SQ_STRING,
    [0x96] = IN_SQ_STRING,
    [0x97] = IN_SQ_STRING,
    [0x98] = IN_SQ_STRING,
    [0x99] = IN_SQ_STRING,
    [0x9A] = IN_SQ_STRING,
    [0x9B] = IN_SQ_STRING,
    [0x9C] = IN_SQ_STRING,
    [0x9D] = IN_SQ_STRING,
    [0x9E] = IN_SQ_STRING,
    [0x9F] = IN_SQ_STRING,
    [0xA0] = IN_SQ_STRING,
    [0xA1] = IN_SQ_STRING,
    [0xA2] = IN_SQ_STRING,
    [0xA3] = IN_SQ_STRING,
    [0xA4] = IN_SQ_STRING,
    [0xA5] = IN_SQ_STRING,
    [0xA6] = IN_SQ_STRING,
    [0xA7] = IN_SQ_STRING,
    [0xA8] = IN_SQ_STRING,
    [0xA9] = IN_SQ_STRING,
    [0xAA] = IN_SQ_STRING,
    [0xAB] = IN_SQ_STRING,
    [0xAC] = IN_SQ_STRING,
    [0xAD] = IN_SQ_STRING,
    [0xAE] = IN_SQ_STRING,
    [0xAF] = IN_SQ_STRING,
    [0xB0] = IN_SQ_STRING,
    [0xB1] = IN_SQ_STRING,
    [0xB2] = IN_SQ_STRING,
    [0xB3] = IN_SQ_STRING,
    [0xB4] = IN_SQ_STRING,
    [0xB5] = IN_SQ_STRING,
    [0xB6] = IN_SQ_STRING,
    [0xB7] = IN_SQ_STRING,
    [0xB8] = IN_SQ_STRING,
    [0xB9] = IN_SQ_STRING,
    [0xBA] = IN_SQ_STRING,
    [0xBB] = IN_SQ_STRING,
    [0xBC] = IN_SQ_STRING,
    [0xBD] = IN_SQ_STRING,
    [0xBE] = IN_SQ_STRING,
    [0xBF] = IN_SQ_STRING,
    [0xC2] = IN_SQ_STRING,
    [0xC3] = IN_SQ_STRING,
    [0xC4] = IN_SQ_STRING,
    [0xC5] = IN_SQ_STRING,
    [0xC6] = IN_SQ_STRING,
    [0xC7] = IN_SQ_STRING,
    [0xC8] = IN_SQ_STRING,
    [0xC9] = IN_SQ_STRING,
    [0xCA] = IN_SQ_STRING,
    [0xCB] = IN_SQ_STRING,
    [0xCC] = IN_SQ_STRING,
    [0xCD] = IN_SQ_STRING,
    [0xCE] = IN_SQ_STRING,
    [0xCF] = IN_SQ_STRING,
    [0xD0] = IN_SQ_STRING,
    [0xD1] = IN_SQ_STRING,
    [0xD2] = IN_SQ_STRING,
    [0xD3] = IN_SQ_STRING,
    [0xD4] = IN_SQ_STRING,
    [0xD5] = IN_SQ_STRING,
    [0xD6] = IN_SQ_STRING,
    [0xD7] = IN_SQ_STRING,
    [0xD8] = IN_SQ_STRING,
    [0xD9] = IN_SQ_STRING,
    [0xDA] = IN_SQ_STRING,
    [0xDB] = IN_SQ_STRING,
    [0xDC] = IN_SQ_STRING,
    [0xDD] = IN_SQ_STRING,
    [0xDE] = IN_SQ_STRING,
    [0xDF] = IN_SQ_STRING,
    [0xE0] = IN_SQ_STRING,
    [0xE1] = IN_SQ_STRING,
    [0xE2] = IN_SQ_STRING,
    [0xE3] = IN_SQ_STRING,
    [0xE4] = IN_SQ_STRING,
    [0xE5] = IN_SQ_STRING,
    [0xE6] = IN_SQ_STRING,
    [0xE7] = IN_SQ_STRING,
    [0xE8] = IN_SQ_STRING,
    [0xE9] = IN_SQ_STRING,
    [0xEA] = IN_SQ_STRING,
    [0xEB] = IN_SQ_STRING,
    [0xEC] = IN_SQ_STRING,
    [0xED] = IN_SQ_STRING,
    [0xEE] = IN_SQ_STRING,
    [0xEF] = IN_SQ_STRING,
    [0xF0] = IN_SQ_STRING,
    [0xF1] = IN_SQ_STRING,
    [0xF2] = IN_SQ_STRING,
    [0xF3] = IN_SQ_STRING,
    [0xF4] = IN_SQ_STRING,
    ['\\'] = IN_SQ_STRING_ESCAPE,
    ['\''] = JSON_STRING,
},

/* Zero */
[IN_ZERO] = {
    TERMINAL(JSON_INTEGER),
    ['0'] = IN_ERROR,
    ['1'] = IN_ERROR,
    ['2'] = IN_ERROR,
    ['3'] = IN_ERROR,
    ['4'] = IN_ERROR,
    ['5'] = IN_ERROR,
    ['6'] = IN_ERROR,
    ['7'] = IN_ERROR,
    ['8'] = IN_ERROR,
    ['9'] = IN_ERROR,
    ['.'] = IN_MANTISSA,
},

/* Float */
[IN_DIGITS] = {
    TERMINAL(JSON_FLOAT),
    ['0'] = IN_DIGITS,
    ['1'] = IN_DIGITS,
    ['2'] = IN_DIGITS,
    ['3'] = IN_DIGITS,
    ['4'] = IN_DIGITS,
    ['5'] = IN_DIGITS,
    ['6'] = IN_DIGITS,
    ['7'] = IN_DIGITS,
    ['8'] = IN_DIGITS,
    ['9'] = IN_DIGITS,
},

[IN_DIGIT] = {
    ['0'] = IN_DIGITS,
    ['1'] = IN_DIGITS,
    ['2'] = IN_DIGITS,
    ['3'] = IN_DIGITS,
    ['4'] = IN_DIGITS,
    ['5'] = IN_DIGITS,
    ['6'] = IN_DIGITS,
    ['7'] = IN_DIGITS,
    ['8'] = IN_DIGITS,
    ['9'] = IN_DIGITS,
},

[IN_EXP_E] = {
    ['-'] = IN_DIGIT,
    ['+'] = IN_DIGIT,
    ['0'] = IN_DIGITS,
    ['1'] = IN_DIGITS,
    ['2'] = IN_DIGITS,
    ['3'] = IN_DIGITS,
    ['4'] = IN_DIGITS,
    ['5'] = IN_DIGITS,
    ['6'] = IN_DIGITS,
    ['7'] = IN_DIGITS,
    ['8'] = IN_DIGITS,
    ['9'] = IN_DIGITS,
},

[IN_MANTISSA_DIGITS] = {
    TERMINAL(JSON_FLOAT),
    ['0'] = IN_MANTISSA_DIGITS,
    ['1'] = IN_MANTISSA_DIGITS,
    ['2'] = IN_MANTISSA_DIGITS,
    ['3'] = IN_MANTISSA_DIGITS,
    ['4'] = IN_MANTISSA_DIGITS,
    ['5'] = IN_MANTISSA_DIGITS,
    ['6'] = IN_MANTISSA_DIGITS,
    ['7'] = IN_MANTISSA_DIGITS,
    ['8'] = IN_MANTISSA_DIGITS,
    ['9'] = IN_MANTISSA_DIGITS,
    ['e'] = IN_EXP_E,
    ['E'] = IN_EXP_E,
},

[IN_MANTISSA] = {
    ['0'] = IN_MANTISSA_DIGITS,
    ['1'] = IN_MANTISSA_DIGITS,
    ['2'] = IN_MANTISSA_DIGITS,
    ['3'] = IN_MANTISSA_DIGITS,
    ['4'] = IN_MANTISSA_DIGITS,
    ['5'] = IN_MANTISSA_DIGITS,
    ['6'] = IN_MANTISSA_DIGITS,
    ['7'] = IN_MANTISSA_DIGITS,
    ['8'] = IN_MANTISSA_DIGITS,
    ['9'] = IN_MANTISSA_DIGITS,
},

/* Number */
[IN_NONZERO_NUMBER] = {
    TERMINAL(JSON_INTEGER),
    ['0'] = IN_NONZERO_NUMBER,
    ['1'] = IN_NONZERO_NUMBER,
    ['2'] = IN_NONZERO_NUMBER,
    ['3'] = IN_NONZERO_NUMBER,
    ['4'] = IN_NONZERO_NUMBER,
    ['5'] = IN_NONZERO_NUMBER,
    ['6'] = IN_NONZERO_NUMBER,
    ['7'] = IN_NONZERO_NUMBER,
    ['8'] = IN_NONZERO_NUMBER,
    ['9'] = IN_NONZERO_NUMBER,
    ['e'] = IN_EXP_E,
    ['E'] = IN_EXP_E,
    ['.'] = IN_MANTISSA,
},

[IN_NEG_NONZERO_NUMBER] = {
    ['0'] = IN_ZERO,
    ['1'] = IN_NONZERO_NUMBER,
    ['2'] = IN_NONZERO_NUMBER,
    ['3'] = IN_NONZERO_NUMBER,
    ['4'] = IN_NONZERO_NUMBER,
    ['5'] = IN_NONZERO_NUMBER,
    ['6'] = IN_NONZERO_NUMBER,
    ['7'] = IN_NONZERO_NUMBER,
    ['8'] = IN_NONZERO_NUMBER,
    ['9'] = IN_NONZERO_NUMBER,
},

/* keywords */
[IN_KEYWORD] = {
    TERMINAL(JSON_KEYWORD),
    ['a'] = IN_KEYWORD,
    ['b'] = IN_KEYWORD,
    ['c'] = IN_KEYWORD,
    ['d'] = IN_KEYWORD,
    ['e'] = IN_KEYWORD,
    ['f'] = IN_KEYWORD,
    ['g'] = IN_KEYWORD,
    ['h'] = IN_KEYWORD,
    ['i'] = IN_KEYWORD,
    ['j'] = IN_KEYWORD,
    ['k'] = IN_KEYWORD,
    ['l'] = IN_KEYWORD,
    ['m'] = IN_KEYWORD,
    ['n'] = IN_KEYWORD,
    ['o'] = IN_KEYWORD,
    ['p'] = IN_KEYWORD,
    ['q'] = IN_KEYWORD,
    ['r'] = IN_KEYWORD,
    ['s'] = IN_KEYWORD,
    ['t'] = IN_KEYWORD,
    ['u'] = IN_KEYWORD,
    ['v'] = IN_KEYWORD,
    ['w'] = IN_KEYWORD,
    ['x'] = IN_KEYWORD,
    ['y'] = IN_KEYWORD,
    ['z'] = IN_KEYWORD,
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
    ['1'] = IN_NONZERO_NUMBER,
    ['2'] = IN_NONZERO_NUMBER,
    ['3'] = IN_NONZERO_NUMBER,
    ['4'] = IN_NONZERO_NUMBER,
    ['5'] = IN_NONZERO_NUMBER,
    ['6'] = IN_NONZERO_NUMBER,
    ['7'] = IN_NONZERO_NUMBER,
    ['8'] = IN_NONZERO_NUMBER,
    ['9'] = IN_NONZERO_NUMBER,
    ['-'] = IN_NEG_NONZERO_NUMBER,
    ['{'] = JSON_OPERATOR,
    ['}'] = JSON_OPERATOR,
    ['['] = JSON_OPERATOR,
    [']'] = JSON_OPERATOR,
    [','] = JSON_OPERATOR,
    [':'] = JSON_OPERATOR,
    ['a'] = IN_KEYWORD,
    ['b'] = IN_KEYWORD,
    ['c'] = IN_KEYWORD,
    ['d'] = IN_KEYWORD,
    ['e'] = IN_KEYWORD,
    ['f'] = IN_KEYWORD,
    ['g'] = IN_KEYWORD,
    ['h'] = IN_KEYWORD,
    ['i'] = IN_KEYWORD,
    ['j'] = IN_KEYWORD,
    ['k'] = IN_KEYWORD,
    ['l'] = IN_KEYWORD,
    ['m'] = IN_KEYWORD,
    ['n'] = IN_KEYWORD,
    ['o'] = IN_KEYWORD,
    ['p'] = IN_KEYWORD,
    ['q'] = IN_KEYWORD,
    ['r'] = IN_KEYWORD,
    ['s'] = IN_KEYWORD,
    ['t'] = IN_KEYWORD,
    ['u'] = IN_KEYWORD,
    ['v'] = IN_KEYWORD,
    ['w'] = IN_KEYWORD,
    ['x'] = IN_KEYWORD,
    ['y'] = IN_KEYWORD,
    ['z'] = IN_KEYWORD,
    ['%'] = IN_ESCAPE,
    [' '] = IN_WHITESPACE,
    ['\t'] = IN_WHITESPACE,
    ['\r'] = IN_WHITESPACE,
    ['\n'] = IN_WHITESPACE,
},
#endif
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
