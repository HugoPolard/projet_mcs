#define CHECK(sts,msg) if ((sts) == -1) {perror(msg);exit(-1);}
#define max(x,y) ((x) > (y) ? (x) : (y))
#define suivant(i) (((mes_services[i])==NULL)?0 : (i) +1)

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

#define INADDR_CLT "127.0.0.1"
#define MSG "100:Je dis que \"le fond de l’eau est clair par ici ! Où ça ?\""  
#define BYE "000:Au revoir et à bientôt ..."  
#define ERR "200:Requête ou réponse non reconnue !"  
#define OK "OK"  
#define NOK "Not OK"  
#define MAX_BUFF 1024
#define MAX_MSG 100
#define SOCK_ADMIN 6500
#define SOCK_REQ_INFO 6501
#define SOCK_PUT_INFO 6502
#define SOCK_CLIENT 6600
#define SOCK_CHAT 6700
#define SOCK_CHAT_ADMIN 6701
#define STR_SIZE 50
#define NB_MAX_USERS 30
#define NB_MAX_CHATS 10
#define NB_MAX_CLIENTS 10
#define NB_CHATS 10

char buffer[MAX_BUFF];

typedef struct {
	char nom[50];
	int port;
	int type;
	int socket;
	char commande[100];
} services;

typedef struct {
	char nom[STR_SIZE];
	char login[STR_SIZE];
	char passwd[STR_SIZE];
	int session_token;
} utilisateur;

struct chat{
	int id;
	char nom[STR_SIZE];
	char host[STR_SIZE];
};