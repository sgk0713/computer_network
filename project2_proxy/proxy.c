#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>//시간제어 함수
#include <fcntl.h>//파일 제어함수
#include <arpa/inet.h>//inet_ntoa함수를 쓰기위한 헤더파일

#define MAX_BUFFER_SIZE 50001
#define URL_SIZE 2101
#define METHOD_SIZE 11
#define HTTP_VERSION_SIZE 21
#define HTTP_REQUEST_SIZE 501

void error(char *msg){
    perror(msg);
    exit(1);
}

typedef enum _bool{true, false} bool;
int getPortNum(char* buf);
char *getHost(char* url);
char *getUrlPath(char* url);
bool isRightRequest(char* method, char* url, char* version);
void writeLog(char* ip, char* url, int size);

int main(int argc, char *argv[]){
	int c_sockfd=0, c_newsockfd=0;
	int s_sockfd=0;
	int c_portno=0, pxy_portno=0;
	int n=0;
	ssize_t data_len=0, recved_data_size = 0;
	struct sockaddr_in cli_addr, pxy_addr, serv_addr;
	struct hostent *server;
	socklen_t clilen;

	char buffer[MAX_BUFFER_SIZE]={0};
	char method[METHOD_SIZE]={0}, url[URL_SIZE]={0}, version[HTTP_VERSION_SIZE]={0}, http_request[HTTP_REQUEST_SIZE]={0}, host[URL_SIZE]={0};
	char* c_req_tmp = (char*)malloc(sizeof(char)*MAX_BUFFER_SIZE);
    char* path      = (char*)malloc(sizeof(char)*URL_SIZE);

	if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    c_sockfd = socket(AF_INET, SOCK_STREAM, 0);//사용자와의 소켓
    if (c_sockfd < 0) error("ERROR opening c_socket");

    int opt = 1;
	setsockopt(c_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(int));
    // s_sockfd = socket(AF_INET, SOCK_STREAM, 0);//서버와의 소켓
    // if (s_sockfd < 0) error("ERROR opening s_socket");

    bzero((char *) &pxy_addr, sizeof(pxy_addr));

    pxy_portno = atoi(argv[1]); //atoi converts from String to Integer
    pxy_addr.sin_family = AF_INET;
    pxy_addr.sin_addr.s_addr = INADDR_ANY; //for the server the IP address is always the address that the server is running on
    pxy_addr.sin_port = htons(pxy_portno); //convert from host to network byte order

    if (bind(c_sockfd, (struct sockaddr *) &pxy_addr, sizeof(pxy_addr)) < 0){ //Bind the socket to the server address
        error("ERROR on binding");
    }
    listen(c_sockfd,10); // Listen for socket connections. Backlog queue (connections to wait) is 5
	
	while(1){

		clilen = sizeof(cli_addr);
printf("FIRST OF WHILE=============================\n\n");
	    c_newsockfd = accept(c_sockfd, (struct sockaddr *) &cli_addr, &clilen);//사용자의 요청을 기다림
	    if (c_newsockfd < 0) error("ERROR on accept");


	    bzero(buffer, MAX_BUFFER_SIZE);
	    n = read(c_newsockfd, buffer, MAX_BUFFER_SIZE); //Read is a block function. It will read at most 1024 bytes
	    if (n < 0) error("ERROR reading from socket");


		
	    bzero((char*)c_req_tmp, MAX_BUFFER_SIZE);
	    bzero(method, METHOD_SIZE);
	    bzero(url, URL_SIZE);
	    bzero(version, HTTP_VERSION_SIZE);

		strcpy(c_req_tmp, buffer);
		strtok(c_req_tmp, "\r\n");
		sscanf(c_req_tmp, "%s %s %s", method, url, version);
	    
	    if(isRightRequest(method, url, version) == false){
	    	printf("NO_RIGHT_REQUEST : %s\n", c_req_tmp);
	    	continue;
	    }

	    bzero((char*)host, URL_SIZE);
	    printf("FIRST====buffer====\n%s\n", buffer);
	    bzero((char*)c_req_tmp, MAX_BUFFER_SIZE);
	    strcpy(c_req_tmp, buffer);
		strtok(c_req_tmp, "\r\n");
		strcpy(host, getHost(strtok(NULL, "\r\n")));
		path = getUrlPath(url);
	////////////

	    s_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	    if (s_sockfd < 0) error("ERROR opening s_socket");

	sprintf(http_request, "%s /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", method, path, version, host);
printf("\"%s\" %lu\n", host, strlen(host));
	printf("host = %s\n\nhttp_request=====\n%s\n\n", host, http_request);
printf("check==0\n");
		// struct hostent *server={0};
	    server = gethostbyname(host);
printf("check==01\n");
	    if (server == NULL) {
printf("check==02\n");
	        fprintf(stderr,"ERROR, no such host\n");
	        exit(0);
	    }
printf("check==03\n");
	    c_portno = getPortNum(buffer);
printf("check==04\n");
	    bzero((char *) &serv_addr, sizeof(serv_addr));
printf("check==05\n");	    
	    serv_addr.sin_family = AF_INET; //initialize server's address
printf("check==06\n");
	    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
printf("check==07\n");
	    serv_addr.sin_port = htons(c_portno);
printf("check==08\n");	    

	    if (connect(s_sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) //establish a connection to the server
	        error("ERROR connecting");
printf("check==09\n");
	    if((data_len = send(s_sockfd, http_request, strlen(http_request), 0)) < 0){
	    	error("ERROR writing socket");
printf("check==001\n");
	    }else{
printf("check==002\n");
	    	do{
printf("check==1111111111\n");
	    		bzero(buffer, MAX_BUFFER_SIZE);
	    		data_len = recv(s_sockfd, buffer, MAX_BUFFER_SIZE-1, 0);
	    		recved_data_size += data_len;
	    		if(data_len > 0){
	    			send(c_newsockfd, buffer, data_len, 0);
printf("check==22222\n");
	    		}
	    	}while(data_len > 0);
printf("recved_data_size === %zd\n", recved_data_size);
			writeLog(inet_ntoa(cli_addr.sin_addr), url, recved_data_size);
	    }
	    recved_data_size = 0;
	    close(c_newsockfd);
    	close(s_sockfd);
	}

	free(c_req_tmp);
	free(host);
    free(path);

    close(c_sockfd);
    return 0;
}

int getPortNum(char* buf){
	char tmp[MAX_BUFFER_SIZE]={0};
	char *tmp1=NULL;
	int portnum = 80;
	strcpy(tmp, buf);
	strtok(tmp, " ");
	tmp1 = strtok(NULL, " ");
	if(index(tmp1+7, ':')==NULL){
		return portnum;
	}else{
		strtok(tmp1+7, ":");
		tmp1 = strtok(NULL, "/");
		return portnum = atoi(tmp1);
	}
}

char* getHost(char* secondline){
	char* tmp=0;
	strcat(secondline, "^&");
	strtok(secondline, " ");
	tmp = strtok(NULL, "^&");
	if(index(tmp, ':')==NULL){
		return tmp;
	}else{
		tmp = strtok(tmp, ":");
		return tmp;
	}
}

char* getUrlPath(char* url){
	char tmparr[URL_SIZE];
	strcpy(tmparr, url);
	strcat(tmparr, "^&");
	strtok(tmparr+8, "/");
	char* tmp = strtok(NULL, "^&");
	return tmp==NULL?"": tmp;
}

bool isRightRequest(char* method, char* url, char* version){
	if(!strncmp(method, "GET", 3) && !strncmp(url, "http://", 7) && (!strncmp(version, "HTTP/1.1", 8) || !strncmp(version, "HTTP/1.0", 8)))
		return true;
	else return false;
}

void writeLog(char* ip, char* url, int size){
	int fd;
	time_t     rawtime;
    struct tm *timeinfo;
	char tmp[50], buffer[500];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(tmp, 50, "%a %d %b %Y %X %Z:", timeinfo);
	sprintf(buffer, "%s %s %s %d\n", tmp, ip, url, size);

	if(0<(fd = open("proxy.log", O_WRONLY | O_CREAT | O_APPEND, 0644))){
		write(fd, buffer, strlen(buffer));
		close(fd);
	}else error("ERROR opening PROXY.LOG FILE");
}
