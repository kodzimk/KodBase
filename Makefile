CFLAGS=-Wall -Wextra -Wswitch-enum -Wconversion -std=c11 -pedantic 
LIBS=

.PHONY: all
all: db kodi

db: src/main.c src/kodbase.h
	$(CC) $(CFLAGS) -o db src/main.c src/kodi.h $(LIBS)

kodi: src/kodi.c src/kodi.h
	$(CC) $(CFLAGS) -o kodi src/kodi.c src/kodi.h $(LIBS)