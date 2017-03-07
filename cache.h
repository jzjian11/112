#ifndef _CACHE_H_
#define _CACHE_H_

typedef struct {
    char *buf;
    int size;
} cache_buffer;

typedef struct {
    char *host;
    char *uri;
    cache_buffer **buffers;
    int buffers_len;
} cache_item;

void save_cache(cache_item *cache);

cache_item *load_cache(char *host, char *uri);

void add_cache_buffer(cache_item *cache, char *buf, int size);

void send_cache(cache_item *cache, int fd);


#endif
