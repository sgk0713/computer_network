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
#include "lru.h"

#define MAX_BUFFER_SIZE 50001
#define URL_SIZE 2101
#define METHOD_SIZE 11
#define HTTP_VERSION_SIZE 21
#define HTTP_REQUEST_SIZE 501
#define MAX_OBJECT_SIZE 524288


void error(char *msg){
    perror(msg);
    exit(1);
}

typedef enum _bool{true, false} bool;
int getPortNum(char* buf);//포트번호를 처리하는 함수, 유저가 따로 요청하지않았다면 80을 리턴한다
char *getHost(char* url);//url을 통하여 host를 처리하는 함수
char *getUrlPath(char* url);//url을 통하여 경로를 처리하는 함수
bool isRightRequest(char* method, char* url, char* version);//올바른 http요청인지 확인하는 함수
void writeLog(char* ip, char* url, int size);//proxy.log를 생성, 추가하는 함수.

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
	char tmp_data[MAX_OBJECT_SIZE]={0};
	char method[METHOD_SIZE]={0}, url[URL_SIZE]={0}, version[HTTP_VERSION_SIZE]={0}, http_request[HTTP_REQUEST_SIZE]={0}, host[URL_SIZE]={0};
	char* c_req_tmp = (char*)malloc(sizeof(char)*MAX_BUFFER_SIZE);
    char path[URL_SIZE]={0};

//lru q생성
    lruQueue* q = init_queue();
//lru 큐에 담을 node 선언
    node* requestedNode=NULL;

	if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    c_sockfd = socket(AF_INET, SOCK_STREAM, 0);//사용자와의 소켓
    if (c_sockfd < 0) error("ERROR opening c_socket");

    int opt = 1;
	setsockopt(c_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(int));

    bzero((char *) &pxy_addr, sizeof(pxy_addr));
    pxy_portno = atoi(argv[1]); //atoi converts from String to Integer
    pxy_addr.sin_family = AF_INET;
    pxy_addr.sin_addr.s_addr = INADDR_ANY; //for the server the IP address is always the address that the server is running on
    pxy_addr.sin_port = htons(pxy_portno); //convert from host to network byte order

    if (bind(c_sockfd, (struct sockaddr *) &pxy_addr, sizeof(pxy_addr)) < 0){ //Bind the socket to the server address
        error("ERROR on binding");
    }
    listen(c_sockfd, 10);
	
	while(1){
		clilen = sizeof(cli_addr);
//사용자의 요청을 기다린다.
	    c_newsockfd = accept(c_sockfd, (struct sockaddr *) &cli_addr, &clilen);
	    if (c_newsockfd < 0) error("ERROR on accept");

	    bzero(buffer, MAX_BUFFER_SIZE);
	    n = read(c_newsockfd, buffer, MAX_BUFFER_SIZE);
	    if (n < 0) error("ERROR reading from socket");
	    bzero((char*)c_req_tmp, MAX_BUFFER_SIZE);
	    bzero(method, METHOD_SIZE);
	    bzero(url, URL_SIZE);
	    bzero(version, HTTP_VERSION_SIZE);

		strcpy(c_req_tmp, buffer);
		strtok(c_req_tmp, "\r\n");
		sscanf(c_req_tmp, "%s %s %s", method, url, version);
	    
	    if(isRightRequest(method, url, version) == true){
		    bzero((char*)host, URL_SIZE);
		    bzero((char*)c_req_tmp, MAX_BUFFER_SIZE);
		    bzero((char*)path, URL_SIZE);

		    strcpy(c_req_tmp, buffer);
			strtok(c_req_tmp, "\r\n");
//host를 함수로 처리하여 host 변수에 담는다		
			strcpy(host, getHost(strtok(NULL, "\r\n")));
//host 주소를 제외한 경로를 path변수에 담는다.
			strcpy(path, getUrlPath(url));

//LRU 큐에 요청한 데이터가 있는지 확인한다. 있으면 큐에서 가져온 데이터를 전송해준다.
			requestedNode = search(q, url);
			if(requestedNode != NULL){
				printf("\n\n\n------------------------------------------------\n|         H            I             T         |\n------------------------------------------------\n");
				send(c_newsockfd, requestedNode->data, requestedNode->size, 0);
printf("requestedNode->size = %d\n", requestedNode->size);
//log에 작성한다
				writeLog(inet_ntoa(cli_addr.sin_addr), url, getFirst(q)->size);
			}else{
				printf("\n\n\n------------------------------------------------\n|        M         I          S         S      |\n------------------------------------------------\n");
//서버와 통신을 위한 소켓을 생성한다.
		    	s_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		    	if (s_sockfd < 0) error("ERROR opening s_socket");
//서버에게 보낼 request 양식을 만든다.
				sprintf(http_request, "%s /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", method, path, version, host);
		    	server = gethostbyname(host);
		    	if (server == NULL) {
		    	    fprintf(stderr,"ERROR, no such host\n");
		    	    exit(0);
		    	}
//유저가 보낸 url을 기반으로 port번호를 가져온다 default는 80이다.
		    	c_portno = getPortNum(buffer);
		    	bzero((char *) &serv_addr, sizeof(serv_addr));
		    	serv_addr.sin_family = AF_INET; //initialize server's address
		    	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
		    	serv_addr.sin_port = htons(c_portno);
//서버와 connect를 한다
		    	if (connect(s_sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) //establish a connection to the server
			        error("ERROR connecting");
//서버에게 request를 보낸다.
			    if((data_len = send(s_sockfd, http_request, strlen(http_request)+1, 0)) < 0){
			    	error("ERROR writing socket");
			    }else{//서버에 정상적으로 보냈다면 데이터를 받아서 유저에게 넘겨준다.
//서버로부터 받아온 전체데이터를 받아놓기 위해 초기화작업을 한다.
			    	int tmp_idx = 0;
			    	bzero(tmp_data, MAX_OBJECT_SIZE);

//서버에서 데이터를 받아서 유저에게 넘겨준다
			    	do{
			    		bzero(buffer, MAX_BUFFER_SIZE);
			    		data_len = recv(s_sockfd, buffer, MAX_BUFFER_SIZE-1, 0);
			    		recved_data_size += data_len;
printf("buffer = %lu\n", strlen(buffer));
printf("data_len = %lu\n", data_len);
			    		if(data_len > 0){
//유저에게 데이터를 보내준다
		    				send(c_newsockfd, buffer, data_len, 0);
printf("buffer1 = %lu\n", strlen(buffer));
printf("data_len1 = %lu\n", data_len);
//새로운 노드를 만들기 위해 유저에게 보낸 데이터를 tmp_data에 담아둔다. 지정한 object 사이즈보다 크면 넣지않는다.
			    			if(data_len+tmp_idx < MAX_OBJECT_SIZE)
			    				memcpy(tmp_data+tmp_idx, buffer, data_len);

			    			tmp_idx += data_len;
			    		}
		    		}while(data_len > 0);//do-while문
//지정한 Object 사이즈보다 작은 데이터면 LRU큐에 삽입한다.
					if(recved_data_size < MAX_OBJECT_SIZE){
//큐에 남은 공간보다 object사이즈가 클경우 마지막 노드들을 삭제하여 공간을 생성한다.
						while((MAX_CACHE_SIZE - (q->object_size)) < recved_data_size){
							free_node(removeLast(q));
						}
//새로운 노드를 만든다
						node* newNode = node_alloc();
//노드에 데이터를 만들어 입력한다 
						char* data_tmp = (char*)malloc(sizeof(char) * (recved_data_size + 1));
						bzero((char*)data_tmp, sizeof(data_tmp));
printf("strlen(tmp_data) = %lu\n", strlen(tmp_data));
printf("recved_data_size = %lu\n", recved_data_size);
//NULL까지 복사해야된다*********!!!!
						memcpy(data_tmp, tmp_data, recved_data_size);
						newNode->data = data_tmp;

						char* url_tmp = (char*)malloc(sizeof(char) * (strlen(url) + 1)); 
						bzero((char*)url_tmp, sizeof(url_tmp));
						strcpy(url_tmp, url);
			    		newNode->url = url_tmp;

						newNode->size = recved_data_size;
//큐에 삽입
				    	addFirst(q, newNode);
					}
//log에 작성한다
					writeLog(inet_ntoa(cli_addr.sin_addr), url, recved_data_size);
				}
			}//요청했던 request가 아니라면 else문 닫힘
//큐의 상태를 보여준다
			print(q);
//다음 요청에 사용하기 위해 초기화
		    recved_data_size = 0;
		    close(c_newsockfd);
	    	close(s_sockfd);
		}//올바른 http request일 경우 if문 닫힘
	}//while문 닫힘

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
	char tmparr[URL_SIZE]={0};
	strcpy(tmparr, url);
	strcat(tmparr, "^&");
	strtok(tmparr+8, "/");
	char* tmp = strtok(NULL, "^&");
	return tmp==NULL?"": tmp;
}

bool isRightRequest(char* method, char* url, char* version){
	if(!strncmp(method, "GET", 3) && !strncmp(url, "http://", 7) && (!strncmp(version, "HTTP/1.1", 8) || !strncmp(version, "HTTP/1.0", 8)))
		return true;
	return false;
}

void writeLog(char* ip, char* url, int size){
	int fd;
	time_t rawtime;
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
