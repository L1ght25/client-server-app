#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "../net_util/util.h"

#define PREPARE_SIGNALS(signal_handler)  \
    struct sigaction act;  \
    sigemptyset(&act.sa_mask);  \
    sigaddset(&act.sa_mask, SIGINT);  \
    sigaddset(&act.sa_mask, SIGTERM);  \
    act.sa_handler = signal_handler;  \
    sigaction(SIGINT, &act, NULL);  \
    sigaction(SIGTERM, &act, NULL);


void send_signal_to_server(int signal, int fd);

int parse_address(char *address);

int send_proc_type_to_server(int proc_type, int fd);

void send_args_to_server(int fd, int argc, char *argv[]);
