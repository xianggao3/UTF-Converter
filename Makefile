CC = gcc
SRC = src/utfconverter.c
INCLUDE = include/
BIN = utfconverter
CFLAGS = -g -Wall -Werror -pedantic -Wextra -DDEBUG
REQ = $(SRC) include/utfconverter.h
BINDIR = ./bin
OBJDIR = ./build


all: $(OBJDIR) $(BINDIR) $(BIN)

$(OBJDIR):
	mkdir $(OBJDIR)

$(BINDIR):
	mkdir $(BINDIR)

$(BIN): $(REQ)
	$(CC) $(CFLAGS) -I $(INCLUDE) $(SRC) -o $(BINDIR)/utf

.PHONY: clean

clean:
	rm -rf $(OBJDIR) $(BINDIR) *.o utf 

