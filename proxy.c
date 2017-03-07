#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "connection.h"
#include "blacklist.h"

// Start the proxy
void start_proxy(int port);

int main(int argc, char *argv[]) {
    // The default port
    int port = 8080;

    if (argc >= 2) {
        // Get port from command-argument
        port = atoi(argv[1]);
    }

    load_blacklist("blacklist.conf");

    start_proxy(port);

    return 0;
}

void start_proxy(int port) {
    // Init the address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;

    // Create the listen socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    // Set the SO_REUSEADDR socket option
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    // Bind and Listen
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(fd, 128) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Listenning on %d.\n", port);

    int conn = -1;
    pthread_t pid;
    // Set pthread PTHREAD_CREATE_DETACHED
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    while (1) {
        // Accept connection from client
        conn = accept(fd, NULL, NULL);
        if (conn < -1) {
            perror("accept");
            continue;
        }

        // Create a new pthread to handle the client connection
        if (pthread_create(&pid, &attr, connection_handler, (void*)(long)conn) < 0) {
            perror("pthread_create");
            exit(1);
        }
    }
}
