#ifndef _CONNECTION_H_
#define _CONNECTION_H_

// The pthread handler function
void *connection_handler(void *data);

// Connect to host and return the fd
int connect_host(char *host, int port);


#endif
