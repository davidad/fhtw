#include <stdio.h>
#include <stdint.h>
#include "fhtw.h"

int main(int argc, int* argv[]) {
  printf("\nTest 1, testing new and delete... \n\n");
  uint64_t* foo = (uint64_t*)fhtw_new(100);
  printf("%d %d %d\n", foo[0], foo[1], foo[2]);
  fhtw_free(foo);
}
