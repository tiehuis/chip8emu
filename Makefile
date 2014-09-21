CC     ?= gcc
CFLAGS += -Wall -Wextra
LIBS    = -lcurses
PROG    = chip8

all: chip8

chip8: src/chip8.c src/chip8.h
	$(CC) -o $(PROG) $(CFLAGS) src/chip8.c $(LIBS)

clean:
	@rm -f $(PROG)

