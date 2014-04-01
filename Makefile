uname := $(shell uname)
ifeq ($(uname),Darwin)
	format := macho64
endif
ifeq ($(uname),Linux)
	format := elf64
endif

all: fhtw.o

%.o:%.asm
	nasm -f $(format) $<

test_%: test%.c fhtw.o
	gcc -std=c99 -o $@ $^

.PHONY: clean
clean:
	rm -f *.o test_*
