#include "server.h"

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

void *handle_finishing_proc(void *arg) {
    FinishContext context = *(FinishContext *)arg;
    waitpid(context.proc_pid, NULL, 0);
    close(context.client);
    close(context.pipefd);
    close(context.sock);
    exit(0);
}

int increment_sessions_number() {
    char buffer[MAX_SESSIONS_COUNT_LEN] = {0};
    int count_sessions = 0;
    if (!atomic_read_file("./sessions_context.txt", buffer, MAX_SESSIONS_COUNT_LEN)) {
        count_sessions = atoi(buffer);
    }
    ++count_sessions;
    snprintf(buffer, MAX_SESSIONS_COUNT_LEN, "%d", count_sessions);
    atomic_write_file("./sessions_context.txt", buffer, MAX_SESSIONS_COUNT_LEN);
    return count_sessions;
}

int get_sessions_number() {
    char buffer[MAX_SESSIONS_COUNT_LEN] = {0};
    int count_sessions = 0;
    if (!atomic_read_file("./sessions_context.txt", buffer, MAX_SESSIONS_COUNT_LEN)) {
        count_sessions = atoi(buffer);
    }
    return count_sessions;
}

void handle_saving_session(int proc_pid, int client) {
    pid_t pid = fork();
    if (!pid) {
        char *args[SIZE_OF_CRIU_ARGS];
        args[0] = "criu";
        args[1] = "dump";
        args[2] = "-t";
        char *pid_str = calloc(MAX_PID_LENGTH, sizeof(*pid_str));
        snprintf(pid_str, MAX_PID_LENGTH, "%d", proc_pid);
        args[3] = pid_str;

        args[4] = "-D";
        char *path = calloc(PATH_MAX, sizeof(*path));
        snprintf(path, PATH_MAX, "./session_%d", get_sessions_number());
        char *real_path = calloc(PATH_MAX, sizeof(*path));
        realpath(path, real_path);
        free(path);

        int res = mkdir(real_path, 0777);

        args[5] = real_path;
        args[6] = "--tcp-established";
        args[7] = "--shell-job";
        execvp(args[0], args);
        exit(EXIT_FAILURE);
    } else {
        int dump_status;
        waitpid(pid, &dump_status, 0);

        int count_sessions = increment_sessions_number();
        char buffer[BUFFER_SIZE];

        int n_bytes;
        if (WIFSIGNALED(dump_status) || WEXITSTATUS(dump_status)) {
            n_bytes = snprintf(buffer, BUFFER_SIZE,
            "OOPS... Failure. This feature is unstable. Let's assume we've dumped process successfully :)\nsession id: %d\n", count_sessions - 1);
        } else {
            n_bytes = snprintf(buffer, BUFFER_SIZE, "\nSaved session successfuly, session id: %d\n", count_sessions - 1);
        }
        kill(proc_pid, SIGTERM);
        write(client, buffer, n_bytes);
    }
}

void handle_kill_session(int proc_pid, int client) {
    kill(proc_pid, SIGKILL);
    char buffer[BUFFER_SIZE];
    int n_bytes = snprintf(buffer, BUFFER_SIZE, "\nKilled session successfuly\n");
    write(client, buffer, n_bytes);
}

int receive_args_from_client(int client, char ***argv) {
    int cnt_of_args;
    int n_bytes = read(client, &cnt_of_args, sizeof(cnt_of_args));
    *argv = calloc(cnt_of_args, sizeof(**argv));

    int size_of_arg;
    for (size_t i = 0; i < cnt_of_args; ++i) {
        // read args in format [size of arg, arg]
        n_bytes = recv(client, &size_of_arg, sizeof(size_of_arg), MSG_WAITALL);
        if (n_bytes <= 0) {
            free(*argv);
            return -1;
        }
        (*argv)[i] = calloc(size_of_arg, sizeof(**argv[i]));
        n_bytes = recv(client, *argv[i], size_of_arg, MSG_WAITALL);
        if (n_bytes <= 0) {
            free(*argv);
            return -1;
        }
    }
    return 0;
}

void proxy_event_loop(int client, int pipe_to_proc, int proc_pid) {
    char buffer[BUFFER_SIZE];
    int n_bytes;
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int cnt_of_bytes;
        if ((n_bytes = recv(client, &cnt_of_bytes, sizeof(cnt_of_bytes), 0)) > 0) {
            if (cnt_of_bytes == 0) {
                close(pipe_to_proc);
            }

            if (cnt_of_bytes == SAVE_SESSION) {
                handle_saving_session(proc_pid, client);
                break;
            } else if (cnt_of_bytes == KILL_SESSION) {
                handle_kill_session(proc_pid, client);
                break;
            }

            n_bytes = recv(client, buffer, cnt_of_bytes, MSG_WAITALL);
            write(pipe_to_proc, buffer, n_bytes);
        } else {
            break;
        }
    }
}

void handle_spawn(int sock, int client) {

    // count of args part
    char **argv;
    if (receive_args_from_client(client, &argv)) {
        exit(EXIT_FAILURE);
    }

    int pipefd[2];

    pipe(pipefd);

    pid_t proc_pid = fork();

    if (proc_pid > 0) {
        close(pipefd[0]);

        pthread_t checking_finish;
        FinishContext check_context = {.client = client, .pipefd = pipefd[1], .proc_pid = proc_pid, .sock = sock};
        pthread_create(&checking_finish, NULL, handle_finishing_proc, &check_context);

        proxy_event_loop(client, pipefd[1], proc_pid);

        close(client);
        close(sock);
        close(pipefd[1]);
        pthread_kill(checking_finish, SIGKILL);
        pthread_join(checking_finish, NULL);
        exit(0);
    } else {
        dup2(pipefd[0], STDIN_FILENO);
        dup2(client, STDOUT_FILENO);
        close(client);
        close(sock);
        close(pipefd[1]);

        execvp(argv[0], argv);
        exit(EXIT_FAILURE);
    }
}

bool check_and_handle_http(int client) {
    char buffer[BUFFER_SIZE];
    int n_bytes = recv(client, buffer, BUFFER_SIZE, MSG_PEEK);
    if (!is_http_request(buffer)) {
        return false;
    }
    char command[MAX_COMMAND_LEN];
    parse_get_http_to_command(buffer, command);
    handle_http_request(client, command);
    return true;
}

void handle_http_request(int client, char *command) {
    char header[BUFFER_SIZE] = {0};
    char buffer[BUFFER_SIZE] = {0};
    int n_bytes;
    int header_bytes;

    if (!strcmp(command, "list")) {
        n_bytes = snprintf(buffer, BUFFER_SIZE, "There are %d saved sessions now\n", get_sessions_number());
        header_bytes = snprintf(header, BUFFER_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n", n_bytes);
    } else {
        n_bytes = snprintf(buffer, BUFFER_SIZE, "Unknown command\n");
        header_bytes = snprintf(header, BUFFER_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n", n_bytes);
    }
    send(client, header, header_bytes, 0);
    send(client, buffer, n_bytes, 0);
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

        pid_t pid = fork();
        if (!pid) {

            // check if we have http request
            if (check_and_handle_http(client)) {
                sleep(1);
                exit(EXIT_SUCCESS);
            }

            // type part
            int proc_type;
            int n_bytes = read(client, &proc_type, sizeof(proc_type));

            switch(proc_type) {
                case SPAWN:
                    handle_spawn(sock, client);
                    break;
                // case LIST:
                    // handle_list();
                // case ATTACH:
                    // handle_attach();
                // case KILL:
                    // handle_kill();
                
                // default:
                //     break; 
            }
            exit(EXIT_SUCCESS);
        } else {
            close(client);
        }
    }
}
