//
// Created by Remi Robert on 28/04/16.
//

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "server.h"

#define CLOSE_FD(fd) do {   \
    if (-1 != (fd)) {   \
        close(fd);  \
        (fd)    = -1;   \
    }   \
} while (0)

void route_init(struct server_routes *routes);
void check_size_capacity(struct server_routes *routes);
void add_route(struct server_routes *routes, struct server_route route);
void free_routes(struct server_routes *routes);

struct server_config serv_config;

int initSocket() {
    struct sockaddr_in addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        exit(1);
    } else {
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(serv_config.port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // Binding Address and Port
    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in))) {
        exit(1);
    } else {
    }

    // Start Listening
    listen(sockfd, 2048);
    return sockfd;
}


void init_server(int port) {
    serv_config.port = port;
    serv_config.fd_server = initSocket();
    route_init(&serv_config.routes);

    printf("init size capacity : %d\n", serv_config.routes.size);
}

void server_add_route(char *method, char *request_URI, struct response (* route_func)(struct request)) {
    struct server_route route = {method_from_char(method), request_URI, route_func};
    add_route(&serv_config.routes, route);
}

void stop_server() {
    free_routes(&serv_config.routes);
}

void server_run() {
    printf("serve runs\n");

    int fd;
    printf("sockfd : %d\n", serv_config.fd_server);

    int stop_running = 1;

    while (stop_running) {
        fd = accept(serv_config.fd_server, NULL, NULL);
        char buf[2048] = {0};
        read(fd, buf, 2047);

        struct request *req = request(buf);

        int has_found = 0;
        for (int i = 0; i < serv_config.routes.count; i++) {
            struct server_route current_route = serv_config.routes.routes[i];

            if (current_route.method == req->header.method &&
                strcmp(req->header.request_URI, current_route.request_URI) == 0) {
                struct response rep = current_route.route_func(*req);

                char *response_header = make_response(rep.code, rep.body);

                ssize_t ret_write = write(fd, response_header, strlen(response_header));
                write(fd, rep.body, strlen(rep.body));


                printf("ret_write : %lu\n", ret_write);
                has_found = 1;
                break;
            }
        }
        if (!has_found) {
            char *response_error = make_response(R_404, NULL);
            write(fd, response_error, strlen(response_error));
        }
        CLOSE_FD(fd);
    }
    CLOSE_FD(serv_config.fd_server);
}
