#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "fhtw.h"
#define SIZE 1e6
#define BUF_SIZE 64

int main() {
  setbuf(stdout, NULL);
  printf("Test 5, testing ...");

  fhtw foo = fhtw_new(SIZE);
  srand(1337);
  char** keys = malloc(SIZE * sizeof(char*));
  char** vals = malloc(SIZE * sizeof(char*));

  for (int i = 0; i < SIZE; i++) {
    if (!(i % 10000)) printf(".");

    keys[i] = malloc(BUF_SIZE);
    vals[i] = malloc(BUF_SIZE);

    int key_rand = rand();
    int val_rand = rand();

    snprintf(keys[i], BUF_SIZE, "key %d: %d", i, key_rand);
    snprintf(vals[i], BUF_SIZE, "val %d: %d", i, val_rand);

    fhtw_set(foo, keys[i], BUF_SIZE, vals[i]);
  }

  printf("\n");

  for(int i = 0; i < SIZE; i++) {
    if (!(i % 10000)) printf("^");
    char* value = fhtw_get(foo, keys[i], BUF_SIZE);
    assert(value == vals[i]);
    free(keys[i]);
    free(vals[i]);
  }

  printf("\n");

  free (vals);
  free (keys);
  fhtw_free(foo);
}
