.PHONY: clean

all: fhtw.a

fhtw.a: fhtw.o
	ar rvs $@ fhtw.o

%.o:%.asm
	nasm -felf64 $<

test_%: test%.c fhtw.a
	gcc -std=c99 -o $@ $^

clean:
	rm -f *.o test_* fhtw.a

