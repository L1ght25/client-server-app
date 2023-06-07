#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../common_part/common.h"
#include "../tools/tools.h"

enum {
    MAX_SESSIONS_COUNT_LEN = 18,
    MAX_TIME_SECONDS_LEN = 18
};

void update_delta_sessions(int sig);

void print_server_stats(int sig);
