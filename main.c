//
// Created by Remi Robert on 11/05/16.
//

#include <printf.h>
#include "rocko.h"

struct response hello_world(struct request req) {
    printf("hello world route\n");
    return (struct response){R_200, "hello world"};
}

struct response upper_hello_world(struct request req) {
    printf("upper hello world route\n");
    return (struct response){R_200, "HELLO WORLD"};
}

struct response page_test(struct request req) {
    return (struct response) {R_200, "<html><head><title>C-custom server test</title>\\\n"
            "             </head><body><h1>Hello world</h1></body></html>"};
}

int main(int argc, char **argv) {
    rocko_init();
    rocko_add_route("GET", "/", hello_world);
    rocko_add_route("GET", "/upper", upper_hello_world);
    rocko_add_route("GET", "/page", page_test);
    rocko_start(8080);
    return 0;
}