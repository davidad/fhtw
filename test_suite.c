#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>
#include "greatest.h"
#include "fhtw.h"


TEST hash_of_empty_string_is_zero() {
  char* empty_string = "";
  ASSERT_EQ(0,fhtw_hash(empty_string,strlen(empty_string)));
  PASS();
}

TEST hash_purity() {
  char* strings [] = {
    "Lorem ipsum dolor sit amet, consecteuer adispicit elictus",
    "q", "",
    "Lorem ipsum",
    "eight88", "\x02 ",
    "Lorem ipsum dolor sit amet, consecteuer adispicit elictus",
    "\x02 ", "eight88",
    "Lorem ipsum",
    "q", ""};
  const size_t n = sizeof(strings)/sizeof(char*);
  int i;
  unsigned int *hashes = malloc(sizeof(unsigned int)*n);
  for(i=0;i<n;i++)
    hashes[i]=fhtw_hash(strings[i],strlen(strings[i])+1);
  ASSERT_EQ(hashes[0],hashes[6]);
  ASSERT_EQ(hashes[1],hashes[10]);
  ASSERT_EQ(hashes[2],hashes[11]);
  ASSERT_EQ(hashes[3],hashes[9]);
  ASSERT_EQ(hashes[4],hashes[8]);
  ASSERT_EQ(hashes[5],hashes[7]);
  free(hashes);
  PASS();
}

SUITE(hash_sanity) {
  RUN_TEST(hash_of_empty_string_is_zero);
  RUN_TEST(hash_purity);
}

static fhtw t;

static void allocate1000(void*arg) {
  printf("-- Allocating table of 1000 elements...\n");
  t = fhtw_new(1000);
}

TEST allocation_variables() {
  uint64_t* metadata = (uint64_t*)t;
  printf("     Metadata words: %llu %llu %llu\n",metadata[0],metadata[1],metadata[2]);
  ASSERT_EQm("First word of metadata, occupancy, should be 0 for a freshly allocated table",0,metadata[0]);
  ASSERT_EQm("Second word of metadata, capacity, should be 1000",1000,metadata[1]);
  ASSERT_EQm("Third word of metadata, hop-info word length, should be 9",9,metadata[2]);
  PASS();
}

static void free_t(void*arg) {
  printf("-- Freeing table...\n");
  fhtw_free(t);
}

SUITE(allocation) {
  SET_SETUP(allocate1000,NULL);
  SET_TEARDOWN(free_t,NULL);
  RUN_TEST(allocation_variables);
}

void dump_table() {
  uint64_t *meta = (uint64_t*)t, *key = meta+3, *val = key+(meta[1]), *hop = val+(meta[1]);
  printf("%016llx\n%016llx\n%016llx\n",meta[0],meta[1],meta[2]);
  for(int i=0;i<meta[1];i++) {
    printf("%016llx    %016llx    %016llx\n",key[i],val[i],hop[i]);
  }
}

#define TEST_small_set_and_get(NAME) \
TEST small_set_and_get_##NAME () { \
  fprintf(stdout, "\n-- test set_and_get_" #NAME " ... \n"); \
  t = fhtw_new(TABLE_SIZE); \
  char* pairs[][2] = PAIRS; \
  const size_t n = sizeof(pairs)/sizeof(char*[2]); \
  int i; \
  for(i=0;i<n;i++) fhtw_set(t,pairs[i][0],strlen(pairs[i][0])+1,pairs[i][1]); \
  for(i=0;i<n;i++) ASSERT_EQ(pairs[i][1],fhtw_get(t,pairs[i][0],strlen(pairs[i][0])+1)); \
  fprintf(stdout,"     passed"); \
  fhtw_free(t); \
  PASS(); }

#define TABLE_SIZE 10
#define PAIRS { \
    { "key 1", "value 1" }, \
    { "key 2", "value 2" }, \
    { "eight88", "value 3" }, \
    { "Lorem ipsum dolor sit amet", "value 4"} \
}
TEST_small_set_and_get(4_in_10)
#undef PAIRS
#define PAIRS { \
    { "", "value 0"}, \
    { "key 1", "value 1" }, \
    { "key 2", "value 2" }, \
    { "eight88", "value 3" }, \
    { "Lorem ipsum dolor sit amet", "value 4"} \
}
TEST_small_set_and_get(5_in_10)
#undef PAIRS
#define PAIRS { \
    { "Lorem ipsum", "dolor sit amet"}, \
    { "dolor sit amet", "consecteuer"}, \
    { "consecteteur", "adipiscing"}, \
    { "adipiscing", "sed do"}, \
    { "sed do", "eiusmod"}, \
    { "eiusmod", "tempor"}, \
    { "tempor", "incididunt ut labore"}, \
    { "incididunt ut labore", "et dolore magna"}, \
    { "", "aliquat"} \
}
TEST_small_set_and_get(9_in_10)
#undef PAIRS
#define PAIRS { \
    { "Lorem ipsum", "dolor sit amet"}, \
    { "dolor sit amet", "consecteuer"}, \
    { "consecteteur", "adipiscing"}, \
    { "adipiscing", "sed do"}, \
    { "sed do", "eiusmod"}, \
    { "eiusmod", "tempor"}, \
    { "tempor", "incididunt ut labore"}, \
    { "incididunt ut labore", "et dolore magna"}, \
    { "et dolore magna", "aliqua"}, \
    { "aliqua", "." } \
}
TEST_small_set_and_get(10_in_10)
#undef PAIRS
#undef TABLE_SIZE

#define BUF_SIZE 64
TEST big_set_and_get(uint64_t n_keys, uint64_t table_size, unsigned int seed) {
  t = fhtw_new(table_size);
  srand(seed);
  char** keys = malloc(n_keys*sizeof(char*));
  char** vals = malloc(n_keys*sizeof(char*));
  const int p = n_keys / 74;
  printf("\n-- testing %llu keys in a table of size %llu:\n",n_keys,table_size);
  int i;
  printf("     ");
  for(i=0;i<n_keys;i++) {
    if(!(i%p)) putchar('v'), fflush(stdout);
    keys[i]=malloc(BUF_SIZE);          vals[i]=malloc(BUF_SIZE);
    int key_rand = rand();             int val_rand = rand();
    snprintf(keys[i],BUF_SIZE,"key %d: %d",i,key_rand);
    snprintf(vals[i],BUF_SIZE,"val %d: %d",i,val_rand);
    fhtw_set(t,keys[i],BUF_SIZE,vals[i]);
  }
  printf("\n     ");
  for(i=0;i<n_keys;i++) {
    if(!(i%p)) putchar('^'), fflush(stdout);
    ASSERT_EQ(vals[i],fhtw_get(t,keys[i],BUF_SIZE));
  }
  for(i=0;i<n_keys;i++) {
    free(keys[i]);
    free(vals[i]);
  }
  fprintf(stdout,"\n     passed");
  free(keys); free(vals);
  fhtw_free(t);
  PASS();
}

SUITE(set_and_get) {
  RUN_TEST(small_set_and_get_4_in_10);
  RUN_TEST(small_set_and_get_5_in_10);
  RUN_TEST(small_set_and_get_9_in_10);
  RUN_TEST(small_set_and_get_10_in_10);
  for(int k = 400;k<1000;k+=260) RUN_TESTp(big_set_and_get,k*1e3,1e6,0x18a3);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(hash_sanity);
  RUN_SUITE(allocation);
  RUN_SUITE(set_and_get);
  GREATEST_MAIN_END();
}
