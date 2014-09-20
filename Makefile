CC     ?= gcc
CFLAGS += -Wall -Wextra
LIBS    = -lcurses
PROG    = chip8

all: chip8

chip8: src/emulator.c
	$(CC) -o $(PROG) $(CFLAGS) src/emulator.c $(LIBS)

clean:
	@rm -f $(PROG)

