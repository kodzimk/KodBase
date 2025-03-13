#!/bin/sh

set -xe

CC=${CC:=/usr/bin/cc}
CFLAGS="-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -Wconversion -pedantic -fno-strict-aliasing -ggdb -std=c11"
LIBS=

$CC $CFLAGS -o kodi ./src/kodi.c $LIBS

for example in `find examples/ -name \*.kb | sed "s/\.kodi//"`; do
    ./kodi -g "$example.kb" 
done