#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define ERROR_IF_EQUAL(expression, error_code, error_signature, ...)  \
if ((expression) == error_code) {  \
    fprintf(stderr, error_signature __VA_ARGS__);  \
    exit(EXIT_FAILURE);  \
}

#define ERROR_IF_NOT_EQUAL(expression, error_code, error_signature, ...)  \
if ((expression) != error_code) {  \
    fprintf(stderr, error_signature __VA_ARGS__);  \
    exit(EXIT_FAILURE);  \
}

int create_connection(char *node, char *service);