//
// Created by Remi Robert on 30/04/16.
//

#include <sys/socket.h>
#include <sys/event.h>
#include <printf.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <err.h>
#include <stdlib.h>

#include "rocko.h"

#define NUMBER_USERS 10
#define ROUTE_INITIAL_CAPACITY  50
#define CLOSE_FD(fd) do {   \
    if (-1 != (fd)) {   \
        close(fd);  \
        (fd) = -1;   \
    }   \
} while (0)

struct response_status_code {
    char *code;
    char *info_status;
};
 struct response_status_code response_codes_informations[38] = {
        {"100", "Continue"},
        {"101", "Switching Protocols"},
        {"200", "OK"},
        {"201", "Created"},
        {"202", "Accepted"},
        {"203", "Non-Authoritative Information"},
        {"204", "No Content"},
        {"205", "Reset Content"},
        {"206", "Partial Content"},
        {"300", "Multiple Choices"},
        {"301", "Moved Permanently"},
        {"304", "Not Modified"},
        {"305", "Use Proxy"},
        {"307", "Temporary Redirect"},
        {"400", "Bad Request"},
        {"401", "Unauthorized"},
        {"402", "Payment Required"},
        {"403", "Forbidden"},
        {"404", "Not Found"},
        {"405", "Method Not Allowed"},
        {"406", "Not Acceptable"},
        {"407", "Proxy Authentication Required"},
        {"408", "Request Time-out"},
        {"409", "Conflict"},
        {"410", "Gone"},
        {"411", "Length Required"},
        {"412", "Precondition Failed"},
        {"413", "Request Entity Too Large"},
        {"414", "Request-URI Too Large"},
        {"415", "Unsupported Media Type"},
        {"416", "Requested range not satisfiable"},
        {"417", "Expectation Failed"},
        {"500", "Internal Server Error"},
        {"501", "Not Implemented"},
        {"502", "Bad Gateway"},
        {"503", "Service Unavailable"},
        {"504", "Gateway Time-out"},
        {"505", "HTTP Version not supported"}
};

struct uc {
    struct request req;
    int uc_fd;
    char *uc_addr;
} users[NUMBER_USERS];

struct server_routes {
    int size;
    int count;
    struct server_route *routes;
};

struct server_routes *routes;
struct addrinfo *addr;
struct addrinfo hints;
int local_s;

void watch_loop(int);
static int conn_index(int);
static int conn_add(int);
static int conn_delete(int);
void send_msg(int s, char *message, ...);
void recv_request(int s);

enum method_request method_from_char(char *method) {
    if (strcmp(method, "GET") == 0) return GET;
    else if (strcmp(method, "POST") == 0) return POST;
    return UNKNOW;
}

struct server_routes *route_init() {
    struct server_routes *routes = malloc(sizeof(struct server_routes));
    routes->size = ROUTE_INITIAL_CAPACITY;
    routes->count = 0;
    routes->routes = malloc(sizeof(struct server_route) * routes->size);
    return routes;
}

void check_size_capacity(struct server_routes *routes) {
    if (routes->count < routes->size) {
        return;
    }
    routes->size *= 2;
    routes->routes = realloc(routes->routes, sizeof(struct server_route) * routes->size);
}

void add_route(struct server_routes *routes, struct server_route route) {
    check_size_capacity(routes);
    routes->routes[routes->count++] = route;
}

void free_routes(struct server_routes *routes) {
    free(routes->routes);
}

void rocko_add_route(char *method, char *request_URI, struct response (* route_func)(struct request)) {
    struct server_route route = {method_from_char(method), request_URI, route_func};
    add_route(routes, route);
}

char *make_response(enum response_code code, char *body) {
    size_t size_content = (body != NULL) ? strlen(body) : 0;
    char *header_response = malloc(256 + size_content * sizeof(char));
    memset(header_response, 0, 256 + size_content);
    struct response_status_code code_infos = response_codes_informations[code];

    sprintf(header_response, "HTTP/1.1 %s %s\r\n\
            Connection: close\r\n\
            Content-Type: text/plain\r\n\
            Content-Length: %zu\r\n\r\n%s",
            code_infos.code, code_infos.info_status, size_content, body);
    return header_response;
}

void parse_request(char *buf, struct request *req) {
    int isBody = 0;
    char *line;
    char *buf_copy = strdup(buf);

    printf("[READ REQUEST] : [%s]\n\n", buf);

    line = strsep(&buf_copy, "\n");
    printf("current line request : %s\n", line);
    req->header.method = method_from_char(strdup(strsep(&line, " ")));
    printf("current line request : %s\n", line);
    req->header.request_URI = strdup(strsep(&line, " "));
    printf("current line request : %s\n", line);
    req->header.http_version = strdup(strsep(&line, " "));
    printf("current line request : %s\n", line);

    line = NULL;
    while ((line = strsep(&buf_copy, "\n"))) {
        if (strlen(line) == 1 && strcmp(line, "\n")) {
            isBody = 1;
            continue;
        }
        if (!isBody) {
            char *token = strsep(&line, " ");
            if (!isBody && token && line) {
                if (strcmp(token, "Host:") == 0) req->header.host = strdup(line);
                if (strcmp(token, "Connection:") == 0) req->header.connection = strdup(line);
                if (strcmp(token, "Accept-Encoding:") == 0) req->header.accept_encoding = strdup(line);
                if (strcmp(token, "Accept:") == 0) req->header.accept = strdup(line);
                if (strcmp(token, "User-Agent:") == 0) req->header.user_agent = strdup(line);
            }
        }
        else {
            req->body = strdup(line);
            return;
        }
    }
}

int kq;
struct kevent evSet;

void init_event_k() {
    kq = kqueue();

    EV_SET(&evSet, local_s, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1)
        err(1, "kevent");

    watch_loop(kq);
}

void init_socket(unsigned int port) {
    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC; /* any supported protocol */
    hints.ai_flags = AI_PASSIVE; /* result for bind() */
    hints.ai_socktype = SOCK_STREAM; /* TCP */

    char buf_port[5] = {0};
    sprintf(buf_port, "%d", port);
    int error = getaddrinfo("127.0.0.1", buf_port, &hints, &addr);
    if (error)
        errx(1, "getaddrinfo failed: %s", gai_strerror(error));

    local_s = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (bind(local_s, addr->ai_addr, addr->ai_addrlen) == -1) {
        errx(1, "bind failed: error binding socket");
    }
    listen(local_s, 5);
}

void rocko_init() {
    routes = route_init();
}

void rocko_start(unsigned int port) {
    init_socket(port);
    init_event_k();
}

void response_error(int fd) {
    char *response_error = make_response(R_404, NULL);
    send_msg(fd, response_error);
}

void match_route_request(struct request *req, int fd) {
    int has_found = 0;

    printf("march route : %s\n", req->header.request_URI);

    if (req == NULL || routes == NULL) {
        response_error(fd);
        return;
    }

    for (int i = 0; i < routes->count; i++) {
        struct server_route current_route = routes->routes[i];

        if (current_route.method == req->header.method &&
            strcmp(req->header.request_URI, current_route.request_URI) == 0) {
            struct response rep = current_route.route_func(*req);

            char *response = make_response(rep.code, rep.body);

            send_msg(fd, response);

//            ssize_t ret_write = write(fd, response_header, strlen(response_header));
//            write(fd, rep.body, strlen(rep.body));

            has_found = 1;
            break;
        }
    }
    if (!has_found) {
        response_error(fd);
    }
}

void watch_loop(int kq) {
    struct kevent evSet;
    struct kevent evList[32];
    int nev, i;
    struct sockaddr_storage addr;
    socklen_t socklen = sizeof(addr);
    int fd;

    while (1) {
        nev = kevent(kq, NULL, 0, evList, 32, NULL);
        if (nev < 1)
            err(1, "kevent");
        for (i = 0; i < nev; i++) {
            if (evList[i].flags & EV_EOF) {
                printf("disconnect\n");
                fd = evList[i].ident;
                EV_SET(&evSet, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1)
                    err(1, "kevent");
                conn_delete(fd);
            }
            else if (evList[i].ident == local_s) {
                fd = accept(evList[i].ident, (struct sockaddr *) &addr,
                            &socklen);
                if (fd == -1)
                    err(1, "accept");
                if (conn_add(fd) == 0) {
                    EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                    if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1)
                        err(1, "kevent");
//                    send_msg(fd, "welcome!\n");
                } else {
                    printf("connection refused\n");
                    close(fd);
                }
            }
            else if (evList[i].flags & EVFILT_WRITE) {
                printf("send request : \n");
//                send_msg(fd, "HTTP/1.1 200 OK\r\n\r\n");
            }
            else if (evList[i].flags & EVFILT_READ) {
                recv_request(evList[i].ident);
            }
        }
    }
}

void send_msg(int s, char *message, ...) {
    char buf[256];
    int len;

    printf("send response \n");
    va_list ap;
    va_start(ap, message);
    len = vsnprintf(buf, sizeof(buf), message, ap);
    va_end(ap);
    send(s, buf, len, 0);
}

void recv_request(int s) {
    char buf[256] = {0};
    size_t bytes_read;
    int uidx;

    if ((uidx = conn_index(s)) == -1)
        return;

    bytes_read = recv(s, buf, sizeof(buf), 0);
    if (bytes_read != -1)
        printf("%d bytes read\n", (int) bytes_read);
    printf("buf : %s\n", buf);

    struct uc current_user = users[uidx];

    parse_request(buf, &current_user.req);
    match_route_request(&current_user.req, s);

//    char *response = make_response(R_200, current_user.req.body);
//
//    send_msg(s, response);
    CLOSE_FD(s);
    conn_delete(s);
}

/* find the index of a file descriptor or a new slot if fd=0 */
int conn_index(int fd) {
    int uidx;
    for (uidx = 0; uidx < NUMBER_USERS; uidx++)
        if (users[uidx].uc_fd == fd)
            return uidx;
    return -1;
}

/* add a new connection storing the IP address */
int conn_add(int fd) {
    int uidx;
    if (fd < 1) return -1;
    if ((uidx = conn_index(0)) == -1)
        return -1;
    if (uidx == NUMBER_USERS) {
        CLOSE_FD(fd);
        return -1;
    }
    users[uidx].uc_fd = fd; /* users file descriptor */
    users[uidx].uc_addr = 0; /* user IP address */
    return 0;
}

/* remove a connection and close it's fd */
int conn_delete(int fd) {
    int uidx;
    if (fd < 1) return -1;
    if ((uidx = conn_index(fd)) == -1)
        return -1;

    users[uidx].uc_fd = 0;
    users[uidx].uc_addr = NULL;

    /* free(users[uidx].uc_addr); */
    CLOSE_FD(fd);
    return 0;
}