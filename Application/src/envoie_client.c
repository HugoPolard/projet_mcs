#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define CHECK(sts,msg) if ((sts) == -1) {perror(msg);exit(-1);}



int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    CHECK(sockfd = socket(AF_INET, SOCK_STREAM, 0),"error opening socket");
  
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    CHECK(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)), "ERROR connecting" ) ;
    while(1)
    {
        bzero(buffer,256);
        fgets(buffer,255,stdin);
       	CHECK( n = write(sockfd,buffer,strlen(buffer)),"ERROR writing to socket");
        bzero(buffer,256);
        CHECK(n = read(sockfd,buffer,255),"ERROR reading from socket");
        printf("%s\n",buffer);
        int i = strncmp("Bye" , buffer , 3);
        if(i == 0)
               break;
    }
    close(sockfd);
    return 0;
}
