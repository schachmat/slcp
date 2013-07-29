# slcp - suckless C prompt
# See LICENSE file for copyright and license details.
# Customize below to fit your system

# paths
PREFIX = /usr/local

# includes and libs
INCS = -I. -I/usr/include -I/usr/local/include
LIBS = -L/usr/lib -L/usr/local/lib -lgit2

# debug flags
CFLAGS = -g -std=c99 -D_POSIX_C_SOURCE=200112L -pedantic -Wall -O0 ${INCS}
LDFLAGS = -g ${LIBS}

# release flags
#CFLAGS = -std=c99 -pedantic -Wall -O3 ${INCS}
#LDFLAGS = -s ${LIBS}

# compiler and linker
CC = cc
