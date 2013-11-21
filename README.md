#slcp

slcp is a simple shell prompt written in C. If you are annoyed by different Prompt configurations in various shells or would like to minimize the amount of processes required to get all information you want to have in your prompt, feel free to fork and modify slcp to your needs.


#Features

* user- and hostname
* ptsname
* statuscode of the last executed command
* pwd with automatic truncation
* automatic fitting in your terminal width
* git
  * the part of the pwd that is contained in the repo is highlighted
  * current local and remote branch
  * state (rebase, merge, apply-mailbox, …)
  * ahead/behind numbers for tracking branches

![Screenshots](http://schachmat.github.io/slcp/animated.gif)


#Dependencies

Currently slcp just requires libgit2. If you do not need git repository highlighting or any other git features in your prompt, it can easily be removed from the source.


#Usage

slcp expects the terminal width as first and the exit status of the last command as second argument. If you're using mksh, you can put the following in you .mkshrc:

    PS1='$(slcp $COLUMNS $?)'

#Contribution

If you have an awesome idea, that should be implemented, you can do it yourself and open a pull request or send a message and tell me, why your idea is useful und should be included in slcp. You can also fork your own version (the code is relatively easy to understand) and tell me about your changes.


#License

"THE BEER-WARE LICENSE" (Revision 42):
<teichm@in.tum.de> wrote this file. As long as you retain this notice you
can do whatever you want with this stuff. If we meet some day, and you think
this stuff is worth it, you can buy me a beer in return Markus Teich
