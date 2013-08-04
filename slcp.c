// slcp - suckless C prompt
// See LICENSE file for copyright and license details.

#include <git2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LENPWD 42
#define MAX_PATH 4096

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

#include "config.h"


/* global variables */
static git_repository* git_repo = NULL;
static git_reference* git_local_branch = NULL;
static git_reference* git_remote_branch = NULL;
static const char* git_local_branch_name = NULL;
static const char* git_remote_branch_name = NULL;
static char* git_state = NULL;


/* function declarations */
static int catpwd();
static void git_init();
static void git_exit();
static int prepgitstate();
static unsigned int get_term_width();
static void catfg(unsigned int col);
static void cathost();
static void catprompt();
static void catptsname();
static void catreset();
static void cats(const char* str, size_t len);
static void catscol(const char* str, unsigned int col);
static void catuser();


static void cats(const char* str, size_t len)
{
	size_t i;
	for(i = 0; i < len && str[i]; i++)
		fputc(str[i], stdout);
}

static void git_init()
{
	char tmpgitd[MAX_PATH];
	if(!git_repository_discover(tmpgitd, MAX_PATH, ".", 0, NULL)
		&& git_repository_open(&git_repo, tmpgitd))
		git_repo = NULL;
}

static void git_exit()
{
	git_repository_free(git_repo);
}

static int prepgitstate()
{
#define GS(len, str) {git_state = str; return len;}
	git_repository_state_t state = git_repository_state(git_repo);
	if(state == GIT_REPOSITORY_STATE_NONE || state == -1)     GS(0, "")
	if(state == GIT_REPOSITORY_STATE_APPLY_MAILBOX)           GS(2, "am")
	if(state == GIT_REPOSITORY_STATE_MERGE)                   GS(5, "merge")
	if(state == GIT_REPOSITORY_STATE_REVERT)                  GS(6, "revert")
	if(state == GIT_REPOSITORY_STATE_CHERRY_PICK)             GS(6, "cherry")
	if(state == GIT_REPOSITORY_STATE_BISECT)                  GS(6, "bisect")
	if(state == GIT_REPOSITORY_STATE_REBASE)                  GS(6, "rebase")
	if(state == GIT_REPOSITORY_STATE_REBASE_INTERACTIVE)      GS(7, "irebase")
	if(state == GIT_REPOSITORY_STATE_REBASE_MERGE)            GS(7, "rbmerge")
	if(state == GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE) GS(9, "am/rebase")
	GS(5, "ERROR")
}

static int prepgitblocal()
{
	if(git_repository_head(&git_local_branch, git_repo)
	|| git_branch_name(&git_local_branch_name, git_local_branch)) {
		git_local_branch = NULL;
		git_local_branch_name = NULL;
		return 0;
	}
	return strlen(git_local_branch_name);
}

static int prepgitbremote()
{
	if(!git_local_branch
	|| git_branch_upstream(&git_remote_branch, git_local_branch)
	|| git_branch_name(&git_remote_branch_name, git_remote_branch)) {
		git_remote_branch = NULL;
		git_remote_branch_name = NULL;
		return 0;
	}
	return strlen(git_remote_branch_name);
}

static void catgitstate()
{
	catscol(git_state, col_git_state);
	catscol("@", NYAN_WHITE);
	catscol(git_local_branch_name, col_git_state);
	catscol(":", NYAN_WHITE);
	catscol(git_remote_branch_name, col_git_state);
}

static unsigned int get_term_width()
{
	struct winsize sz;
	memset(&sz, 0, sizeof(sz));

	if(!ioctl(1, TIOCGWINSZ, &sz))
		return sz.ws_col;
	return 0;
}

static void catfg(unsigned int col)
{
	fputs("\033[3", stdout);
	fputc((col > 7 ? 0 : col) + '0', stdout);
	fputc('m', stdout);
}

static void catreset()
{
	fputs("\033[0m", stdout);
}

static void catscol(const char* str, unsigned int col)
{
	catfg(col);
	fputs(str, stdout);
}

static void catuser()
{
	char* username;
	if((username = getenv("USER")) || (username = getenv("LOGNAME")))
		catscol(username, col_user);
	else
		catscol("ERROR", col_error);
}

static void cathost()
{
	char hostname[16];
	if(!gethostname(hostname, 15)) {
		hostname[15] = '\0';
		catscol(hostname, col_host);
	} else
		catscol("ERROR", col_error);
}

static void catprompt()
{
	catscol("$ ", col_prompt);
}

static void catptsname()
{
	char* name;
	char* i;
	if((name = ttyname(0))) {
		for(i = name; *i; i++)
			if(*i == '/')
				name = i + 1;
		catscol(name, col_ptsname);
	} else
		catscol("ERROR", col_error);
}

static int catpwd()
{
	char* origpwd = NULL;
	char* pwd = NULL;
	const char* origgitd;
	char* gitd = NULL;
	char* homed = NULL;
	size_t i = 0;
	size_t lenpwd;
	size_t lengit = 0;

	if(!(origpwd = getcwd(NULL, 0)))
		goto err;

	if(git_repo && (origgitd = git_repository_workdir(git_repo))) {
		// get pointers and len of outside-repo- and inside-repo-path
		for(i=0; origpwd[i] && origgitd[i] && origpwd[i] == origgitd[i]; i++);
		for(i -= !origpwd[i] ? 1 : 2; i>=0 && origpwd[i] != '/'; i--);
		i++;
		gitd = origpwd + i;
		for(lenpwd=i, lengit=0; origpwd[lenpwd]; lengit++, lenpwd++);
	} else
		lenpwd = strlen(origpwd);

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
		if(lengit >= MAX_LENPWD) {
			catscol("\u2026", col_git_pwd);
			cats(gitd + lengit + 1 - MAX_LENPWD, MAX_LENPWD - 1);
		} else {
			catscol("\u2026", col_pwd);
			cats(pwd + lenpwd + 1 - MAX_LENPWD, MAX_LENPWD - lengit - 1);
			if(gitd) {
				catscol(gitd, col_git_pwd);
			}
		}
	} else {
		catfg(col_pwd);
		cats(pwd, lenpwd - lengit);
		if(gitd) {
			catscol(gitd, col_git_pwd);
		}
	}
	free(origpwd);
	return lenpwd;

err:
	catscol("ERROR", col_error);
	return 5;
}

int main(int argc, char* argv[])
{
	unsigned int width;

	git_init();
	if(git_repo) {
		prepgitstate();
		prepgitblocal();
		prepgitbremote();
	}
	catpwd();
	if(git_repo) {
		catgitstate();
	}
	fputc('\n', stdout);

	catuser();
	catscol("@", NYAN_WHITE);
	cathost();
	catscol(":", NYAN_WHITE);
	catptsname();

	// status code of last programm if error.
	if(argc > 1 && strcmp(argv[1], "0")) {
		catscol("?", NYAN_WHITE);
		catscol(argv[1], col_error);
	}

	catprompt();
	catreset();

	git_exit();
	return 0;
}
