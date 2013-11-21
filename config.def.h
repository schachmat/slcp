// slcp - suckless C prompt
// See LICENSE file for copyright and license details.

const unsigned int col_git_pwd   = NYAN_WHITE;
const unsigned int col_git_state = NYAN_YELLOW;
const unsigned int col_pwd       = NYAN_GREEN;
const unsigned int col_error     = NYAN_RED;
const unsigned int col_prompt    = NYAN_CYAN;
const unsigned int col_ptsname   = NYAN_MAGENTA;
const unsigned int col_user      = NYAN_YELLOW;
const unsigned int col_host      = NYAN_BLUE;

// How much do we want to see from the PWD
#define LEN_PWD_MIN 32
#define LEN_SPACER_MIN 4

// This allows to setup ignoring the colorcodes in prompt length
// mksh
#define PROMPT_PREFIX "\001\r"
#define PROMPT_EXCLUDE_BEGIN "\001"
#define PROMPT_EXCLUDE_END "\001"
// bash
//#define PROMPT_PREFIX ""
//#define PROMPT_EXCLUDE_BEGIN "\\["
//#define PROMPT_EXCLUDE_END "\\]"
