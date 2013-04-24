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
	out[len] = '\0';
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
	char* origpwd = NULL;
	char* pwd = NULL;
	const char* origgitd;
	char* tmpgitd = malloc(PATH_MAX * sizeof(char));
	char* gitd = NULL;
	char* homed = NULL;
	git_repository* repo = NULL;
	size_t i = 0;
	size_t lenpwd;
	size_t lengit = 0;

	if(!(origpwd = getcwd(NULL, 0)))
		goto err;

	if(!git_repository_discover(tmpgitd, PATH_MAX, ".", 0, "")
	&& !git_repository_open(&repo, tmpgitd) && repo
	&& (origgitd = git_repository_workdir(repo))) {
		// get pointers and len of outside-repo- and inside-repo-path
		for(i=0; origpwd[i] && origgitd[i] && origpwd[i] == origgitd[i]; i++);
		for(i -= !origpwd[i] ? 1 : 2; i>=0 && origpwd[i] != '/'; i--);
		i++;
		gitd = origpwd + i;
		for(lenpwd=i, lengit=0; origpwd[lenpwd]; lengit++, lenpwd++);
	} else
		lenpwd = strlen(origpwd);
	git_repository_free(repo);
	if(tmpgitd) free(tmpgitd);

	pwd = origpwd;
	if((homed = getenv("HOME"))) {
		for(i=1; homed[i] && homed[i] == origpwd[i]; i++);
		if(!homed[i]) {
			origpwd[--i] = '~';
			pwd = origpwd + i;
			lenpwd -= i;
			if(gitd && gitd < pwd) {
				gitd = pwd;
				lengit -= i;
			}
		}
	}

	if(lenpwd > MAX_LENPWD) {
		char* bufc = malloc(7 * sizeof(char));
		utf8_unicode_to_char(bufc, 8230);
		if(lengit >= MAX_LENPWD) {
			catscol(buf, bufc, NYAN_WHITE);
			stralloc_catb(buf, gitd + lengit + 1 - MAX_LENPWD, MAX_LENPWD - 1);
		} else {
			catscol(buf, bufc, NYAN_GREEN);
			stralloc_catb(buf, pwd + lenpwd + 1 - MAX_LENPWD, MAX_LENPWD - lengit - 1);
			if(gitd) {
				catscol(buf, gitd, NYAN_WHITE);
			}
		}
		free(bufc);
	} else {
		catfg(buf, NYAN_GREEN);
		stralloc_catb(buf, pwd, lenpwd - lengit);
		if(gitd) {
			catscol(buf, gitd, NYAN_WHITE);
		}
	}
	free(origpwd);
	return lenpwd;

err:
	catfg(buf, NYAN_RED);
	stralloc_cats(buf, "ERROR");
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
	catscol(&prompt, "$ ", NYAN_CYAN);
	catreset(&prompt);

	buffer_putsaflush(buffer_1, &prompt);

	stralloc_free(&prompt);
	return 0;
}
