#include "util.h"

enum {
    MAX_METHOD_LEN = 6,
    MAX_URL_LEN = 255
};

int create_connection(char* node, char *service) {
    int server_socket;
    struct addrinfo *res = NULL;
    int gai_err;
    struct addrinfo hint = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
    };
    if ((gai_err = getaddrinfo(node, service, &hint, &res))) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(gai_err));
        return -1;
    }
    for (struct addrinfo *ai = res; ai; ai = ai->ai_next) {
        if ((server_socket = socket(ai->ai_family, ai->ai_socktype, 0)) == -1) {
            perror("socket");
            continue;
        }
        if (connect(server_socket, ai->ai_addr, ai->ai_addrlen) < 0) {
            perror("connect");
            close(server_socket);
            continue;
        }
        break;
    }
    freeaddrinfo(res);
    return server_socket;
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

bool is_http_request(char *buffer) {
    char method[MAX_METHOD_LEN];
    char url[MAX_URL_LEN];
    sscanf(buffer, "%s %s", method, url);

    if (!strcmp(method, "GET")) {
        return true;
    }
    return false;
}

bool parse_get_http_to_command(char *buffer, char *command) {
    sscanf(buffer, "GET /%s", command);
    return true;
}

void send_data(int server, int n_bytes, char *data) {
    write(server, &n_bytes, sizeof(n_bytes));
    write(server, data, n_bytes);
}
