CFLAGS=-Wall -Wextra -Wswitch-enum  -std=c11 -pedantic
LIBS=

.PHONY: all
all: kodi

kodi: src/kodi.c src/kodi.h src/sv.h
	$(CC) $(CFLAGS) -o kodi src/kodi.c src/kodi.h src/sv.h $(LIBS)