#include <stdio.h>
#include <stdlib.h>
#include <mach/host_priv.h>

#include "server.h"

struct response hello_world(struct request req) {
    printf("hello world route\n");
    return (struct response){R_200, "hello world"};
}

struct response upper_hello_world(struct request req) {
    printf("upper hello world route\n");
    return (struct response){R_200, "HELLO WORLD"};
}

struct response page_test(struct request req) {
    return (struct response){R_200, "<html><head><title>C-custom server test</title>\\\n"
            "             </head><body><h1>Hello world</h1></body></html>"};
}

int main(int argc, char **argv) {

    init_server(4242);
    server_add_route("GET", "/", hello_world);
    server_add_route("GET", "/upper", upper_hello_world);
    server_add_route("GET", "/page", page_test);
    server_run();
    return 0;
}