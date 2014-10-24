CC	     = gcc
CFLAGS   = -march=native -O2

LD	     = ld
LIBS	 = 
INCLUDES = -lcurses
LFLAGS   = $(INCLUDES) $(LIBS)

TARGET	 = chip8
DIRSRC   = src
DIRBUILD = build
SRCS	 = $(wildcard $(DIRSRC)/*.c)
OBJS	 = $(addprefix $(DIRBUILD)/, $(notdir $(SRCS:.c=.o)))

all: $(DIRBUILD) $(TARGET)

$(DIRBUILD):
	@mkdir -p $(DIRBUILD)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LFLAGS)

$(DIRBUILD)/%.o: $(DIRSRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS)
	@rm -f $(TARGET)
