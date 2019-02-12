/* slcp - suckless C prompt
 * See LICENSE file for copyright and license details. */

#include <git2.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH 4096
#define FREE_NON_NULL(p) if (p) {free (p); p = NULL;}

/* colors */
#define NYAN_BLACK    0x00
#define NYAN_RED      0x01
#define NYAN_GREEN    0x02
#define NYAN_YELLOW   0x03
#define NYAN_BLUE     0x04
#define NYAN_MAGENTA  0x05
#define NYAN_CYAN     0x06
#define NYAN_WHITE    0x07
#define NYAN_DEFAULT  0x09

#include "config.h"


/* function prototypes */
static size_t catslen(const char* str, size_t len);
static void catscol(const char* str, unsigned int col);
static void change_col(unsigned int col);


/* print first len characters of string str to stdout. Stop at '\0'. return
 * count of characters not printed. */
static size_t catslen(const char* str, size_t len)
{
	size_t i;
	for (i = 0; i < len && str[i]; i++)
		fputc(str[i], stdout);
	return len - i;
}

/* print escapecode to switch to another foreground color and also exclude it
 * from the length calculation of the shell */
static void change_col(unsigned int col)
{
	fputs(PROMPT_EXCLUDE_BEGIN, stdout);
	fputs("\033[3", stdout);
	fputc((col > 7 ? 0 : col) + '0', stdout);
	fputc('m', stdout);
	fputs(PROMPT_EXCLUDE_END, stdout);
}

/* print string str in color col. Does not change the color back to the previous
 * value. */
static void catscol(const char* str, unsigned int col)
{
	change_col(col);
	fputs(str, stdout);
}

int main(int argc, char* argv[])
{
	char fqdn[256];
	git_buf tmpgitdb = { 0 };
	char* git_ahead = malloc(4);
	char* git_behind = malloc(4);
	char* hostname = NULL;
	char* origpwd = NULL;
	char* venv = NULL;
	const char* git_local_branch_name = "";
	const char* git_remote_branch_name = "";
	const char* git_state = "";
	const char* gitd = NULL;
	const char* homed = NULL;
	const char* idx = NULL;
	const char* origgitd = NULL;
	const char* pwd = NULL;
	const char* termname = NULL;
	const char* username = NULL;
	const git_oid* id_loc = NULL;
	const git_oid* id_rem = NULL;
	git_reference* direct_local = NULL;
	git_reference* direct_remote = NULL;
	git_reference* git_local_branch = NULL;
	git_reference* git_remote_branch = NULL;
	git_repository* git_repo = NULL;
	size_t ahead = 0;
	size_t behind = 0;
	size_t i = 0;
	size_t lengit = 0;
	size_t lengitpath = 0;
	size_t lenpwd = 0;
	size_t lenpwdmax = 0;
	size_t lentmp = 0;
	size_t termwidth = 0;
	unsigned int git_local_branch_col = NYAN_CYAN;

	/* get term width */
	if (argc > 1) termwidth = atoi(argv[1]);

	/* init git repo */
	git_libgit2_init();
	if (!git_repository_discover(&tmpgitdb, ".", 0, NULL)
	&& git_repository_open(&git_repo, tmpgitdb.ptr))
	{
		git_repo = NULL;
	}
	git_buf_dispose(&tmpgitdb);

	/* prepare some git information */
	if (git_repo) {
		git_ahead[0] = '\0';
		git_behind[0] = '\0';

		/* prepare local git branch */
		if (git_repository_head(&git_local_branch, git_repo)
		|| git_branch_name(&git_local_branch_name, git_local_branch))
		{
			git_local_branch = NULL;
			git_local_branch_name = "detached";
			git_local_branch_col = NYAN_YELLOW;
		}
		lentmp = strlen(git_local_branch_name);
		if (LEN_PWD_MIN + LEN_SPACER_MIN + lentmp > termwidth) {
			git_local_branch_name = "";
			goto draw;
		} else {
			lengit += lentmp;
		}

		/* prepare git repository state */
		switch (git_repository_state(git_repo)) {
			case GIT_REPOSITORY_STATE_NONE:
			case -1:
				git_state = "";
				lentmp = 0;
				break;
			case GIT_REPOSITORY_STATE_APPLY_MAILBOX:
				git_state = "am";
				lentmp = 2;
				break;
			case GIT_REPOSITORY_STATE_MERGE:
				git_state = "merge";
				lentmp = 5;
				break;
			case GIT_REPOSITORY_STATE_REVERT:
				git_state = "revert";
				lentmp = 6;
				break;
			case GIT_REPOSITORY_STATE_CHERRYPICK:
				git_state = "cherry";
				lentmp = 6;
				break;
			case GIT_REPOSITORY_STATE_BISECT:
				git_state = "bisect";
				lentmp = 6;
				break;
			case GIT_REPOSITORY_STATE_REBASE:
				git_state = "rebase";
				lentmp = 6;
				break;
			case GIT_REPOSITORY_STATE_REBASE_INTERACTIVE:
				git_state = "irebase";
				lentmp = 7;
				break;
			case GIT_REPOSITORY_STATE_REBASE_MERGE:
				git_state = "rbmerge";
				lentmp = 7;
				break;
			case GIT_REPOSITORY_STATE_APPLY_MAILBOX_OR_REBASE:
				git_state = "am/rb";
				lentmp = 5;
				break;
			default:
				git_state = "ERROR";
				lentmp = 5;
		}
		if (LEN_PWD_MIN + LEN_SPACER_MIN + lentmp + 1 + lengit > termwidth) {
			git_state = "";
			goto draw;
		} else {
			lengit += lentmp + (lentmp ? 1 : 0);
		}

		/* prepare remote git branch */
		if (git_local_branch_col == NYAN_YELLOW) goto draw;
		if (!git_local_branch
		|| git_branch_upstream(&git_remote_branch, git_local_branch)
		|| git_branch_name(&git_remote_branch_name, git_remote_branch))
		{
			git_remote_branch = NULL;
			git_remote_branch_name = "";
		}
		lentmp = strlen(git_remote_branch_name);

		/* prepare git repository current branch ahead/behind values */
		if (git_local_branch
		&& git_remote_branch
		&& !git_reference_resolve(&direct_local, git_local_branch)
		&& !git_reference_resolve(&direct_remote, git_remote_branch)
		&& (id_loc = git_reference_target(direct_local))
		&& (id_rem = git_reference_target(direct_remote))
		&& !git_graph_ahead_behind(&ahead, &behind, git_repo, id_loc, id_rem))
		{
			if (ahead > 0 && ahead < 1000) {
				lentmp += snprintf(git_ahead, 4, "%zu", ahead);
			} else if (ahead > 999) {
				lentmp += snprintf(git_ahead, 4, "\u2026") ? 1 : 0;
			}
			if (behind > 0 && behind < 1000) {
				lentmp += snprintf(git_behind, 4, "%zu", behind);
			} else if (behind > 999) {
				lentmp += snprintf(git_behind, 4, "\u2026") ? 1 : 0;
			}
		}
		git_reference_free(direct_local);
		git_reference_free(direct_remote);
		if (LEN_PWD_MIN + LEN_SPACER_MIN + lengit + 2 + lentmp > termwidth) {
			git_remote_branch_name = "";
			git_ahead[0] = '\0';
			git_behind[0] = '\0';
		} else {
			lengit += lentmp + 2;
		}
	}

draw:
	lenpwdmax = termwidth - lengit - LEN_SPACER_MIN;
	fputs(PROMPT_PREFIX, stdout);

	/* draw pwd */
	if (!(origpwd = getcwd(NULL, 0))) {
		catscol("ERROR", NYAN_RED);
		lenpwd = 5;
	} else {
		if (git_repo && (origgitd = git_repository_workdir(git_repo))) {
			/* get pointers and len of outside-repo- and inside-repo-path */
			for (i = 0; origpwd[i] && origpwd[i] == origgitd[i]; i++);
			for (i -= !origpwd[i] ? 1 : 2; origpwd[i] != '/'; i--) {
				if (i == 0)
					break;
			}
			i++;
			gitd = origpwd + i;
			for (lengitpath = 0; origpwd[i + lengitpath]; lengitpath++);
			lenpwd = i + lengitpath;
		} else {
			lenpwd = strlen(origpwd);
		}

		pwd = origpwd;
		if ((homed = getenv("HOME"))) {
			for (i=1; homed[i] && homed[i] == origpwd[i]; i++);
			if (!homed[i]) {
				origpwd[--i] = '~';
				pwd = origpwd + i;
				lenpwd -= i;
				if (gitd && gitd < pwd) {
					gitd = pwd;
					lengitpath -= i;
				}
			}
		}

		if (lenpwd > lenpwdmax) {
			if (lengitpath >= lenpwdmax) {
				catscol("\u2026", NYAN_WHITE);
				catslen(gitd + lengitpath + 1 - lenpwdmax, lenpwdmax - 1);
			} else {
				catscol("\u2026", NYAN_CYAN);
				catslen(pwd + lenpwd + 1 - lenpwdmax, lenpwdmax - lengitpath - 1);
				if (gitd) catscol(gitd, NYAN_WHITE);
			}
			lenpwd = lenpwdmax;
		} else {
			change_col(NYAN_CYAN);
			catslen(pwd, lenpwd - lengitpath);
			if (gitd) catscol(gitd, NYAN_WHITE);
		}
	}
	FREE_NON_NULL(origpwd);

	/* draw spacer */
	for (i = 0; lenpwd + i + lengit < termwidth; i++) fputc(' ', stdout);

	/* draw git state */
	if (git_repo) {
		catscol(git_state, NYAN_YELLOW);
		if (*git_state != '\0') catscol("@", NYAN_WHITE);
		catscol(git_local_branch_name, git_local_branch_col);
		if (*git_remote_branch_name != '\0') catscol("<", NYAN_WHITE);
		catscol(git_behind, NYAN_GREEN);
		catscol(git_ahead, NYAN_RED);
		if (*git_remote_branch_name != '\0') catscol(">", NYAN_WHITE);
		catscol(git_remote_branch_name, NYAN_CYAN);
	}

	/* second line */
	fputc('\n', stdout);

	/* virtual env */
	if ((venv = getenv("VIRTUAL_ENV"))) {
		change_col(NYAN_YELLOW);
		fputc('(', stdout);
		fputs(basename(venv), stdout);
		fputc(')', stdout);
	}

	/* draw username */
	if ((username = getenv("USER")) || (username = getenv("LOGNAME")))
		catscol(username, NYAN_CYAN);
	else
		catscol("ERROR", NYAN_RED);

	catscol("@", NYAN_WHITE);

	/* draw hostname */
	if (!gethostname(fqdn, 255)) {
		fqdn[255] = '\0';
		if(!(hostname = strtok(fqdn, ".")))
			catscol("ERROR", NYAN_RED);
		else
			catscol(hostname, col_host);
	} else
		catscol("ERROR", NYAN_RED);

	catscol(":", NYAN_WHITE);

	/* draw pts name */
	if ((termname = ttyname(0))) {
		for (idx = termname; *idx; idx++)
			if (*idx == '/')
				termname = idx + 1;
		catscol(termname, NYAN_CYAN);
	} else {
		catscol("ERROR", NYAN_RED);
	}

	/* status code of last programm if error. */
	if (argc > 2 && strcmp(argv[2], "0")) {
		catscol("?", NYAN_WHITE);
		catscol(argv[2], NYAN_RED);
	}

	/* draw prompt */
	catscol("$ ", NYAN_YELLOW);

	/* reset colors */
	fputs(PROMPT_EXCLUDE_BEGIN, stdout);
	fputs("\033[0m", stdout);
	fputs(PROMPT_EXCLUDE_END, stdout);

	/* cleanup */
	FREE_NON_NULL(git_ahead);
	FREE_NON_NULL(git_behind);
	git_reference_free(git_local_branch);
	git_reference_free(git_remote_branch);
	git_repository_free(git_repo);

	return 0;
}
