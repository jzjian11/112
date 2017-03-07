#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "blacklist.h"

char *blacklist[MAX_BLACKLIST];
int blacklist_len = 0;

// Load blacklist from file
int load_blacklist(char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }
    char host[MAX_HOST_LEN];
    while (fgets(host, sizeof(host), fp) != NULL) {
        // trim the last "\n"
        host[strlen(host) - 1] = '\0';
        blacklist[blacklist_len++] = strdup(host);
    }
    fclose(fp);
    return 0;
}

// Check if host in blacklist
int check_blacklist(char *host) {
    int i = 0;
    for (i = 0; i < blacklist_len; i++) {
        if (strcmp(blacklist[i], host) == 0) {
            return 1;
        }
    }
    return 0;
}
