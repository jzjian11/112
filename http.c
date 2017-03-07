#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "http.h"

// Free resourse of the http request
void free_http_request(http_request *request) {
    if (request->method) {
        free(request->method);
    }
    if (request->uri) {
        free(request->uri);
    }
    if (request->host) {
        free(request->host);
    }
}

void handler_header(http_request *request, char *header) {
    char *p1 = NULL, *p2 = NULL;
    if (request->method == NULL) {
        // Need handle the first line of the header
        p1 = strstr(header, " ");
        p1[0] = '\0';
        p1 += 1;
        p2 = strstr(p1, " ");
        p2[0] = '\0';
        p2 += 1;
        request->method = strdup(header);
        request->uri = strdup(p1);
    } else {
        // Handler header
        p1 = strstr(header, ":");
        if (p1 == NULL) {
            return;
        }
        p1[0] = '\0';
        p1 += 1;
        if (p1[0] == ' ') {
            p1[0] = '\0';
            p1 += 1;
        }
        if (strcasecmp(header, "Host") == 0) {
            p2 = strstr(p1, ":");
            if (p2 == NULL) {
                request->port = 80;
            } else {
                p2[0] = '\0';
                request->port = atoi(p2 + 1);
            }
            request->host = strdup(p1);
        }
    }
}

// Read a http request from a socket
http_request* read_http_request(int fd) {
    http_request *request = calloc(1, sizeof(http_request));

    // First read headers
    int n = 0;
    char *p = NULL;

    char buf[MAX_BUFFER];
    int buf_len = 0;
    while (sizeof(buf) - buf_len > 0) {
        // Read some data to the buf
        n = read(fd, buf + buf_len, sizeof(buf) - buf_len);
        if (n < 0) {
            perror("read");
            free_http_request(request);
            return NULL;
        }
        buf_len += n;
        buf[buf_len] = '\0';
        if (strstr(buf, "\r\n\r\n") != NULL) {
            // If header finished, break
            break;
        }
    }
    memcpy(request->buf, buf, buf_len);
    request->buf_len = buf_len;

    p = strtok(buf, "\r\n");
    while (p) {
        handler_header(request, p);
        p = strtok(NULL, "\r\n");
    }
    return request;
}


void http_response_404(int fd) {
    char *res = "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: 14\r\n\r\n"
                "404 Not Found\n";
    write(fd, res, strlen(res));
}
