#include <stdio.h>
#include <string.h>
#include "fhtw.h"

int main(int argc, char* argv[]) {
  char* strings[] = {
    "bing bang boom we're going to the moon", "hello world", 
    "q", "q",
    "seven!!", "seven!!",
    "a", "a",
    "davidad!", "knock knock who's there boo boo hoo don't cry it's only me", 
    "", "hello world", 
    "", "davidad!"
  };

  for (int i = 0; i < sizeof(strings) / sizeof(char*); i++) {
    printf("'%s' hashes to ", strings[i]);
    printf("%x\n", fhtw_hash(strings[i], strlen(strings[i])));
  }

  char* str = "bing bang bo.m we're going to the moon";
  for (int i = 0; i < 9; i++) {
    printf("'%s' hashes to ", str);
    printf("%x\n", fhtw_hash(str, 7 + i));
  }
  
}
