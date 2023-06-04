#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>

enum { BUFFER_SIZE = 1000 };

int daemonize() {
    pid_t pid = fork();

    if (pid != 0) {
        exit(0);
    }

    if (setsid() < 0) {
        return -1;
    }

    return 0;
}

int create_listener(char* service) {
    struct addrinfo *res = NULL;
    int gai_err;
    struct addrinfo hint = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE,
    };
    if ((gai_err = getaddrinfo(NULL, service, &hint, &res))) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(gai_err));
        return -1;
    }
    int sock = -1;
    for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype, 0);
        if (sock < 0) {
            perror("socket");
            continue;
        }
        if (bind(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
            perror("bind");
            close(sock);
            sock = -1;
            continue;
        }
        if (listen(sock, SOMAXCONN) < 0) {
            perror("listen");
            close(sock);
            sock = -1;
            continue;
        }
        break;
    }
    freeaddrinfo(res);
    return sock;
}


int main(int argc, char *argv[]) {
    struct sigaction child_act = {.sa_handler = SIG_IGN};
    sigaction(SIGCHLD, &child_act, NULL);

    int sock = create_listener(argv[1]);

    struct sockaddr_in client;
    socklen_t client_addrlen = sizeof(client);

    if(daemonize() < 0) {
        exit(EXIT_FAILURE);
    }

    while (1) {
        int to_client = accept(sock, (struct sockaddr *)&client, &client_addrlen);
        int from_client = accept(sock, (struct sockaddr *)&client, &client_addrlen);
        pid_t pid = fork();
        if (!pid) {
            char buffer[BUFFER_SIZE];

            // count of args part
            int cnt_of_args;
            int n_bytes = read(from_client, &cnt_of_args, sizeof(cnt_of_args));
            char **argv = calloc(cnt_of_args, sizeof(*argv));

            int size_of_arg;
            for (size_t i = 0; i < cnt_of_args; ++i) {
                // read args in format [size of arg, arg]
                n_bytes = read(from_client, &size_of_arg, sizeof(size_of_arg));
                argv[i] = calloc(size_of_arg, sizeof(*argv[i]));
                n_bytes = read(from_client, argv[i], size_of_arg);
            }

            dup2(from_client, STDIN_FILENO);
            close(from_client);
            dup2(to_client, STDOUT_FILENO);
            close(to_client);

            execvp(argv[0], argv);
        } else {
            waitpid(pid, NULL, 0);
            close(from_client);
            close(to_client);
        }
    }
}
