CC = gcc

CFLAGS = -D_XOPEN_SOURCE=600 \
         -D_DEFAULT_SOURCE \
         -g -O0 -I. \
         -pedantic \
         -Wall \
         -Wextra \
         -Wmissing-prototypes \
         -Wno-sign-compare \
         -Wno-unused-parameter
