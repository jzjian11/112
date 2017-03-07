all: proxy

proxy: proxy.o connection.o http.o blacklist.o cache.o
	gcc -Wall -g -o proxy proxy.o connection.o http.o blacklist.o cache.o -lpthread

proxy.o: proxy.c
	gcc -Wall -g -c proxy.c -lpthread

connection.o: connection.h connection.c
	gcc -Wall -g -c connection.c -lpthread

http.o: http.h http.c
	gcc -Wall -g -c http.c -lpthread

blacklist.o: blacklist.h blacklist.c
	gcc -Wall -g -c blacklist.c -lpthread

cache.o: cache.h cache.c
	gcc -Wall -g -c cache.c -lpthread

clean:
	rm -rf *.o proxy
