/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <teichm@in.tum.de> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return Markus Teich
 * ----------------------------------------------------------------------------
 */

#include <buffer.h>
#include <git2.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stralloc.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define MAX_LENPWD 42

/* colors */
#define NYAN_BLACK		0x00
#define NYAN_RED		0x01
#define NYAN_GREEN		0x02
#define NYAN_YELLOW		0x03
#define NYAN_BLUE		0x04
#define NYAN_MAGENTA	0x05
#define NYAN_CYAN		0x06
#define NYAN_WHITE		0x07
#define NYAN_DEFAULT	0x09


static int catscol(stralloc* buf, char* str, uint16_t col);
static int utf8_unicode_to_char(char *out, uint32_t c);
static int catpwd(stralloc* buf);
static unsigned int get_term_width();
static int catfg(stralloc* buf, uint16_t col);
static int catreset(stralloc* buf);

static int utf8_unicode_to_char(char *out, uint32_t c)
{
	int len = 0;
	int first;
	int i;

	if (c < 0x80) {
		first = 0;
		len = 1;
	} else if (c < 0x800) {
		first = 0xc0;
		len = 2;
	} else if (c < 0x10000) {
		first = 0xe0;
		len = 3;
	} else if (c < 0x200000) {
		first = 0xf0;
		len = 4;
	} else if (c < 0x4000000) {
		first = 0xf8;
		len = 5;
	} else {
		first = 0xfc;
		len = 6;
	}

	for (i = len - 1; i > 0; --i) {
		out[i] = (c & 0x3f) | 0x80;
		c >>= 6;
	}
	out[0] = c | first;

	return len;
}

static unsigned int get_term_width()
{
	struct winsize sz;
	memset(&sz, 0, sizeof(sz));

	if(!ioctl(1, TIOCGWINSZ, &sz))
		return sz.ws_col;
	return 0;
}

static int catfg(stralloc* buf, uint16_t col)
{
	stralloc_cats(buf, "\033[3");
	stralloc_catulong0(buf, (unsigned long int)(col > 7 ? 0 : col), 1);
	stralloc_cats(buf, "m");
	return 5;
}

static int catreset(stralloc* buf)
{
	stralloc_cats(buf, "\033[0m");
	return 4;
}

static int catscol(stralloc* buf, char* str, uint16_t col)
{
	catfg(buf, col);
	stralloc_cats(buf, str);
	return 0;
}

static int catpwd(stralloc* buf)
{
	char* bufc = NULL;
	char* pwdc = NULL; //malloc(PATH_MAX * sizeof(char));
	char* tmpc = malloc(PATH_MAX * sizeof(char));
	char* gitc = NULL;
	git_repository* repo = NULL;
	size_t i = 0;
	size_t lenpwd;

	if(!(pwdc = getcwd(NULL, 0))) //pwdc, PATH_MAX)))
		goto err;

	if(!git_repository_discover(tmpc, PATH_MAX, ".", 0, "")
	&& !git_repository_open(&repo, tmpc) && repo
	&& (gitc = git_repository_workdir(repo))) {
		// get pointers and len of outside-repo- and inside-repo-path
		for(i=0; pwdc[i] && gitc[i] && pwdc[i] == gitc[i]; i++);
		for(i -= !pwdc[i] ? 1 : 2; i>=0 && pwdc[i] != '/'; i--);
		gitc[++i] = '\0';
		free(tmpc);
		bufc = (tmpc=pwdc)+i;
		pwdc = gitc;
		gitc = bufc;
		for(lenpwd=i, i=0; gitc[i]; i++, lenpwd++);
	} else
		lenpwd = strlen(pwdc);

	if(lenpwd > MAX_LENPWD) {
		bufc = malloc(7 * sizeof(char));
		utf8_unicode_to_char(bufc, 8230);
		if(i >= MAX_LENPWD) {
			catscol(buf, bufc, NYAN_WHITE);
			stralloc_cats(buf, gitc + i + 1 - MAX_LENPWD);
		} else {
			catscol(buf, bufc, NYAN_GREEN);
			stralloc_cats(buf, pwdc + lenpwd + 1 - MAX_LENPWD);
			if(gitc) {
				catscol(buf, gitc, NYAN_WHITE);
			}
		}
		free(bufc);
	} else {
		catscol(buf, pwdc, NYAN_GREEN);
		if(gitc) {
			catscol(buf, gitc, NYAN_WHITE);
		}
	}
	catreset(buf);
	if(!gitc) free(pwdc);
	free(tmpc);
	git_repository_free(repo);
	return lenpwd;

err:
	catfg(buf, NYAN_RED);
	stralloc_cats(buf, "ERROR");
	catreset(buf);
	free(gitc);
	return 5;
}

int main(int argc, char* argv[])
{
	unsigned int width;
	stralloc prompt;

	stralloc_init(&prompt);
	if((width = get_term_width()))
		stralloc_ready(&prompt, 3*width);

	catpwd(&prompt);
	stralloc_cats(&prompt, "$ ");

	buffer_putsaflush(buffer_1, &prompt);

	stralloc_free(&prompt);
	return 0;
}
