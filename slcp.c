/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <teichm@in.tum.de> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return Markus Teich
 * ----------------------------------------------------------------------------
 */

#include <buffer.h>
#include <limits.h>
#include <stdlib.h>
#include <stralloc.h>
#include <termininfo.h>
#include <unistd.h>

#define MAX_LENPWD 42

stralloc* getpwd()
{
	char* buf = NULL;
	char* pwdc = malloc(PATH_MAX * sizeof(char));
	size_t lenpwd;
	stralloc* pwd = malloc(sizeof(stralloc));

	stralloc_init(pwd);
	if(!(pwdc = getcwd(pwdc, PATH_MAX)))
		stralloc_copys(pwd, "ERROR");
	else if((lenpwd = strlen(pwdc)) > MAX_LENPWD) {
		buf = malloc(7 * sizeof(char));
		utf8_unicode_to_char(buf, 8230);
		stralloc_copys(pwd, buf);
		stralloc_cats(pwd, pwdc + lenpwd + 1 - MAX_LENPWD);
	} else
		stralloc_copys(pwd, pwdc);
	return pwd;
}

int main(int argc, char* argv[])
{
	unsigned int width;
	stralloc prompt;

	tb_init();
	width = tb_width();
	stralloc_init(&prompt);
	stralloc_ready(&prompt, 2*width);

	stralloc_copy(&prompt, getpwd());
	stralloc_cats(&prompt, "$ ");
	buffer_putsaflush(buffer_1, &prompt);
	tb_shutdown();

	return 0;
}
