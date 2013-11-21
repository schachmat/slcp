// slcp - suckless C prompt
// See LICENSE file for copyright and license details.

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
