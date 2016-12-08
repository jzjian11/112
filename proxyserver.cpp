#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pthread.h>



#define MAXSIZE 65507
#define HTTP_PORT 80


#define CACHE_MAXSIZE 100
#define DATELENGTH 40


struct HttpHeader{
	char method[4]; // POST or GET
	char url[1024]; //destination url
	char host[1024];
	char cookie[1024 * 10];
	HttpHeader() {
		memset(this, 0, sizeof(HttpHeader));
	}
};


//used to find items in cache
struct cache_HttpHeader{
	char method[4];
	char url[1024];
	char host[1024];
	cache_HttpHeader() {
		memset(this, 0, sizeof(cache_HttpHeader));
	}
};

//struct used to cache
struct __CACHE{
	cache_HttpHeader htphed;
	char buffer[MAXSIZE];
	char date[DATELENGTH];
	__CACHE() {
		memset(&this->buffer, 0, sizeof(buffer));
		memset(&this->date, 0, sizeof(date));
	}
};

int max(int a, int b)
{
	return a >= b ? a : b;
}

int __CACHE_number = 0; //index of next item in cache;
__CACHE cache[CACHE_MAXSIZE];

bool InitSocket();
bool ParseHttpHead(char *buffer, HttpHeader *HttpHeader);
bool ConnectToServer(int *serverSocket, char *host);


int ProxyServer;
struct sockaddr_in ProxyServerAddr;
const int ProxyPort = 8887;
//blocked website
char *host[10] = {"youtube.com", "hotmail.com", "amazon.com", "netflix.com"};
const int host_number = 4;

int Cache_find(__CACHE *cache, HttpHeader htp);
//redirect from host_to_another to another
char* host_to_another = "djangobook.py3k.cn";
char* another[2] = { "tufts.edu","http://tufts.edu/" };

struct ProxyParam
{
	int clientSocket;
	int serverSocket;
};



bool Isequal(cache_HttpHeader htp1, HttpHeader htp2)
{
	if(strcmp(htp1.method, htp2.method))
		return false;
	if(strcmp(htp1.url, htp2.url))
		return false;
	if(strcmp(htp1.host, htp2.host))
		return false;
	return true;
}

int Cache_find(__CACHE *cache, HttpHeader htp)
{
	int i = 0;
	for(i = 0; i < CACHE_MAXSIZE; i++)
	{
		if(Isequal(cache[i].htphed, htp))
			return i;
	}

	return -1;
}

bool ParseHttpHead(char *buffer, HttpHeader *httpHeader)
{
	char *p;
	bool change = false;
	char *ptr;
	const char *delim = "\r\n";

	p = strtok_r(buffer, delim, &ptr);
	printf("%s\n", p);
	//GET request
	if(p[0] == 'G')
	{
		memcpy(&httpHeader->method, "GET", 3);
		memcpy(&httpHeader->url, &p[4], strlen(p) - 13);
	}
	//POST
	else if(p[0] == 'P')
	{
		memcpy(&httpHeader->method, "POST", 4);
		memcpy(&httpHeader->url, &p[5], strlen(p) - 14);
	}
	printf("%s\n", httpHeader->url);
	p = strtok_r(NULL, delim, &ptr);
	printf("%s-135\n", p);
	while(p)
	{
		switch(p[0])
		{
			//Host
			case 'H':
				printf("Reach 142\n");
				if(!memcmp(&p[6], host_to_another, strlen(p) - 6))
				{
					printf("Reach 145\n");
					memcpy(&httpHeader->host, another[0], strlen(another[0]));
					memcpy(&httpHeader->url, another[1], strlen(another[1]));
					change = true;
				}
				else
				{
					printf("Reach 152\n");
					memcpy(&httpHeader->host, &p[6], strlen(p) - 6);

				}
				break;
			//cookie
			case 'C':
				if(strlen(p) > 8)
				{
					char header[8];
					memset(&header, 0, sizeof(header));
					memcpy(&header, p, 6);
					if(!strcmp(header, "Cookie"))
					{
						memcpy(&httpHeader->cookie, &p[8], strlen(p) - 8);
					}
				}
				break;
			default:
				break;
		}
		p = strtok(NULL, delim);
	}
	printf("%s-175\n", httpHeader->host);
	return change;
}

bool ConnectToServer(int *serverSocket, char *host)
{
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(HTTP_PORT);
	struct hostent *HOSTENT = gethostbyname(host);
	if(HOSTENT == NULL)
		return false;
	in_addr Inaddr = *((in_addr*)*HOSTENT->h_addr_list);
	serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));
	*serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(*serverSocket < 0)
		return false;
	if(connect(*serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		close(*serverSocket);
		return false;
	}

	return true;
}

bool InitSocket(){
	ProxyServer = socket(AF_INET, SOCK_STREAM, 0);
	if(ProxyServer < 0)
	{
		printf("Fail in 175\n");
		return false;
	}
	printf("%d\n", ProxyServer);
	memset(&ProxyServerAddr, 0, sizeof(ProxyServerAddr));
	ProxyServerAddr.sin_family = AF_INET;
	ProxyServerAddr.sin_port = htons(ProxyPort);
	ProxyServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	printf("%d\n", sizeof(ProxyServerAddr));
	if(bind(ProxyServer, (struct sockaddr *)&ProxyServerAddr, sizeof(ProxyServerAddr)) < 0)
	{
		printf("Fail in 184\n");
		return false;
	}

	if(listen(ProxyServer, 10) < 0)
	{
		printf("Fail in 190\n");
		return false;
	}

	return true;
}

void *ProxyThread(void* lpParameter)
{
	char Buffer[MAXSIZE];
	memset(&Buffer, 0, MAXSIZE);
	struct sockaddr_in clientAddr;
	int length = sizeof(sockaddr_in);
	int recvSize = recv(((ProxyParam*)lpParameter)->clientSocket, Buffer, MAXSIZE, 0);
	int ret;

	
	HttpHeader* httpHeader = new HttpHeader();
	char CacheBuffer[recvSize + 1];
	printf("RECVSIZE IS %d\n", recvSize);
	memset(&CacheBuffer, 0, recvSize+1);
	printf("readh here\n");
	printf("SIZE OF CacheBuffer is %d\n", sizeof(CacheBuffer));
	printf("SIZE OF Buffer is %d\n", sizeof(Buffer));
	memcpy(&CacheBuffer, Buffer, recvSize);
	bool change = ParseHttpHead(CacheBuffer, httpHeader);
	int j;
	printf("%s-248\n", httpHeader->host);
	for(j = 0; j < host_number; j++)//search for blocked website
	{
		int i;
		bool find = true;
		for(i = 0; i < strlen(host[j]); i++)
		{
			if(host[j][i] != httpHeader->host[i])
			{
				find = false;
				break;
			}
		}
		if(find)
		{
			printf("close connection in 264\n");
			close(((ProxyParam*)lpParameter)->clientSocket);
			close(((ProxyParam*)lpParameter)->serverSocket);
			return NULL;
		}
	}
	printf("%s-263\n", httpHeader->host);
	if (!ConnectToServer(&((ProxyParam*)lpParameter)->serverSocket, httpHeader->host))
		printf("Fail in 205\n");

	printf("Proxy server %s success(261)\n", httpHeader->host);
	printf("%d\n", change);
	if(change)//dirent to other website
	{
		char CacheBuffer[MAXSIZE];
		memset(&CacheBuffer, 0, sizeof(CacheBuffer));
		int ii = 0, lengthth = 0;
		for(ii = 0; ii < strlen(httpHeader->method); ii++)
			CacheBuffer[lengthth++] = httpHeader->method[ii];

		CacheBuffer[lengthth] = ' ';
		for(ii = 0; ii < strlen(another[1]); ii++)
			CacheBuffer[lengthth++] = another[1][ii];

		CacheBuffer[lengthth++] = ' ';
		char *hh = "HTTP/1.1";
		for(ii = 0; ii < strlen(hh); ii++)
			CacheBuffer[lengthth++] = hh[ii];

		CacheBuffer[lengthth++] = '\r';
		CacheBuffer[lengthth++] = '\n';

		char *hhh = "HOST: ";
		for(ii = 0; ii < strlen(hhh); ii++)
			CacheBuffer[lengthth++] = hhh[ii];
		for(ii = 0; ii < strlen(httpHeader->host); ii++)
			CacheBuffer[lengthth++] = httpHeader->host[ii];

		CacheBuffer[lengthth++] = '\r';
		CacheBuffer[lengthth++] = '\n';

		char *ptr;
		const char *delim = "\r\n";
		char *p = strtok_r(Buffer, delim, &ptr);
		int length1 = strlen(p);
		length1 += 2;
		for(ii = 1; ii < recvSize-length1+1; ii++)
			CacheBuffer[lengthth++] = ptr[ii];
		memcpy(&Buffer, CacheBuffer, max(strlen(CacheBuffer), recvSize));
	}

	int find = Cache_find(cache, *httpHeader);
	printf("Reach 313\n");
	printf("%d\n", find);
	//find content in cache
	//
	if(find >= 0)
	{
		//add if-modified-since
		char Buffer2[MAXSIZE];
		int i = 0, length = 0, length2 = 0;
		memset(&Buffer2, 0, MAXSIZE);
		char CacheBuffer[recvSize+1];
		memset(&CacheBuffer, 0, recvSize+1);
		memcpy(&CacheBuffer, Buffer, recvSize);

		const char *delim = "\r\n";
		char *ptr;
		char *p = strtok_r(CacheBuffer, delim, &ptr);
		length += strlen(p);
		length += 2;
		length2 = length;

		char *ife = "If-Modified-Since: ";
		for(i = 0; i < length; i++)
			Buffer2[i] = Buffer[i];
		for(i = 0; i < strlen(ife); i++)
			Buffer2[length + i] = ife[i];
		length = length + strlen(ife);
		for(i = 0; i < strlen(cache[find].date); i++)
			Buffer2[length + i] = cache[find].date[i];

		length += strlen(cache[find].date);
		Buffer2[length++] = '\r';
		Buffer2[length++] = '\n';
		for(i = length2; i < recvSize; i++)
			Buffer2[length++] = Buffer[i];
		write(((ProxyParam *)lpParameter)->serverSocket, Buffer2, strlen(Buffer2)+ 1);
		recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer2, MAXSIZE, 0);
		if(recvSize <= 0)
			printf("Fail in 349\n");

		char *blank = " ";
		char *Modd = "304";
		//not modified, send original buffer
		if(!memcmp(&Buffer2[9], Modd, strlen(Modd)))
		{
			printf("Reach same buffer\n");
			write(((ProxyParam *)lpParameter)->clientSocket, cache[find].buffer, strlen(cache[find].buffer)+ 1);
	
		}

	}

	//send http to destination server
	write(((ProxyParam *)lpParameter)->serverSocket, Buffer, strlen(Buffer)+ 1);
	recvSize = recv(((ProxyParam*)lpParameter)->serverSocket, Buffer, MAXSIZE, 0);
	printf("Reach 356\n");
	if(recvSize <= 0)
		printf("Fail in 376\n");

	//get time
	char chacheBuff[MAXSIZE];
	memset(&chacheBuff, 0, MAXSIZE);
	memcpy(&chacheBuff, Buffer, MAXSIZE);
	const char *delim = "\r\n";
	char *ptr;
	char dada[DATELENGTH];
	memset(&dada, 0, sizeof(dada));
	char *p = strtok_r(chacheBuff, delim, &ptr);
	bool cun = false;
	while(p)
	{
		//check whether it's modified or not
		if(p[0] == 'L')
		{
			if(strlen(p) > 15)
			{
				char header[15];
				memset(&header, 0, sizeof(header));
				memcpy(&header, p, 14);
				if(!strcmp(header, "Last-Modified:"))
				{
					memcpy(&dada, &p[15], strlen(p));
					cun = true;
					break;
				}
			}
		}
		p = strtok_r(NULL, delim, &ptr);
	}
	if(cun)//if modified
	{
		if(find >= 0)//old cache items need to be changed
		{
			memcpy(&(cache[find].buffer), Buffer, strlen(Buffer));
			memcpy(&(cache[find].date), dada, strlen(dada));
		}
		else//addd new cache
		{
			memcpy(&(cache[__CACHE_number%CACHE_MAXSIZE].htphed.host), httpHeader->host, strlen(httpHeader->host));
			memcpy(&(cache[__CACHE_number%CACHE_MAXSIZE].htphed.method), httpHeader->method, strlen(httpHeader->method));
			memcpy(&(cache[__CACHE_number%CACHE_MAXSIZE].htphed.url), httpHeader->url, strlen(httpHeader->url));
			memcpy(&(cache[__CACHE_number%CACHE_MAXSIZE].buffer), Buffer, strlen(Buffer));
			memcpy(&(cache[__CACHE_number%CACHE_MAXSIZE].date), dada, strlen(dada));
			__CACHE_number++;
		}
	}
	
	write(((ProxyParam*)lpParameter)->clientSocket, Buffer, sizeof(Buffer));//write back to client
	printf("Reach 409\n");

}

int main(int argc, char *argv[])
{
	if(!InitSocket())
	{
		printf("Fail to initialize socket\n");
		return -1;
	}
	printf("Proxy Server listening on Port %d\n", ProxyPort);
	int acceptSocket;
	ProxyParam *lpProxyParam;

	struct sockaddr_in verAddr;
	socklen_t client_addr_len = sizeof(verAddr);
	int *new_sock;

	while(true)
	{
		acceptSocket = accept(ProxyServer, (struct sockaddr *)&verAddr, &client_addr_len);
		if(acceptSocket < 0)
			printf("Fail in 392\n");

		printf("receive client\n");
		lpProxyParam = new ProxyParam;
		if(lpProxyParam == NULL)
			continue;

		lpProxyParam->clientSocket = acceptSocket;

		pthread_t sniffer_thread;
		pthread_create(&sniffer_thread, NULL, ProxyThread, (void*)(lpProxyParam));

		
	} 
	return 0;

}
