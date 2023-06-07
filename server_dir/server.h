#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <limits.h>
#include <pthread.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include "../common_part/common.h"
#include "../net_util/util.h"
#include "../tools/tools.h"

enum {
    SIZE_OF_CRIU_ARGS = 8,
    MAX_PID_LENGTH = 10,
    MAX_COMMAND_LEN = 255,
    MAX_SESSIONS_COUNT_LEN = 18,
    UPDATE_INTERVAL_SEC = 5,
    MAX_TIME_SECONDS_LEN = 18,
    DEFAULT_CAPACITY_PIDS = 10
};

typedef struct {
    int client;
    int pipefd;
    int sock;
    int proc_pid;
} FinishContext;

void free_server(int sig);

int daemonize();

void complete_session_handler(int sig);

void *handle_finish_proc(void *arg);

void handle_active_sessions(int client);

void handle_spawn(int sock, int client);

int increment_saved_sessions_number();

int get_saved_sessions_number();

void handle_saving_session(int proc_pid, int client);

void handle_kill_session(int proc_pid, int client);

bool check_and_handle_http(int client);

void handle_http_request(int client, char *command);

int receive_args_from_client(int client, char ***argv);

void update_start_time();
