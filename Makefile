uname := $(shell uname)
ifeq ($(uname),Darwin)
	format := macho64
endif
ifeq ($(uname),Linux)
	format := elf64
endif

.PHONY: all clean test
all: fhtw.o

%.o:%.asm
	nasm -f $(format) $<

test: test_suite
	./test_suite

test_suite: test_suite.c fhtw.o
	gcc -std=c99 -o $@ $^

clean:
	rm -f *.o test_*
