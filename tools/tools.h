#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct {
    int *data;
    size_t capacity;
    size_t size;
} DynArray;

#define INIT_ARRAY {.data = calloc(5, sizeof(int)), .capacity = 5, .size = 0};

int atomic_write_file(const char* filename, const char* data, size_t size);

int atomic_read_file(const char* filename, char* data, size_t size);

int add_elem_to_array(DynArray *array, int elem);

int init_array(DynArray *array, int cap);