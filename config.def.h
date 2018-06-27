// slcp - suckless C prompt
// See LICENSE file for copyright and license details.

// How much do we want to see from the PWD
#define LEN_PWD_MIN 32
#define LEN_SPACER_MIN 4

// configure appearance
const unsigned int col_host = NYAN_BLUE;

// This allows to setup ignoring the colorcodes in prompt length
// bash
#define PROMPT_PREFIX ""
#define PROMPT_EXCLUDE_BEGIN "\x01"
#define PROMPT_EXCLUDE_END "\x02"
// mksh
//#define PROMPT_PREFIX "\x01\r"
//#define PROMPT_EXCLUDE_BEGIN "\x01"
//#define PROMPT_EXCLUDE_END "\x01"
