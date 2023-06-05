#include "server.h"

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
        .ai_flags = AI_PASSIVE
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
        int tmp = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) < 0) {
            close(sock);
            sock = -1;
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

    if(daemonize() < 0) {
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    while (1) {
        int client = accept(sock, (struct sockaddr *)&client_addr, &client_addrlen);
        // printf("%s:%d\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));

        pid_t pid = fork();
        if (!pid) {
            char buffer[BUFFER_SIZE];

            // count of args part
            int cnt_of_args;
            int n_bytes = read(client, &cnt_of_args, sizeof(cnt_of_args));
            char **argv = calloc(cnt_of_args, sizeof(*argv));

            int size_of_arg;
            for (size_t i = 0; i < cnt_of_args; ++i) {
                // read args in format [size of arg, arg]
                n_bytes = read(client, &size_of_arg, sizeof(size_of_arg));
                argv[i] = calloc(size_of_arg, sizeof(*argv[i]));
                n_bytes = recv(client, argv[i], size_of_arg, MSG_WAITALL);
            }

            int pipefd[2];

            pipe(pipefd);

            pid_t input_pid = fork();

            if (input_pid > 0) {
                close(pipefd[0]);
                while (1) {
                    memset(buffer, 0, BUFFER_SIZE);
                    int cnt_of_bytes;
                    if ((n_bytes = recv(client, &cnt_of_bytes, sizeof(cnt_of_bytes), 0)) > 0) {
                        if (cnt_of_bytes == 0) {
                            break;
                        }

                        n_bytes = recv(client, buffer, cnt_of_bytes, MSG_WAITALL);
                        write(pipefd[1], buffer, n_bytes);
                    } else {
                        break;
                    }
                }
                close(client);
                close(sock);
                close(pipefd[1]);
                exit(0);
            } else {
                int to_client = dup(client);

                dup2(pipefd[0], STDIN_FILENO);
                dup2(to_client, STDOUT_FILENO);
                close(client);
                close(to_client);
                close(sock);
                close(pipefd[1]);

                execvp(argv[0], argv);
            }
        } else {
            close(client);
        }
    }
}
