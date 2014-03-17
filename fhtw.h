typedef void* fhtw ;

fhtw fhtw_new(int size);
void fhtw_free(fhtw);
int fhtw_set(fhtw, void* key, void* value);
void* fhtw_get(fhtw, void* key);



