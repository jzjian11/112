#ifndef _HTTP_H_
#define _HTTP_H_

#define MAX_HEADERS 64
#define MAX_BUFFER 4096

typedef struct {
    // For the request
    char *method;
    char *uri;
    char *host;
    int port;

    char buf[MAX_BUFFER];
    int buf_len;
} http_request;

// Free a http request
void free_http_reqeust(http_request *request) ;

// Read a http buffer from a socket 
http_request* read_http_request(int fd);

// Response a 404 to client
void http_response_404(int fd);


#endif
