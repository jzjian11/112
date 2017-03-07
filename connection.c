#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#include "http.h"
#include "connection.h"
#include "blacklist.h"
#include "cache.h"

void *connection_handler(void *data) {
    // Resume fd from data
    int fd = (int)(long)data;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    char client_ip[16];
    int client_port;

    // Get client address from fd
    getpeername(fd, (struct sockaddr*)&client_addr, &client_addr_len);
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, sizeof(client_ip));
    client_port = ntohs(client_addr.sin_port);

    printf("[%s:%d] client connected.\n", client_ip, client_port);

    // Read request from fd
    http_request *request = read_http_request(fd);

    if (request == NULL) {
        printf("[%s:%d] read http request failed.\n", client_ip, client_port);
        close(fd);
        pthread_exit(NULL);
    }

    printf("[%s:%d] read http request success, [request->host: %s] [request->uri: %s]\n",
            client_ip, client_port, request->host, request->uri);

    // Check blacklist
    if (check_blacklist(request->host) == 1) {
        printf("[%s:%d] %s in blacklist, response 404\n", client_ip, client_port, request->host);
        http_response_404(fd);
        close(fd);
        pthread_exit(NULL);
    }

    cache_item *cache = load_cache(request->host, request->uri);
    if (cache != NULL) {
        // just send cache result to client
        send_cache(cache, fd);

        printf("[%s:%d] found in cache, just return the result. [request->host: %s] [request->uri: %s]\n",
                client_ip, client_port, request->host, request->uri);

        close(fd);
        pthread_exit(NULL);
    }


    int server_fd = connect_host(request->host, request->port);
    if (server_fd < 0) {
        printf("[%s:%d] connect to host failed, [request->host: %s][request->port: %d]\n",
                client_ip, client_port, request->host, request->port);
        close(fd);
        pthread_exit(NULL);
    }

    printf("[%s:%d] connect to host success, [request->host: %s][request->port: %d]\n",
            client_ip, client_port, request->host, request->port);

    // Write request to server_fd
    write(server_fd, request->buf, request->buf_len);

    cache = calloc(1, sizeof(cache_item));
    cache->host = strdup(request->host);
    cache->uri = strdup(request->uri);

    // Using select to listen fd and server_fd
    fd_set fds;
    int max_fd = (server_fd > fd ? server_fd : fd) + 1;
    char buf[MAX_BUFFER];
    int n = 0;
    struct timeval tv;
    while (1) {
        // Set timeout 1s
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(server_fd, &fds);
        if (select(max_fd, &fds, NULL, NULL, &tv) <= 0) {
            break;
        }
        if (FD_ISSET(fd, &fds)) {
            // Read data to client
            n = read(fd, buf, sizeof(buf));
            if (n == 0) {
                printf("[%s:%d] client closed\n", client_ip, client_port);
                break;
            }
            if (n < 0) {
                printf("[%s:%d] read data from client failed\n", client_ip, client_port);
                close(fd);
                close(server_fd);
                pthread_exit(NULL);
            }
            // Write data to server
            write(server_fd, buf, n);
        }
        if (FD_ISSET(server_fd, &fds)) {
            // Read data to server
            n = read(server_fd, buf, sizeof(buf));
            if (n == 0) {
                printf("[%s:%d] server closed\n", client_ip, client_port);
                break;
            }
            if (n < 0) {
                printf("[%s:%d] read data from client failed\n", client_ip, client_port);
                close(fd);
                close(server_fd);
                pthread_exit(NULL);
            }
            // Write data to client
            write(fd, buf, n);
            add_cache_buffer(cache, buf, n);
        }
    }
    save_cache(cache);


    printf("[%s:%d] proxy success. [request->host: %s] [request->uri: %s]\n",
            client_ip, client_port, request->host, request->uri);

    close(fd);
    close(server_fd);
    pthread_exit(NULL);
}

int connect_host(char *host, int port) {
    char buf[MAX_BUFFER];
    strcpy(buf, host);

    char ip[16];

    // Get IP from host
    struct hostent *he = gethostbyname(buf);
    if (he == NULL) {
        perror("gethostbyname");
        return -1;
    }
    inet_ntop(AF_INET, (he->h_addr_list)[0], ip, sizeof(ip));

    // Init the server address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    // Connect to the server
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return -1;
    }

    return fd;
}
