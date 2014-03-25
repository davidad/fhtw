#include <stdio.h>
#include <string.h>
#include "fhtw.h"

int main() {
  printf("\nTest 3, testing set and get...\n\n");
  fhtw foo = fhtw_new(10);
  char* keys[] = { "key1", "key2", "a", "boooooooo" };
  char* vals[] = { "val1", "val2", "nothing", "a" };
  char* result;

  for (int i = 0; i < sizeof(keys)/sizeof(char*); i++) {
    fhtw_set(foo, keys[i], strlen(keys[i]), vals[i]);
  }

  for (int i = 0; i < sizeof(keys)/sizeof(char*); i++) {
    result = (char*)fhtw_get(foo, keys[i], strlen(keys[i]));
    printf("pair %d is %s, %s\n", i, keys[i], result);
  }

  fhtw_free(foo);
}
