//
// Created by Remi Robert on 28/04/16.
//

#ifndef SOCKETTEST_SERVER_H
#define SOCKETTEST_SERVER_H

#define ROUTE_INITIAL_CAPACITY  50

enum method_request {
    GET,
    POST,
    UNKNOW
};

enum response_code {
    R_100 = 0,
    R_101,
    R_200,
    R_201,
    R_202,
    R_203,
    R_204,
    R_205,
    R_206,
    R_300,
    R_301,
    R_304,
    R_305,
    R_307,
    R_400,
    R_401,
    R_402,
    R_403,
    R_404,
    R_405,
    R_406,
    R_407,
    R_408,
    R_409,
    R_410,
    R_411,
    R_412,
    R_413,
    R_414,
    R_415,
    R_416,
    R_417,
    R_500,
    R_501,
    R_502,
    R_503,
    R_504,
    R_505
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

struct request{
    struct header header;
    char *body;
};

struct response {
    enum response_code code;
    char *body;
};

struct server_route {
    enum method_request method;
    char *request_URI;
    struct response (* route_func)(struct request);
};

struct server_routes {
    int size;
    int count;
    struct server_route *routes;
};

struct server_config {
    int port;
    int fd_server;
    struct request *req;
    struct response *rep;
    struct server_routes routes;
};

struct request *request(char *buf);
void init_server(int port);
enum method_request method_from_char(char *method);
void server_add_route(char *method, char *request_URI, struct response (* route_func)(struct request));
//void add_route(char *method, char *request_URI, char *(* route_func)(struct request));
void server_run();
char * make_response(enum response_code code, char *body);

#endif //SOCKETTEST_SERVER_H
