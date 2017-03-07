#ifndef _BLACKLIST_H_
#define _BLACKLIST_H_

#define MAX_BLACKLIST 128
#define MAX_HOST_LEN 128

// Load blacklist from file
int load_blacklist(char *path);

// Check if host in blacklist
int check_blacklist(char *host);

#endif
