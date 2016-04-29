//
// Created by Remi Robert on 29/04/16.
//

#include <stdlib.h>
#include <printf.h>
#include "server.h"

void route_init(struct server_routes *routes) {
    routes->size = ROUTE_INITIAL_CAPACITY;
    routes->count = 0;
    routes->routes = malloc(sizeof(struct server_route) * routes->size);
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