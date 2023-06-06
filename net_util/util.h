#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

int create_connection(char *node, char *service);

int create_listener(char *service);

bool is_http_request(char *buffer);

bool parse_get_http_to_command(char *buffer, char *command);

void send_data(int server, int n_bytes, char *data);