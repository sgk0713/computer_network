#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

void error(char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]){
    int sockfd, newsockfd; //descriptors rturn from socket and accept system calls
    int portno, filesize, i, sndsize; // port number
    socklen_t clilen;

    FILE* file;
    char buffer[1024];//request header or message
    char* c_req;//file that Client requested
    char* HTTPstate;//for responding state
    char path[1024];//working directory path
    char* c_req_tmp;//using for strtok
    char* data;//data of requested file
    char* extension;//save here extension of file such as pdf, txt, jpg or mp3

    struct sockaddr_in serv_addr, cli_addr;

    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));

    portno = atoi(argv[1]); //atoi converts from String to Integer
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; //for the server the IP address is always the address that the server is running on
    serv_addr.sin_port = htons(portno); //convert from host to network byte order

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ //Bind the socket to the server address
        error("ERROR on binding");
    }
    listen(sockfd,5); // Listen for socket connections. Backlog queue (connections to wait) is 5

    c_req_tmp = (char*)malloc(sizeof(char)*128);
    c_req     = (char*)malloc(sizeof(char)*128);
    HTTPstate = (char*)malloc(sizeof(char)*256);
    extension = (char*)malloc(sizeof(char)*256);

    while(1){
        clilen = sizeof(cli_addr);
printf("waiting.....\n");
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) error("ERROR on accept");

        // bzero(buffer, 1024);
        memset(buffer, 0, 1024);//filling with zero instead of using bzero
        n = read(newsockfd, buffer, 1024); //Read is a block function. It will read at most 1024 bytes
        if (n < 0) error("ERROR reading from socket");

        memset(c_req_tmp, '\0', 128);//filling with zero
        memset(c_req, '\0', 128);//filling with zero
        memset(HTTPstate, '\0', 256);//filling with zero

        strcpy(c_req_tmp, buffer);//copying header information into 'c_req_tmp'
        c_req = strtok(c_req_tmp, " ");
        c_req = strtok(NULL, " ");
        c_req[0] = '\0';
        c_req += 1;//to remove '/' character
        getcwd(path, 1024);//get current working directory

        if(!strcmp(c_req, "favicon.ico")){
            close(newsockfd);//client keeps requesting favicon.ico consistantly. so ignore this request not to show on the screen.
            continue;
        }

        printf("==========================\nHere is the message:\n\nClient's Request File : \"%s\"\nCurrent Working Directory: \"%s\"\n\n%s", c_req, path, buffer);

        if(!strcmp(c_req, "")){//if Client doens't request any file.
            strcpy(HTTPstate, "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n");
            file = fopen("index.html", "rb");//shows index.html to client
        }else{
            if(!(file = fopen(c_req, "rb"))){ // if file doens't exist.(if file exist, the return value will be assaigned.)
                file = fopen("404.html", "rb");//shows 404.html file to client
                strcpy(HTTPstate, "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n");
            }else{//if file exists.
                memset(extension, 0, 256);//filling with zero
                strcpy(extension, c_req);
                extension = strtok(extension, ".");
                extension = strtok(NULL, ".");//figuring extension


                strcpy(HTTPstate, "HTTP/1.1 200 OK\r\n");
                if(extension == NULL){//if extension doesn't exist.
                    strcat(HTTPstate, "Content-type: text/plain\r\n\r\n");
                }else if((!strcmp(extension, "jpg")) || (!strcmp(extension, "jpeg"))){
                    strcat(HTTPstate, "Content-type: image/jpeg\r\n\r\n");
                }else if(!strcmp(extension, "gif")){
                    strcat(HTTPstate, "Content-type: image/gif\r\n\r\n");
                }else if(!strcmp(extension, "png")){
                    strcat(HTTPstate, "Content-type: image/png\r\n\r\n");
                }else if(!strcmp(extension, "wave")){
                    strcat(HTTPstate, "Content-type: audio/wave\r\n\r\n");
                }else if(!strcmp(extension, "wav")){
                    strcat(HTTPstate, "Content-type: audio/wav\r\n\r\n");
                }else if(!strcmp(extension, "mp3")){
                    strcat(HTTPstate, "Content-type: audio/mp3\r\n\r\n");
                }else if(!strcmp(extension, "ogg")){
                    strcat(HTTPstate, "Content-type: application/ogg\r\n\r\n");
                }else if(!strcmp(extension, "pdf")){
                    strcat(HTTPstate, "Content-type: application/pdf\r\n\r\n");
                }else if(!strcmp(extension, "zip")){
                    strcat(HTTPstate, "Content-type: application/zip\r\n\r\n");
                }else if(!strcmp(extension, "avi")){
                    strcat(HTTPstate, "Content-type: video/avi\r\n\r\n");
                }else if(!strcmp(extension, "html")){
                    strcat(HTTPstate, "Content-type: text/html\r\n\r\n");
                }else{//default
                    strcat(HTTPstate, "Content-type: text/plain\r\n\r\n");
                }
            }
        }
        fseek(file, 0, SEEK_END);//putting file pointer at the end
        filesize = ftell(file)+1;//set filesize with the last NULL character
printf("1\n");
        data = (char*)malloc(sizeof(char)*(filesize));
printf("2\n");
        memset(data, '\0', filesize);//filling with zero
        fseek(file, 0, SEEK_SET);//putting file pointer at the beginning
        fread(data, sizeof(char), filesize, file);//put file's data into variable data
        fclose(file);//close the open file

        sndsize = 1024;
printf("3\n");
printf("filesize = %d\n", filesize);
        write(newsockfd, HTTPstate, strlen(HTTPstate));
        for(i = 0; i <= filesize-1/1024; i++) {
printf("\"%d::", i);
            if(i == filesize-1/1024) sndsize = filesize-1%1024;
printf("%d(%d)::", i, sndsize);
            if(write(newsockfd, data+(1024*i), sndsize) < 0){//variable i helps where file pointer to start to write every for loop works
                error("ERROR writing to socket");
            }
printf("%d\" ", i);
        }
printf("4\n");
    	free(data);
printf("5\n");
        close(newsockfd);//close socket
printf("6\n");
    }
    free(c_req);
    free(c_req_tmp);
    free(HTTPstate);
    close(sockfd);
    return 0;
}
