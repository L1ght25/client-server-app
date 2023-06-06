#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

int atomic_write_file(const char* filename, const char* data, size_t size);

int atomic_read_file(const char* filename, char* data, size_t size);
