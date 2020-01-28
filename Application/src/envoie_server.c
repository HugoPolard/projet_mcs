/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define CHECK(sts,msg) if ((sts) == -1) {perror(msg);exit(-1);}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[255];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     CHECK(sockfd = socket(AF_INET, SOCK_STREAM, 0),"ERROR opening socket"); 

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     CHECK(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) ,"ERROR on binding") 
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     CHECK(newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,  &clilen),"ERROR on accept");
     while(1)
     {
           bzero(buffer,256);
           CHECK(n = read(newsockfd,buffer,255),"ERROR reading from socket");
        
           printf("%s\n",buffer);
           bzero(buffer,256);
           fgets(buffer,255,stdin);
           CHECK(n = write(newsockfd,buffer,strlen(buffer)),"ERROR writing to socket");
           int i=strncmp("Bye" , buffer, 3);
           if(i == 0)
               break;
     }
     close(newsockfd);
     close(sockfd);
     return 0; 
}
