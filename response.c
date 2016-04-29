//
// Created by Remi Robert on 28/04/16.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "server.h"

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

char * make_response(enum response_code code, char *body) {
    size_t size_content = (body != NULL) ? strlen(body) : 0;
    char *header_response = malloc(256 + size_content * sizeof(char));
    memset(header_response, 0, 256 + size_content);
    struct response_status_code code_infos = response_codes_informations[code];

    sprintf(header_response, "HTTP/1.1 %s %s\r\n\
            Connection: close\r\n\
            Content-Type: text/plain\r\n\
            Content-Length: %zu\r\n\\r\n",
            code_infos.code, code_infos.info_status, size_content);
    return header_response;
}