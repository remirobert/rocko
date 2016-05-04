//
// Created by Remi Robert on 30/04/16.
//

#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <string.h>
#include <err.h>
#include <stdlib.h>

#define NUSERS 10

enum method_request {
    GET,
    POST,
    UNKNOW
};

struct header {
    enum method_request method;
    char *request_URI;
    char *http_version;
    char *connection;
    char *host;
    char *accept_encoding;
    char *accept;
    char *user_agent;
};

struct request {
    struct header header;
    char *body;
};

struct uc {
    struct request req;
    int uc_fd;
    char *uc_addr;
} users[NUSERS];

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

void parse_request(char *buf, struct request *req) {
    char *line;
    char *bufCopy = strdup(buf);

    line = strsep(&bufCopy, "\n");
    req->header.method = method_from_char(strdup(strsep(&line, " ")));
    req->header.request_URI = strdup(strsep(&line, " "));
    req->header.http_version = strdup(strsep(&line, " "));

    line = NULL;
    while ((line = strsep(&bufCopy, "\n"))) {
        char *token = strsep(&line, " ");
        if (token && line) {
            if (strcmp(token, "Host:") == 0) req->header.host = strdup(line);
            if (strcmp(token, "Connection:") == 0) req->header.connection = strdup(line);
            if (strcmp(token, "Accept-Encoding:") == 0) req->header.accept_encoding = strdup(line);
            if (strcmp(token, "Accept:") == 0) req->header.accept = strdup(line);
            if (strcmp(token, "User-Agent:") == 0) req->header.user_agent = strdup(line);
        }
    }
}


void init_socket() {
    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC; /* any supported protocol */
    hints.ai_flags = AI_PASSIVE; /* result for bind() */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    int error = getaddrinfo ("127.0.0.1", "8080", &hints, &addr);
    if (error)
        errx(1, "getaddrinfo failed: %s", gai_strerror(error));

    local_s = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    bind(local_s, addr->ai_addr, addr->ai_addrlen);
    listen(local_s, 5);
}

#include <sys/event.h>
#include <printf.h>
#include <unistd.h>

int kq;
struct kevent evSet;

void init_event_k() {
    kq = kqueue();

    EV_SET(&evSet, local_s, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1)
        err(1, "kevent");

    watch_loop(kq);
}

void watch_loop(int kq) {
    struct kevent evSet;
    struct kevent evList[32];
    int nev, i;
    struct sockaddr_storage addr;
    socklen_t socklen = sizeof(addr);
    int fd;

    while(1) {
        nev = kevent(kq, NULL, 0, evList, 32, NULL);
        if (nev < 1)
            err(1, "kevent");
        for (i=0; i<nev; i++) {
            if (evList[i].flags & EV_EOF) {
                printf("disconnect\n");
                fd = evList[i].ident;
                EV_SET(&evSet, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1)
                    err(1, "kevent");
                conn_delete(fd);
            }
            else if (evList[i].ident == local_s) {
                fd = accept(evList[i].ident, (struct sockaddr *)&addr,
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
                send_msg(fd, "HTTP/1.1 200 OK\r\n\r\n");
            }
            else if (evList[i].flags & EVFILT_READ) {
                recv_request(evList[i].ident);
                send_msg(fd, "HTTP/1.1 200 OK\r\n\nConnection: close\r\n\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n");
                close(fd);
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
        return ;

    bytes_read = recv(s, buf, sizeof(buf), 0);
    if (bytes_read != -1)
        printf("%d bytes read\n", (int)bytes_read);
    printf("buf : %s\n", buf);
    parse_request(buf, &users[uidx].req);

    printf("request method : %s\n", users[uidx].req.header.request_URI);
}

/* find the index of a file descriptor or a new slot if fd=0 */
int
conn_index(int fd) {
    int uidx;
    for (uidx = 0; uidx < NUSERS; uidx++)
        if (users[uidx].uc_fd == fd)
            return uidx;
    return -1;
}

/* add a new connection storing the IP address */
int
conn_add(int fd) {
    int uidx;
    if (fd < 1) return -1;
    if ((uidx = conn_index(0)) == -1)
        return -1;
    if (uidx == NUSERS) {
        close(fd);
        return -1;
    }
    users[uidx].uc_fd = fd; /* users file descriptor */
    users[uidx].uc_addr = 0; /* user IP address */
    return 0;
}

/* remove a connection and close it's fd */
int
conn_delete(int fd) {
    int uidx;
    if (fd < 1) return -1;
    if ((uidx = conn_index(fd)) == -1)
        return -1;

    users[uidx].uc_fd = 0;
    users[uidx].uc_addr = NULL;

    /* free(users[uidx].uc_addr); */
    return close(fd);
}

int main(int argc, char **argv) {
    init_socket();
    init_event_k();
    return 0;
}