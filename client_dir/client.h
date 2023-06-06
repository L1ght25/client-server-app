#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdbool.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "../common_part/common.h"
#include "helpers.h"

#define ERROR_IF_EQUAL(expression, error_code, error_signature, ...)  \
if ((expression) == error_code) {  \
    printf(error_signature __VA_ARGS__);  \
    exit(EXIT_FAILURE);  \
}

#define ERROR_IF_NOT_EQUAL(expression, error_code, error_signature, ...)  \
if ((expression) != error_code) {  \
    printf(error_signature __VA_ARGS__);  \
    exit(EXIT_FAILURE);  \
}

void *reader(void *arg);
