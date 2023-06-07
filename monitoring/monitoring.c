#include "monitoring.h"

static const char *ALL_SESSIONS_CONTEXT = "../server_dir/all_sessions_context.txt";
static const char *COMPLETED_SESSIONS_CONTEXT = "../server_dir/completed_sessions_context.txt";
static int all_sessions = 0;
static int all_completed_sessions = 0;
static int delta_all_sessions = 0;
static int delta_completed_sessions = 0;
static int t_sec = 0;
static time_t server_time_start = 0;

void update_delta_sessions(int sig) {
    static int prev_sessions = 0;
    static int prev_completed_sessions = 0;
    char buffer[MAX_SESSIONS_COUNT_LEN];
    atomic_read_file(ALL_SESSIONS_CONTEXT, buffer, MAX_SESSIONS_COUNT_LEN);
    int all_sessions_count = atoi(buffer);
    delta_all_sessions  = all_sessions_count - prev_sessions;
    all_sessions = all_sessions_count;
    prev_sessions = all_sessions_count;

    memset(buffer, 0, MAX_SESSIONS_COUNT_LEN);
    atomic_read_file(COMPLETED_SESSIONS_CONTEXT, buffer, MAX_SESSIONS_COUNT_LEN);
    int completed_sesisons_count = atoi(buffer);
    delta_completed_sessions  = completed_sesisons_count - prev_completed_sessions;
    all_completed_sessions = completed_sesisons_count;
    prev_completed_sessions = completed_sesisons_count;
}

void print_server_stats(int sig) {
    static bool was_updated = false;
    time_t current_time = time(NULL);
    printf("\nSeconds from server begin: %ld\n", current_time - server_time_start);
    if (current_time - server_time_start < t_sec || !was_updated) {
        was_updated = true;
        update_delta_sessions(0);
    }
    printf("All sessions: %d\n", all_sessions);
    printf("All completed sessions: %d\n", all_completed_sessions);
    printf("Active sessions: %d\n", all_sessions - all_completed_sessions);
    printf("For last %d seconds:\n", t_sec);
    printf("\tNew sessions: %d\n", delta_all_sessions);
    printf("\tCompleted sessions: %d\n", delta_completed_sessions);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        perror("parameter T is not set");
        exit(EXIT_FAILURE);
    }
    signal(SIGINT, print_server_stats);

    t_sec = atoi(argv[1]);

    struct itimerval it_val;
    signal(SIGALRM, update_delta_sessions);
    it_val.it_value.tv_sec = t_sec;
    it_val.it_value.tv_usec = 0;
    it_val.it_interval = it_val.it_value;
    setitimer(ITIMER_REAL, &it_val, NULL);

    char buffer[MAX_TIME_SECONDS_LEN];
    if (atomic_read_file("../server_dir/time_start.txt", buffer, MAX_TIME_SECONDS_LEN)) {
        perror("Server was not started yet\n");
        exit(EXIT_FAILURE);
    }
    server_time_start = strtol(buffer, NULL, 10);

    while (1) {
        pause();
    }
}
