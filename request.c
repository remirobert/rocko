#include <stdlib.h>
#include <string.h>
#include "server.h"

enum method_request method_from_char(char *method) {
    if (strcmp(method, "GET") == 0) return GET;
    else if (strcmp(method, "POST") == 0) return POST;
    return UNKNOW;
}

struct request *request(char *buf) {
    struct request *req = (struct request *)calloc(2, sizeof(struct request));
    char *line;
    char *bufCopy = strdup(buf);

    line = strsep(&bufCopy, "\n");
    req->header.method = method_from_char(strdup(strsep(&line, " ")));
    req->header.request_URI = strdup(strsep(&line, " "));
    req->header.http_version = strdup(strsep(&line, " "));

    line = NULL;
    while((line = strsep(&bufCopy, "\n"))) {
        char *token = strsep(&line, " ");
        if (token && line) {
            if (strcmp(token, "Host:") == 0) req->header.host = strdup(line);
            if (strcmp(token, "Connection:") == 0) req->header.connection = strdup(line);
            if (strcmp(token, "Accept-Encoding:") == 0) req->header.accept_encoding = strdup(line);
            if (strcmp(token, "Accept:") == 0) req->header.accept = strdup(line);
            if (strcmp(token, "User-Agent:") == 0) req->header.user_agent = strdup(line);
        }
    }
    return req;
}