//
// Created by Remi Robert on 11/05/16.
//

#ifndef ROCKO_ROCKO_H
#define ROCKO_ROCKO_H

enum response_code {
    R_100, R_101, R_200, R_201, R_202, R_203, R_204,
    R_205, R_206, R_300, R_301, R_304, R_305, R_307,
    R_400, R_401, R_402, R_403, R_404, R_405, R_406,
    R_407, R_408, R_409, R_410, R_411, R_412, R_413,
    R_414, R_415, R_416, R_417, R_500, R_501, R_502,
    R_503, R_504, R_505
};

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

struct response {
    enum response_code code;
    char *body;
};

struct request {
    struct header header;
    char *body;
};

struct server_route {
    enum method_request method;
    char *request_URI;
    struct response (* route_func)(struct request);
};

void init_socket();
void server_add_route(char *method, char *request_URI, struct response (* route_func)(struct request));


#endif //ROCKO_ROCKO_H
