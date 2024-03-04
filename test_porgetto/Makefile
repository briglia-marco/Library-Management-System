#flags
CC := gcc
CFLAGS := -Wall -pedantic -g

SRC := $(wildcard *.c) 
OBJ := $(SRC:.c=.o)

#phony
.PHONY: all clean run

#all
all: exe

#exe
exe: $(OBJ) 
	@$(CC) $(CFLAGS) $^ -o $@

#clean
clean:
	rm -f *.o exe
	@echo "Cleaned"

#run
run: exe
	./exe

pipe: pipe.c
	$(CC) $(CFLAGS) $^ -o $@
	./pipe
