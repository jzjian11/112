#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cache.h"

cache_item **caches = NULL;
int caches_len = 0;

void save_cache(cache_item *cache) {
    if (caches_len == 0) {
        caches = malloc(sizeof(cache_item *));
    } else {
        caches = realloc(caches, sizeof(cache_item *) * (caches_len + 1));
    }
    caches[caches_len++] = cache;
}

cache_item *load_cache(char *host, char *uri) {
    int i = 0;
    for (i = 0; i < caches_len; i++) {
        if (strcmp(caches[i]->host, host) == 0 && strcmp(caches[i]->uri, uri) == 0) {
            return caches[i];
        }
    }
    return NULL;
}

void add_cache_buffer(cache_item *cache, char *buf, int size) {
    cache_buffer *buffer = malloc(sizeof(cache_buffer));
    buffer->buf = malloc(size);
    buffer->size = size;
    memcpy(buffer->buf, buf, size);
    if (cache->buffers_len == 0) {
        cache->buffers = malloc(sizeof(cache_buffer*));
    } else {
        cache->buffers = realloc(cache->buffers, sizeof(cache_buffer*) * (cache->buffers_len + 1));
    }
    cache->buffers[cache->buffers_len++] = buffer;
}

void send_cache(cache_item *cache, int fd) {
    int i = 0;
    for (i = 0; i < cache->buffers_len; i++) {
        write(fd, cache->buffers[i]->buf, cache->buffers[i]->size);
    }
}
