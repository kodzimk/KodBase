CFLAGS = 
LIBS = 

db: src/main.c
	$(CC) $(CFLAGS) -o db src/main.c $(LIBS)