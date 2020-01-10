#include "../libs/streamInc.h"
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/select.h>

void talk(int, struct sockaddr_in);
void echo(int, struct sockaddr_in);
void create_listening_socket(int*, int);
void deroute();
int socket_max();

struct sockaddr_in svc , clt;
socklen_t cltLen ;

static services mes_services[3] = {
	{"talk", 6500, 2, -1, "NULL"},
	{"echo", 6501, 1, -1, "NULL"},
	{"NULL", 0, 0, -1, "NULL"}
};

int main() {
	int pid;
	int status, ret;
	int fd_max = 0;

	// Création du masque de signaux
	// on installe le handler deroute
	struct sigaction newact;
	newact.sa_handler = deroute;
	CHECK(sigemptyset(&newact.sa_mask), "problème sigemptyset");
	// Attention dans ce cas à bien mettre SA_RESTART car sinon l'appel accept() est interrompu,
	// ce qui fait que le programme s'arrete
	newact.sa_flags = SA_RESTART;
	CHECK(sigaction(SIGCHLD, &newact, NULL), "problème sigaction");
	CHECK(sigaction(SIGINT, &newact, NULL), "problème sigaction");

	// Création des sockets d'ecoute du serveur
	for (int i = 0; strcmp(mes_services[i].nom, "NULL"); i++) {
		// Création de sockets d'écoutes et enregistrement dans la structure des services
		create_listening_socket(&(mes_services[i].socket), mes_services[i].port);
		// Mémorisions du socket le plus grand
		fd_max = max(fd_max, mes_services[i].socket);
		printf("%s : socket %d\n", mes_services[i].nom, mes_services[i].socket);
	}

	for (;;) {
		int ready;
		ssize_t nbytes;
		fd_set readfds, writefds;
		FD_ZERO(&readfds);	// clear le set readfds
		FD_ZERO(&writefds);	// clear le set readfds
		// On ajoute les sockets de la liste dans le set
		for (int i = 0 ; strcmp(mes_services[i].nom, "NULL") ; i++) {
			FD_SET(mes_services[i].socket, &readfds);
		}

		// Création du select pour surveiller tous les sockets en même temps
		/*
			Problème : Le select est interrompu par le signal SIGCHLD, normalement le flag SA_RESTART permet de palier au
			problème en relançant l'éxecution du programme après traitement du signal, mais ici ça ne marche pas
		*/
		ret = select(fd_max + 1, &readfds, &writefds, NULL, NULL), "select() ";	// Bloque l'execution du programme jusqu'à qu'une connexion arrive
		fd_max++;
		/*
			Pour parer au problème, on fait en sorte que si on sort du select avec une erreur, on ne fait rien et on reboucle directement sur le select,
			ainsi on fait comme si l'appel n'avait pas été interrompu
		*/
		if(ret==-1)
			printf("erreur select\n");
		else {
			for (int i = 0 ; strcmp(mes_services[i].nom, "NULL") ; i++) {
				if (FD_ISSET(mes_services[i].socket, &readfds)) {
					socklen_t addrlen;
					struct sockaddr_in client_addr;
					int fd;

					addrlen = sizeof(client_addr);
					memset(&client_addr, 0, addrlen);
					fd = accept(mes_services[i].socket, (struct sockaddr *) &client_addr, &addrlen);

		           	if (!strcmp(mes_services[i].nom, "echo")) {
		           		CHECK(pid = fork(), "can't fork");
						if (pid == 0) {
							// Dialogue avec le client
				            echo(fd, client_addr);
				            close(fd);
				            fprintf(stderr, "********** Fermeture de la session ECHO avec [%s:%d]\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port));
				            exit(0);
				        }
		           	}
		           	else if (!strcmp(mes_services[i].nom, "talk")) {
		           		CHECK(pid = fork(), "can't fork");
						if (pid == 0) {
							// Dialogue avec le client
				            talk(fd, client_addr);
				            close(fd);
				            fprintf(stderr, "********** Fermeture de la session TALK avec [%s:%d]\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port));
				            exit(0);
				        }
		           	}
					close(fd);
				}
			}
		}
	}
	  
	return 0;
}  

void talk(int sd, struct sockaddr_in clt) {  
	char requete[MAX_BUFF];
	printf("********** Ouverture d'une session TALK avec [%s:%d]\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port));
	do {  
		read(sd, buffer, sizeof(buffer));
		sscanf(buffer, "%s:%s", requete , buffer);
		fprintf(stderr, "\t[Reçu : %s]\n", buffer);
		switch(atoi (requete)) {  
			case 0 :
				fprintf(stderr, "\t[Envoyé : %s]\n", BYE);
			break;  
			case 100 :  
				write(sd , OK, strlen (OK)+1); 
				fprintf(stderr, "\t[Envoyé : OK : message recu %s]\n", buffer);
			break;  
			default :  
				write(sd , NOK,  strlen (NOK)+1);  
				fprintf(stderr, "\t[Envoyé : NOK : message recu %s]\n", buffer);
			break;
		}
	} while (atoi(requete) != 0);
}

void echo(int sd, struct sockaddr_in clt) {
	char requete[MAX_BUFF];
	printf("********** Ouverture d'une session ECHO avec [%s:%d]\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port));
	do {
		read(sd, buffer, sizeof(buffer));
		sscanf(buffer, "%s:%s", requete , buffer);
		fprintf(stderr, "\t[Reçu : %s]\n", buffer);
		sscanf(buffer, "%s:%s", requete , buffer);
		fprintf(stderr, "\t[Reçu : %s]\n", buffer); 
		write(sd , buffer,  strlen (buffer)+1);  
		fprintf(stderr, "\t[Envoyé : %s]\n", buffer);
	} while (atoi(requete) != 0);		
}

void create_listening_socket(int* sock, int port) {
	// Création de la socket de réception d’écoute des appels  
	CHECK(*sock=socket(PF_INET, SOCK_STREAM, 0), "Can't create");  
	// Préparation de l’adressage du service (d’appel)  
	svc.sin_family = PF_INET;
	svc.sin_port = htons(port);  
	svc.sin_addr.s_addr  = INADDR_ANY;  
	memset (&svc.sin_zero, 0, 8);  
	// Association de l’adressage préparé avec la socket d’écoute  
	CHECK(bind (*sock, (struct sockaddr*)&svc , sizeof svc), "Can't bind");
	// Mise en écoute de la socket  
	CHECK(listen(*sock, 5), "Can't calibrate");     
}

void deroute (int signal_number)
{
	int status;
	switch (signal_number) {
		case SIGCHLD :
			wait(&status);
			if (status < 0) {
				fprintf(stderr, "Le processus a fini avec une erreur !!\n");
			}
		break;
		case SIGINT : 
			for (int i = 0 ; strcmp(mes_services[i].nom, "NULL") ; i++) {
				CHECK(close(mes_services[i].socket), "can't close");
			}
			exit(0);
		break;
	}
}