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

/*
 * \"([^\\\"]|(\\\"\\'\\\\\\/\\b\\f\\n\\r\\t\\u[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]))*\"
 * '([^\\']|(\\\"\\'\\\\\\/\\b\\f\\n\\r\\t\\u[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]))*'
 * 0|([1-9][0-9]*(.[0-9]+)?([eE]([-+])?[0-9]+))
 * [{}\[\],:]
 * [a-z]+
 *
 */

#undef ERROR // MSVC/Windows SDK defines this in wingcli.h, we just fix that here to make it happy....

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
        [1 ... 0xFF] = IN_DQ_STRING,
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
        [1 ... 0xFF] = IN_SQ_STRING,
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
	[1] = IN_DQ_STRING,
	[2] = IN_DQ_STRING,
	[3] = IN_DQ_STRING,
	[4] = IN_DQ_STRING,
	[5] = IN_DQ_STRING,
	[6] = IN_DQ_STRING,
	[7] = IN_DQ_STRING,
	[8] = IN_DQ_STRING,
	[9] = IN_DQ_STRING,
	[10] = IN_DQ_STRING,
	[11] = IN_DQ_STRING,
	[12] = IN_DQ_STRING,
	[13] = IN_DQ_STRING,
	[14] = IN_DQ_STRING,
	[15] = IN_DQ_STRING,
	[16] = IN_DQ_STRING,
	[17] = IN_DQ_STRING,
	[18] = IN_DQ_STRING,
	[19] = IN_DQ_STRING,
	[20] = IN_DQ_STRING,
	[21] = IN_DQ_STRING,
	[22] = IN_DQ_STRING,
	[23] = IN_DQ_STRING,
	[24] = IN_DQ_STRING,
	[25] = IN_DQ_STRING,
	[26] = IN_DQ_STRING,
	[27] = IN_DQ_STRING,
	[28] = IN_DQ_STRING,
	[29] = IN_DQ_STRING,
	[30] = IN_DQ_STRING,
	[31] = IN_DQ_STRING,
	[32] = IN_DQ_STRING,
	[33] = IN_DQ_STRING,
	[34] = IN_DQ_STRING,  
	[35] = IN_DQ_STRING,
	[36] = IN_DQ_STRING,
	[37] = IN_DQ_STRING,
	[38] = IN_DQ_STRING,
	[39] = IN_DQ_STRING,
	[40] = IN_DQ_STRING,
	[41] = IN_DQ_STRING,
	[42] = IN_DQ_STRING,
	[43] = IN_DQ_STRING,
	[44] = IN_DQ_STRING,
	[45] = IN_DQ_STRING,
	[46] = IN_DQ_STRING,
	[47] = IN_DQ_STRING,
	[48] = IN_DQ_STRING,
	[49] = IN_DQ_STRING,
	[50] = IN_DQ_STRING,
	[51] = IN_DQ_STRING,
	[52] = IN_DQ_STRING,
	[53] = IN_DQ_STRING,
	[54] = IN_DQ_STRING,
	[55] = IN_DQ_STRING,
	[56] = IN_DQ_STRING,
	[57] = IN_DQ_STRING,
	[58] = IN_DQ_STRING,
	[59] = IN_DQ_STRING,
	[60] = IN_DQ_STRING,
	[61] = IN_DQ_STRING,
	[62] = IN_DQ_STRING,
	[63] = IN_DQ_STRING,
	[64] = IN_DQ_STRING,
	[65] = IN_DQ_STRING,
	[66] = IN_DQ_STRING,
	[67] = IN_DQ_STRING,
	[68] = IN_DQ_STRING,
	[69] = IN_DQ_STRING,
	[70] = IN_DQ_STRING,
	[71] = IN_DQ_STRING,
	[72] = IN_DQ_STRING,
	[73] = IN_DQ_STRING,
	[74] = IN_DQ_STRING,
	[75] = IN_DQ_STRING,
	[76] = IN_DQ_STRING,
	[77] = IN_DQ_STRING,
	[78] = IN_DQ_STRING,
	[79] = IN_DQ_STRING,
	[80] = IN_DQ_STRING,
	[81] = IN_DQ_STRING,
	[82] = IN_DQ_STRING,
	[83] = IN_DQ_STRING,
	[84] = IN_DQ_STRING,
	[85] = IN_DQ_STRING,
	[86] = IN_DQ_STRING,
	[87] = IN_DQ_STRING,
	[88] = IN_DQ_STRING,
	[89] = IN_DQ_STRING,
	[90] = IN_DQ_STRING,
	[91] = IN_DQ_STRING,
	[92] = IN_DQ_STRING,
	[93] = IN_DQ_STRING,
	[94] = IN_DQ_STRING,
	[95] = IN_DQ_STRING,
	[96] = IN_DQ_STRING,
	[97] = IN_DQ_STRING,
	[98] = IN_DQ_STRING,
	[99] = IN_DQ_STRING,
	[100] = IN_DQ_STRING,
	[101] = IN_DQ_STRING,
	[102] = IN_DQ_STRING,
	[103] = IN_DQ_STRING,
	[104] = IN_DQ_STRING,
	[105] = IN_DQ_STRING,
	[106] = IN_DQ_STRING,
	[107] = IN_DQ_STRING,
	[108] = IN_DQ_STRING,
	[109] = IN_DQ_STRING,
	[110] = IN_DQ_STRING,
	[111] = IN_DQ_STRING,
	[112] = IN_DQ_STRING,
	[113] = IN_DQ_STRING,
	[114] = IN_DQ_STRING,
	[115] = IN_DQ_STRING,
	[116] = IN_DQ_STRING,
	[117] = IN_DQ_STRING,
	[118] = IN_DQ_STRING,
	[119] = IN_DQ_STRING,
	[120] = IN_DQ_STRING,
	[121] = IN_DQ_STRING,
	[122] = IN_DQ_STRING,
	[123] = IN_DQ_STRING,
	[124] = IN_DQ_STRING,
	[125] = IN_DQ_STRING,
	[126] = IN_DQ_STRING,
	[127] = IN_DQ_STRING,
	[128] = IN_DQ_STRING,
	[129] = IN_DQ_STRING,
	[130] = IN_DQ_STRING,
	[131] = IN_DQ_STRING,
	[132] = IN_DQ_STRING,
	[133] = IN_DQ_STRING,
	[134] = IN_DQ_STRING,
	[135] = IN_DQ_STRING,
	[136] = IN_DQ_STRING,
	[137] = IN_DQ_STRING,
	[138] = IN_DQ_STRING,
	[139] = IN_DQ_STRING,
	[140] = IN_DQ_STRING,
	[141] = IN_DQ_STRING,
	[142] = IN_DQ_STRING,
	[143] = IN_DQ_STRING,
	[144] = IN_DQ_STRING,
	[145] = IN_DQ_STRING,
	[146] = IN_DQ_STRING,
	[147] = IN_DQ_STRING,
	[148] = IN_DQ_STRING,
	[149] = IN_DQ_STRING,
	[150] = IN_DQ_STRING,
	[151] = IN_DQ_STRING,
	[152] = IN_DQ_STRING,
	[153] = IN_DQ_STRING,
	[154] = IN_DQ_STRING,
	[155] = IN_DQ_STRING,
	[156] = IN_DQ_STRING,
	[157] = IN_DQ_STRING,
	[158] = IN_DQ_STRING,
	[159] = IN_DQ_STRING,
	[160] = IN_DQ_STRING,
	[161] = IN_DQ_STRING,
	[162] = IN_DQ_STRING,
	[163] = IN_DQ_STRING,
	[164] = IN_DQ_STRING,
	[165] = IN_DQ_STRING,
	[166] = IN_DQ_STRING,
	[167] = IN_DQ_STRING,
	[168] = IN_DQ_STRING,
	[169] = IN_DQ_STRING,
	[170] = IN_DQ_STRING,
	[171] = IN_DQ_STRING,
	[172] = IN_DQ_STRING,
	[173] = IN_DQ_STRING,
	[174] = IN_DQ_STRING,
	[175] = IN_DQ_STRING,
	[176] = IN_DQ_STRING,
	[177] = IN_DQ_STRING,
	[178] = IN_DQ_STRING,
	[179] = IN_DQ_STRING,
	[180] = IN_DQ_STRING,
	[181] = IN_DQ_STRING,
	[182] = IN_DQ_STRING,
	[183] = IN_DQ_STRING,
	[184] = IN_DQ_STRING,
	[185] = IN_DQ_STRING,
	[186] = IN_DQ_STRING,
	[187] = IN_DQ_STRING,
	[188] = IN_DQ_STRING,
	[189] = IN_DQ_STRING,
	[190] = IN_DQ_STRING,
	[191] = IN_DQ_STRING,
	[192] = IN_DQ_STRING,
	[193] = IN_DQ_STRING,
	[194] = IN_DQ_STRING,
	[195] = IN_DQ_STRING,
	[196] = IN_DQ_STRING,
	[197] = IN_DQ_STRING,
	[198] = IN_DQ_STRING,
	[199] = IN_DQ_STRING,
	[200] = IN_DQ_STRING,
	[201] = IN_DQ_STRING,
	[202] = IN_DQ_STRING,
	[203] = IN_DQ_STRING,
	[204] = IN_DQ_STRING,
	[205] = IN_DQ_STRING,
	[206] = IN_DQ_STRING,
	[207] = IN_DQ_STRING,
	[208] = IN_DQ_STRING,
	[209] = IN_DQ_STRING,
	[210] = IN_DQ_STRING,
	[211] = IN_DQ_STRING,
	[212] = IN_DQ_STRING,
	[213] = IN_DQ_STRING,
	[214] = IN_DQ_STRING,
	[215] = IN_DQ_STRING,
	[216] = IN_DQ_STRING,
	[217] = IN_DQ_STRING,
	[218] = IN_DQ_STRING,
	[219] = IN_DQ_STRING,
	[220] = IN_DQ_STRING,
	[221] = IN_DQ_STRING,
	[222] = IN_DQ_STRING,
	[223] = IN_DQ_STRING,
	[224] = IN_DQ_STRING,
	[225] = IN_DQ_STRING,
	[226] = IN_DQ_STRING,
	[227] = IN_DQ_STRING,
	[228] = IN_DQ_STRING,
	[229] = IN_DQ_STRING,
	[230] = IN_DQ_STRING,
	[231] = IN_DQ_STRING,
	[232] = IN_DQ_STRING,
	[233] = IN_DQ_STRING,
	[234] = IN_DQ_STRING,
	[235] = IN_DQ_STRING,
	[236] = IN_DQ_STRING,
	[237] = IN_DQ_STRING,
	[238] = IN_DQ_STRING,
	[239] = IN_DQ_STRING,
	[240] = IN_DQ_STRING,
	[241] = IN_DQ_STRING,
	[242] = IN_DQ_STRING,
	[243] = IN_DQ_STRING,
	[244] = IN_DQ_STRING,
	[245] = IN_DQ_STRING,
	[246] = IN_DQ_STRING,
	[247] = IN_DQ_STRING,
	[248] = IN_DQ_STRING,
	[249] = IN_DQ_STRING,
	[250] = IN_DQ_STRING,
	[251] = IN_DQ_STRING,
	[252] = IN_DQ_STRING,
	[253] = IN_DQ_STRING,
	[254] = IN_DQ_STRING,
	[255] = IN_DQ_STRING,
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
    [1] = IN_SQ_STRING,
    [2] = IN_SQ_STRING,
    [3] = IN_SQ_STRING,
    [4] = IN_SQ_STRING,
    [5] = IN_SQ_STRING,
    [6] = IN_SQ_STRING,
    [7] = IN_SQ_STRING,
    [8] = IN_SQ_STRING,
    [9] = IN_SQ_STRING,
    [10] = IN_SQ_STRING,
    [11] = IN_SQ_STRING,
    [12] = IN_SQ_STRING,
    [13] = IN_SQ_STRING,
    [14] = IN_SQ_STRING,
    [15] = IN_SQ_STRING,
    [16] = IN_SQ_STRING,
    [17] = IN_SQ_STRING,
    [18] = IN_SQ_STRING,
    [19] = IN_SQ_STRING,
    [20] = IN_SQ_STRING,
    [21] = IN_SQ_STRING,
    [22] = IN_SQ_STRING,
    [23] = IN_SQ_STRING,
    [24] = IN_SQ_STRING,
    [25] = IN_SQ_STRING,
    [26] = IN_SQ_STRING,
    [27] = IN_SQ_STRING,
    [28] = IN_SQ_STRING,
    [29] = IN_SQ_STRING,
    [30] = IN_SQ_STRING,
    [31] = IN_SQ_STRING,
    [32] = IN_SQ_STRING,
    [33] = IN_SQ_STRING,
    [34] = IN_SQ_STRING,
    [35] = IN_SQ_STRING,
    [36] = IN_SQ_STRING,
    [37] = IN_SQ_STRING,
    [38] = IN_SQ_STRING,
    [39] = IN_SQ_STRING,
    [40] = IN_SQ_STRING,
    [41] = IN_SQ_STRING,
    [42] = IN_SQ_STRING,
    [43] = IN_SQ_STRING,
    [44] = IN_SQ_STRING,
    [45] = IN_SQ_STRING,
    [46] = IN_SQ_STRING,
    [47] = IN_SQ_STRING,
    [48] = IN_SQ_STRING,
    [49] = IN_SQ_STRING,
    [50] = IN_SQ_STRING,
    [51] = IN_SQ_STRING,
    [52] = IN_SQ_STRING,
    [53] = IN_SQ_STRING,
    [54] = IN_SQ_STRING,
    [55] = IN_SQ_STRING,
    [56] = IN_SQ_STRING,
    [57] = IN_SQ_STRING,
    [58] = IN_SQ_STRING,
    [59] = IN_SQ_STRING,
    [60] = IN_SQ_STRING,
    [61] = IN_SQ_STRING,
    [62] = IN_SQ_STRING,
    [63] = IN_SQ_STRING,
    [64] = IN_SQ_STRING,
    [65] = IN_SQ_STRING,
    [66] = IN_SQ_STRING,
    [67] = IN_SQ_STRING,
    [68] = IN_SQ_STRING,
    [69] = IN_SQ_STRING,
    [70] = IN_SQ_STRING,
    [71] = IN_SQ_STRING,
    [72] = IN_SQ_STRING,
    [73] = IN_SQ_STRING,
    [74] = IN_SQ_STRING,
    [75] = IN_SQ_STRING,
    [76] = IN_SQ_STRING,
    [77] = IN_SQ_STRING,
    [78] = IN_SQ_STRING,
    [79] = IN_SQ_STRING,
    [80] = IN_SQ_STRING,
    [81] = IN_SQ_STRING,
    [82] = IN_SQ_STRING,
    [83] = IN_SQ_STRING,
    [84] = IN_SQ_STRING,
    [85] = IN_SQ_STRING,
    [86] = IN_SQ_STRING,
    [87] = IN_SQ_STRING,
    [88] = IN_SQ_STRING,
    [89] = IN_SQ_STRING,
    [90] = IN_SQ_STRING,
    [91] = IN_SQ_STRING,
    [92] = IN_SQ_STRING,
    [93] = IN_SQ_STRING,
    [94] = IN_SQ_STRING,
    [95] = IN_SQ_STRING,
    [96] = IN_SQ_STRING,
    [97] = IN_SQ_STRING,
    [98] = IN_SQ_STRING,
    [99] = IN_SQ_STRING,
    [100] = IN_SQ_STRING,
    [101] = IN_SQ_STRING,
    [102] = IN_SQ_STRING,
    [103] = IN_SQ_STRING,
    [104] = IN_SQ_STRING,
    [105] = IN_SQ_STRING,
    [106] = IN_SQ_STRING,
    [107] = IN_SQ_STRING,
    [108] = IN_SQ_STRING,
    [109] = IN_SQ_STRING,
    [110] = IN_SQ_STRING,
    [111] = IN_SQ_STRING,
    [112] = IN_SQ_STRING,
    [113] = IN_SQ_STRING,
    [114] = IN_SQ_STRING,
    [115] = IN_SQ_STRING,
    [116] = IN_SQ_STRING,
    [117] = IN_SQ_STRING,
    [118] = IN_SQ_STRING,
    [119] = IN_SQ_STRING,
    [120] = IN_SQ_STRING,
    [121] = IN_SQ_STRING,
    [122] = IN_SQ_STRING,
    [123] = IN_SQ_STRING,
    [124] = IN_SQ_STRING,
    [125] = IN_SQ_STRING,
    [126] = IN_SQ_STRING,
    [127] = IN_SQ_STRING,
    [128] = IN_SQ_STRING,
    [129] = IN_SQ_STRING,
    [130] = IN_SQ_STRING,
    [131] = IN_SQ_STRING,
    [132] = IN_SQ_STRING,
    [133] = IN_SQ_STRING,
    [134] = IN_SQ_STRING,
    [135] = IN_SQ_STRING,
    [136] = IN_SQ_STRING,
    [137] = IN_SQ_STRING,
    [138] = IN_SQ_STRING,
    [139] = IN_SQ_STRING,
    [140] = IN_SQ_STRING,
    [141] = IN_SQ_STRING,
    [142] = IN_SQ_STRING,
    [143] = IN_SQ_STRING,
    [144] = IN_SQ_STRING,
    [145] = IN_SQ_STRING,
    [146] = IN_SQ_STRING,
    [147] = IN_SQ_STRING,
    [148] = IN_SQ_STRING,
    [149] = IN_SQ_STRING,
    [150] = IN_SQ_STRING,
    [151] = IN_SQ_STRING,
    [152] = IN_SQ_STRING,
    [153] = IN_SQ_STRING,
    [154] = IN_SQ_STRING,
    [155] = IN_SQ_STRING,
    [156] = IN_SQ_STRING,
    [157] = IN_SQ_STRING,
    [158] = IN_SQ_STRING,
    [159] = IN_SQ_STRING,
    [160] = IN_SQ_STRING,
    [161] = IN_SQ_STRING,
    [162] = IN_SQ_STRING,
    [163] = IN_SQ_STRING,
    [164] = IN_SQ_STRING,
    [165] = IN_SQ_STRING,
    [166] = IN_SQ_STRING,
    [167] = IN_SQ_STRING,
    [168] = IN_SQ_STRING,
    [169] = IN_SQ_STRING,
    [170] = IN_SQ_STRING,
    [171] = IN_SQ_STRING,
    [172] = IN_SQ_STRING,
    [173] = IN_SQ_STRING,
    [174] = IN_SQ_STRING,
    [175] = IN_SQ_STRING,
    [176] = IN_SQ_STRING,
    [177] = IN_SQ_STRING,
    [178] = IN_SQ_STRING,
    [179] = IN_SQ_STRING,
    [180] = IN_SQ_STRING,
    [181] = IN_SQ_STRING,
    [182] = IN_SQ_STRING,
    [183] = IN_SQ_STRING,
    [184] = IN_SQ_STRING,
    [185] = IN_SQ_STRING,
    [186] = IN_SQ_STRING,
    [187] = IN_SQ_STRING,
    [188] = IN_SQ_STRING,
    [189] = IN_SQ_STRING,
    [190] = IN_SQ_STRING,
    [191] = IN_SQ_STRING,
    [192] = IN_SQ_STRING,
    [193] = IN_SQ_STRING,
    [194] = IN_SQ_STRING,
    [195] = IN_SQ_STRING,
    [196] = IN_SQ_STRING,
    [197] = IN_SQ_STRING,
    [198] = IN_SQ_STRING,
    [199] = IN_SQ_STRING,
    [200] = IN_SQ_STRING,
    [201] = IN_SQ_STRING,
    [202] = IN_SQ_STRING,
    [203] = IN_SQ_STRING,
    [204] = IN_SQ_STRING,
    [205] = IN_SQ_STRING,
    [206] = IN_SQ_STRING,
    [207] = IN_SQ_STRING,
    [208] = IN_SQ_STRING,
    [209] = IN_SQ_STRING,
    [210] = IN_SQ_STRING,
    [211] = IN_SQ_STRING,
    [212] = IN_SQ_STRING,
    [213] = IN_SQ_STRING,
    [214] = IN_SQ_STRING,
    [215] = IN_SQ_STRING,
    [216] = IN_SQ_STRING,
    [217] = IN_SQ_STRING,
    [218] = IN_SQ_STRING,
    [219] = IN_SQ_STRING,
    [220] = IN_SQ_STRING,
    [221] = IN_SQ_STRING,
    [222] = IN_SQ_STRING,
    [223] = IN_SQ_STRING,
    [224] = IN_SQ_STRING,
    [225] = IN_SQ_STRING,
    [226] = IN_SQ_STRING,
    [227] = IN_SQ_STRING,
    [228] = IN_SQ_STRING,
    [229] = IN_SQ_STRING,
    [230] = IN_SQ_STRING,
    [231] = IN_SQ_STRING,
    [232] = IN_SQ_STRING,
    [233] = IN_SQ_STRING,
    [234] = IN_SQ_STRING,
    [235] = IN_SQ_STRING,
    [236] = IN_SQ_STRING,
    [237] = IN_SQ_STRING,
    [238] = IN_SQ_STRING,
    [239] = IN_SQ_STRING,
    [240] = IN_SQ_STRING,
    [241] = IN_SQ_STRING,
    [242] = IN_SQ_STRING,
    [243] = IN_SQ_STRING,
    [244] = IN_SQ_STRING,
    [245] = IN_SQ_STRING,
    [246] = IN_SQ_STRING,
    [247] = IN_SQ_STRING,
    [248] = IN_SQ_STRING,
    [249] = IN_SQ_STRING,
    [250] = IN_SQ_STRING,
    [251] = IN_SQ_STRING,
    [252] = IN_SQ_STRING,
    [253] = IN_SQ_STRING,
    [254] = IN_SQ_STRING,
    [255] = IN_SQ_STRING,
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

static int json_lexer_feed_char(JSONLexer *lexer, char ch)
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
            return -EINVAL;
        default:
            break;
        }
        lexer->state = new_state;
    } while (!char_consumed);
    return 0;
}

int json_lexer_feed(JSONLexer *lexer, const char *buffer, size_t size)
{
    size_t i;

    for (i = 0; i < size; i++) {
        int err;

        err = json_lexer_feed_char(lexer, buffer[i]);
        if (err < 0) {
            return err;
        }
    }

    return 0;
}

int json_lexer_flush(JSONLexer *lexer)
{
    return lexer->state == IN_START ? 0 : json_lexer_feed_char(lexer, 0);
}

void json_lexer_destroy(JSONLexer *lexer)
{
    QDECREF(lexer->token);
}
