#include "client.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile int server_socket;

void signal_handler(int sig) {
    // close stdin if we have
    close(STDIN_FILENO);

    char *message;
    int signal_name;

    if (sig == SIGINT) {
        message = "\nSEND SAVE SIGNAL TO SERVER...\n";
        signal_name = SAVE_SESSION;
    } else {
        message = "\nSEND KILL SIGNAL TO SERVER...\n";
        signal_name = KILL_SESSION;
    }
    fprintf(stderr, "%s", message);

    // send signal to server
    send_signal_to_server(signal_name, server_socket);
}

void *reader(void *arg) {
    int socket_read = server_socket;
    int n_bytes;
    char buffer[BUFFER_SIZE] = {0};

    while ((n_bytes = recv(socket_read, buffer, BUFFER_SIZE, 0)) > 0) {
        printf("%s", buffer);
        memset(buffer, 0, n_bytes);
    }
    if (n_bytes == 0) {  // session is over
        close(socket_read);
        exit(EXIT_SUCCESS);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Invalid size of arguments. Expected > 2, but got %d\n", argc);
    }

    // handlers for signals
    PREPARE_SIGNALS(signal_handler);

    // parse adress of server
    int border;
    ERROR_IF_EQUAL((border = parse_address(argv[1])), -1, "Incorrenct address\n");

    // connect to server
    ERROR_IF_EQUAL((server_socket = create_connection(argv[1], argv[1] + border + 1)), -1, "Error connecting to server...\n");

    int n_bytes;
    char buffer[BUFFER_SIZE];

    if (!strcmp(argv[2], "spawn")) {
        // process type part
        ERROR_IF_EQUAL((send_proc_type_to_server(SPAWN, server_socket)), -1, "Error sending type to server\n");

        // args part
        send_args_to_server(server_socket, argc, argv);

        pthread_t thread;

        // read from server
        pthread_create(&thread, NULL, reader, NULL);

        //stdin
        while ((n_bytes = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
            write(server_socket, &n_bytes, sizeof(n_bytes));
            write(server_socket, buffer, n_bytes);
        }
        send_signal_to_server(END_OF_INPUT, server_socket);

        pthread_join(thread, NULL);
        close(server_socket);
    } else if (!strcmp(argv[2], "list")) {
        ERROR_IF_EQUAL((send_proc_type_to_server(LIST, server_socket)), -1, "Error sending type to server\n");
        // TODO
    } else if (!strcmp(argv[2], "attach")) {
        ERROR_IF_EQUAL((send_proc_type_to_server(ATTACH, server_socket)), -1, "Error sending type to server\n");
        // TODO
    }
}
