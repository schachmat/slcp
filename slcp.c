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


/* function declarations */
static unsigned int get_term_width();
static void catfg(unsigned int col);
static void cats(const char* str, size_t len);
static void catscol(const char* str, unsigned int col);


static void cats(const char* str, size_t len)
{
	size_t i;
	for(i = 0; i < len && str[i]; i++)
		fputc(str[i], stdout);
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

static void catscol(const char* str, unsigned int col)
{
	catfg(col);
	fputs(str, stdout);
}

int main(int argc, char* argv[])
{
	char hostname[16];
	char tmpgitd[MAX_PATH];
	char* git_ahead = NULL;
	char* git_behind = NULL;
	char* git_state = NULL;
	char* gitd = NULL;
	char* homed = NULL;
	char* idx;
	char* name;
	char* origpwd = NULL;
	char* pwd = NULL;
	char* username;
	const char* git_local_branch_name = NULL;
	const char* git_remote_branch_name = NULL;
	const char* origgitd;
	const git_oid* id_local = NULL;
	const git_oid* id_remote = NULL;
	git_reference* direct_local = NULL;
	git_reference* direct_remote = NULL;
	git_reference* git_local_branch = NULL;
	git_reference* git_remote_branch = NULL;
	git_repository* git_repo = NULL;
	int len = 0;
	size_t ahead = 0;
	size_t behind = 0;
	size_t i = 0;
	size_t lengit = 0;
	size_t lenpwd;

	// init git repo
	if(!git_repository_discover(tmpgitd, MAX_PATH, ".", 0, NULL)
	&& git_repository_open(&git_repo, tmpgitd)) {
		git_repo = NULL;
	}

	// prepare some git information
	if(git_repo) {
		// prepare git repository state
#define GS(length, str) {git_state = str;} //return length;
		switch(git_repository_state(git_repo)) {
			case GIT_REPOSITORY_STATE_NONE:
			case -1:
				GS(0, "")
				break;
			case GIT_REPOSITORY_STATE_APPLY_MAILBOX:
				GS(2, "am")
				break;
			case GIT_REPOSITORY_STATE_MERGE:
				GS(5, "merge")
				break;
			case GIT_REPOSITORY_STATE_REVERT:
				GS(6, "revert")
				break;
			case GIT_REPOSITORY_STATE_CHERRY_PICK:
				GS(6, "cherry")
				break;
			case GIT_REPOSITORY_STATE_BISECT:
				GS(6, "bisect")
				break;
			case GIT_REPOSITORY_STATE_REBASE:
				GS(6, "rebase")
				break;
			case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
				GS(7, "irebase")
				break;
			case GIT_REPOSITORY_STATE_REBASE_MERGE:
				GS(7, "rbmerge")
				break;
			case GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE:
				GS(5, "am/rb")
				break;
			default:
				GS(5, "ERROR")
		}

		// prepare local git branch
		if(git_repository_head(&git_local_branch, git_repo)
		|| git_branch_name(&git_local_branch_name, git_local_branch)) {
			git_local_branch = NULL;
			git_local_branch_name = "";
			//return 0;
		} else {
			//return strlen(git_local_branch_name);
		}

		// prepare remote git branch
		if(!git_local_branch
		|| git_branch_upstream(&git_remote_branch, git_local_branch)
		|| git_branch_name(&git_remote_branch_name, git_remote_branch)) {
			git_remote_branch = NULL;
			git_remote_branch_name = "";
			//return 0;
		} else {
			//return strlen(git_remote_branch_name);
		}

		// prepare git repository current branch ahead/behind values
		git_ahead = malloc(4);
		git_behind = malloc(4);
		if(!git_local_branch
		|| !git_remote_branch
		|| git_reference_resolve(&direct_local, git_local_branch)
		|| git_reference_resolve(&direct_remote, git_remote_branch)
		|| !(id_local = git_reference_target(direct_local))
		|| !(id_remote = git_reference_target(direct_remote))
		|| git_graph_ahead_behind(&ahead, &behind, git_repo, id_local, id_remote)) {
			git_ahead[0] = '\0';
			git_behind[0] = '\0';
		} else {
			if(ahead == 0)
				git_ahead[0] = '\0';
			else if(ahead < 1000)
				len += snprintf(git_ahead, 4, "%zu", ahead);
			else
				len += snprintf(git_ahead, 4, "\u2026");
		}
		git_reference_free(direct_local);
		git_reference_free(direct_remote);
		//return len;
	}

	// draw pwd
	if(!(origpwd = getcwd(NULL, 0))) {
		catscol("ERROR", col_error);
		//return 5;
	} else {
		if(git_repo && (origgitd = git_repository_workdir(git_repo))) {
			// get pointers and len of outside-repo- and inside-repo-path
			for(i=0; origpwd[i] && origgitd[i] && origpwd[i] == origgitd[i]; i++);
			for(i -= !origpwd[i] ? 1 : 2; i>=0 && origpwd[i] != '/'; i--);
			i++;
			gitd = origpwd + i;
			for(lenpwd=i, lengit=0; origpwd[lenpwd]; lengit++, lenpwd++);
		} else {
			lenpwd = strlen(origpwd);
		}

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
				if(gitd) catscol(gitd, col_git_pwd);
			}
		} else {
			catfg(col_pwd);
			cats(pwd, lenpwd - lengit);
			if(gitd) catscol(gitd, col_git_pwd);
		}
		free(origpwd);
		//return lenpwd;
	}

	// draw git state
	if(git_repo) {
		catscol(git_state, col_git_state);
		catscol("@", NYAN_WHITE);
		catscol(git_local_branch_name, col_git_state);
		catscol(":", NYAN_WHITE);
		catscol(git_remote_branch_name, col_git_state);
	}

	// second line
	fputc('\n', stdout);

	// draw username
	if((username = getenv("USER")) || (username = getenv("LOGNAME")))
		catscol(username, col_user);
	else
		catscol("ERROR", col_error);

	catscol("@", NYAN_WHITE);

	// draw hostname
	if(!gethostname(hostname, 15)) {
		hostname[15] = '\0';
		catscol(hostname, col_host);
	} else
		catscol("ERROR", col_error);

	catscol(":", NYAN_WHITE);

	// draw pts name
	if((name = ttyname(0))) {
		for(idx = name; *idx; idx++)
			if(*idx == '/')
				name = idx + 1;
		catscol(name, col_ptsname);
	} else {
		catscol("ERROR", col_error);
	}

	// status code of last programm if error.
	if(argc > 1 && strcmp(argv[1], "0")) {
		catscol("?", NYAN_WHITE);
		catscol(argv[1], col_error);
	}

	// draw prompt
	catscol("$ ", col_prompt);
	// reset colors
	fputs("\033[0m", stdout);

	git_repository_free(git_repo);
	git_reference_free(git_local_branch);
	git_reference_free(git_remote_branch);
	if(git_ahead) {
		free(git_ahead);
		git_ahead = NULL;
	}
	if(git_behind) {
		free(git_behind);
		git_behind = NULL;
	}

	return 0;
}
