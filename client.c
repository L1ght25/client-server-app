#include "client.h"
#include <unistd.h>

enum {
    READ = 0,
    WRITE = 1,
    BUFFER_SIZE = 1000
};

static int server_socket[2];

int create_connection(char* node, char *service) {
    struct addrinfo *res = NULL;
    int gai_err;
    struct addrinfo hint = {
        .ai_family = AF_UNSPEC,     // можно и AF_INET, и AF_INET6
        .ai_socktype = SOCK_STREAM, // но мы хотим поток (соединение)
    };
    if ((gai_err = getaddrinfo(node, service, &hint, &res))) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(gai_err));
        return -1;
    }
    for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        if ((server_socket[0] = socket(ai->ai_family, ai->ai_socktype, 0)) == -1 ||
            (server_socket[1] = socket(ai->ai_family, ai->ai_socktype, 0)) == -1) {
            perror("socket");
            continue;
        }
        if (connect(server_socket[0], ai->ai_addr, ai->ai_addrlen) < 0 ||
            connect(server_socket[1], ai->ai_addr, ai->ai_addrlen) < 0) {
            perror("connect");
            close(server_socket[0]);
            close(server_socket[1]);
            continue;
        }
        break;
    }
    freeaddrinfo(res);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Invalid size of arguments. Expected > 2, but got %d\n", argc);
    }

    int border = 0;
    while (argv[1][border] != ':') {
        ++border;
    }
    argv[1][border] = '\0';

    ERROR_IF_EQUAL(create_connection(argv[1], argv[1] + border + 1), -1, "Error connecting to server...\n");

    int n_bytes;
    char buffer[BUFFER_SIZE];

    // args part
    int count_of_args = argc - 3;
    // snprintf(buffer, BUFFER_SIZE, "%d", argc - 3);

    // count of args
    n_bytes = write(server_socket[WRITE],&count_of_args, sizeof(count_of_args));

    // args and their sizes
    int size_of_arg = 0;
    for (size_t i = 3; i < argc; ++i) {
        size_of_arg = strlen(argv[i]);
        n_bytes = write(server_socket[WRITE], &size_of_arg, sizeof(size_of_arg));
        n_bytes = write(server_socket[WRITE], argv[i], size_of_arg);
    }

    //stdin
    while ((n_bytes = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
        write(server_socket[WRITE], buffer, n_bytes);
    }

    memset(buffer, 0, BUFFER_SIZE);

    while ((n_bytes = recv(server_socket[READ], buffer, BUFFER_SIZE, 0)) > 0) {
        printf("%s", buffer);
        memset(buffer, 0, BUFFER_SIZE);
    }
    // printf("\n");

    close(server_socket[READ]);
    close(server_socket[WRITE]);
}