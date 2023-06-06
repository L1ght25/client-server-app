#include "helpers.h"

void send_signal_to_server(int signal, int fd) {
    write(fd, &signal, sizeof(signal));
}

int parse_address(char *address) {
    int border = 0;
    while (address[border] != ':') {
        if (address[border] == '\0') {
            return -1;
        }
        ++border;
    }
    address[border] = '\0';
    return border;
}

int send_proc_type_to_server(int proc_type, int fd) {
    int n_bytes = write(fd, &proc_type, sizeof(proc_type));
    return n_bytes;
}

void send_args_to_server(int fd, int argc, char *argv[]) {
    int count_of_args = argc - 3;
    int n_bytes = write(fd, &count_of_args, sizeof(count_of_args));

    // args and their sizes
    int size_of_arg = 0;
    for (size_t i = 3; i < argc; ++i) {
        size_of_arg = strlen(argv[i]);
        send_data(fd, size_of_arg, argv[i]);
    }
}
