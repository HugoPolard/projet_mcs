#include "../libs/streamInc.h"

// Prototypes des fonctions
void deroute();
void dial_clt(int chat_sock, struct sockaddr_in clt);
void create_listening_socket(int* sock, int port);
void init_masque();
void create_listening_socket(int* sock, int port);
void authenticate();
void command(char input[MAX_BUFF]);
void message(char input[MAX_BUFF]);
void cmd_auth(char* command);
void cmd_create(char* command);
void create_chat(char* chat_name, char* host_address);
void cmd_list(char* command);
void cmd_create(char* command);
void cmd_join(char* command);
void get_host(char* chat_name);
void join_chat();
void communication(int chat_sock);
void administration(int chat_sock, struct sockaddr_in client_addr);
void server_listen();
int add_client(int fd, struct sockaddr_in client_addr);
void rcv_msg_server(int fd);
void rcv_msg_client(int fd);
int send_msg(int socket, char* message);


// Declaration dse variables globales
struct sockaddr_in svc , clt;
socklen_t cltLen ;
int listen_sock;
utilisateur myProfile;
char admin_address[STR_SIZE] = "0.0.0.0";
char host_address[STR_SIZE] = "0.0.0.0";
int nombre_clients = 0;
int chat_clients[NB_MAX_CLIENTS];
int indice_last_client = 0;
int sock_to_host = -1;

static services mes_services[4] = {
	{"rcv_msg_server", SOCK_CHAT, 1, -1, "NULL"},
	{"rcv_msg_client", SOCK_CLIENT, 2, -1, "NULL"},
	{"administration", SOCK_CHAT_ADMIN, 3, -1, "NULL"},
	{"NULL", 0, 0, -1, "NULL"}
};

void init_masque() {
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

int main(int argc, char** argv) {
	int pid;
	char input[MAX_BUFF] = "/auth 127.0.0.1 user1 mdp1";

	init_masque();
	printf("**************** Telegramm2i ****************\n");
	/*
	CHECK(pid = fork(), "can't fork");
	if (pid == 0) {
		// Ecoute en parallèle des ports du serveur
        server_listen();
        exit(0);
    }*/
	
	/*
		Partie pour des tests plus rapides
	*/
	command(input);
	//Fin

	// Boucle permanente de service
	while(1) {
		// Partie client
		printf(">");
		fgets(input, MAX_BUFF, stdin);
		input[strlen(input)-1] = '\0';	// supprime le \n
		if (input[0] == '/') {
			command(input);
		}
		else {
			message(input);
		}
	}
	close(sock_to_host);
	close(listen_sock);
	exit(0);

}

void server_listen() {
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
		if(ret!=-1) {
			for (int i = 0 ; strcmp(mes_services[i].nom, "NULL") ; i++) {
				if (FD_ISSET(mes_services[i].socket, &readfds)) {
					socklen_t addrlen;
					struct sockaddr_in client_addr;
					int fd;

					addrlen = sizeof(client_addr);
					memset(&client_addr, 0, addrlen);
					fd = accept(mes_services[i].socket, (struct sockaddr *) &client_addr, &addrlen);

		           	switch(mes_services[i].type) {
		           		case 1 :
			           		CHECK(pid = fork(), "can't fork");
							if (pid == 0) {
								fprintf(stderr, "********** Ouverture de la session RCV_MSG_SERVER avec [%s:%d]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
								// Dialogue avec le client
					            rcv_msg_server(fd);
					            close(fd);
					            fprintf(stderr, "********** Fermeture de la session RCV_MSG_SERVER avec [%s:%d]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					            exit(0);
					        }
					    break;
					    case 2 :
			           		CHECK(pid = fork(), "can't fork");
							if (pid == 0) {
								// Dialogue avec le client
								fprintf(stderr, "********** Ouverture de la session RCV_MSG_CLIENT avec [%s:%d]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					            rcv_msg_client(fd);
					            close(fd);
					            fprintf(stderr, "********** Fermeture de la session RCV_MSG_CLIENT avec [%s:%d]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					            exit(0);
					        }
					    break;
						case 3 :
			           		CHECK(pid = fork(), "can't fork");
							if (pid == 0) {
								// Dialogue avec le client
								fprintf(stderr, "********** Ouverture de la session ADMINISTRATION avec [%s:%d]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					            administration(fd, client_addr);
					            close(fd);
					            fprintf(stderr, "********** Fermeture de la session ADMINISTRATION avec [%s:%d]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					            exit(0);
					        }
					    break;
						close(fd);
					}
				}
			}
		}
	}
}

void command(char input[MAX_BUFF]) {
	char* command = input+1;	// On enlève le /
	if(strstr(command, "auth")) {
		cmd_auth(command);
	}
	else if(strstr(command, "create")) {
		cmd_create(command);
	}
	else if(strstr(command, "list")) {
		cmd_list(command);
	}
	else if(strstr(command, "join")) {
		cmd_join(command);
	}
}

void message(char input[MAX_BUFF]) {
	if (sock_to_host == -1) {
		printf("Actuellement dans aucun chat, utilisez la commande /join\n");
		return;
	}
	send_msg(sock_to_host, input);
}

void cmd_create(char* command) {
	char chat_name[STR_SIZE];
	char host_address[STR_SIZE];

	strtok(command, " ");	// On ne sauvegarde pas le nom de la commande
	strcpy(chat_name, strtok(NULL, " "));
	strcpy(host_address, strtok(NULL, " "));
	printf("Création du chat %s chez %s\n", chat_name, host_address);
	create_chat(chat_name, host_address);
}

void cmd_auth(char* command) {
	strtok(command, " "); // On ne sauvegarde pas le nom de la commande
	strcpy(admin_address, strtok(NULL, " "));
	printf("admin_address : %s\n", admin_address);
	strcpy(myProfile.login, strtok(NULL, " "));
	strcpy(myProfile.passwd, strtok(NULL, " "));
	printf("\tAuthentification sur le réseau auprès de %s (%s:%s)\n", admin_address, myProfile.login, myProfile.passwd);
	authenticate(admin_address);
}

void cmd_list(char* command) {
	strtok(command, " "); // On ne sauvegarde pas le nom de la  (plus besoin)
	int sock ;
	struct sockaddr_in svc;
	char reponse[MAX_BUFF];
	char requete[MAX_BUFF] = "LIST:";
	char *chats[NB_MAX_CHATS];

	// Création de la socket d’appel et de dialogue  
	CHECK(sock =socket(PF_INET, SOCK_STREAM, 0), " Can't create ");  
	// Préparation de l’adressage du service à contacter  
	svc.sin_family = PF_INET;
	svc.sin_port = htons(SOCK_REQ_INFO);
	svc.sin_addr.s_addr = inet_addr(admin_address);  
	memset(&svc.sin_zero, 0, 8);  
	// Demande d’une connexion au service  
	CHECK(connect (sock, (struct sockaddr*)&svc, sizeof svc), "Can't connect"); // Dialogue avec le serveur 
	// Envoi de la requete de liste des chats au serveur
	fprintf(stderr, "[Envoyé  à %s : %s]\n", admin_address, requete);
	CHECK(write(sock, requete, strlen(requete)+1), "Can't send");
	CHECK(read(sock, reponse,  sizeof (reponse)), "Can't send");
	fprintf(stderr, "[Reçu : %s]\n", reponse);
	close(sock);

	strtok(reponse, ":");
	chats[0] = strtok(NULL, "-");
	for (int i = 1; chats[i] != NULL; i++) {
		chats[i] = strtok(NULL, "-");
	}
	printf("liste des chats :\n");
	for (int i = 0; chats[i] != NULL; i++) {
		printf("\t%s\n", chats[i]);
	}
}

void cmd_join(char* command) {
	char chat_name[STR_SIZE];
	strtok(command, " ");	// On ne sauvegarde pas le nom de la commande
	strcpy(chat_name, strtok(NULL, " "));
	printf("Connexion au chat %s\n", chat_name);
	get_host(chat_name);
	join_chat();
}

void authenticate() {
	int sock ;
	struct sockaddr_in svc;
	char reponse[MAX_BUFF];
	char requete[MAX_BUFF] = "AUTH:";

	// Création de la socket d’appel et de dialogue  
	CHECK(sock =socket(PF_INET, SOCK_STREAM, 0), " Can't create ");  
	// Préparation de l’adressage du service à contacter  
	svc.sin_family = PF_INET;
	svc.sin_port = htons(SOCK_ADMIN);
	svc.sin_addr.s_addr = inet_addr(admin_address);  
	memset(&svc.sin_zero, 0, 8);  
	// Demande d’une connexion au service  
	CHECK(connect (sock, (struct sockaddr*)&svc, sizeof svc), "Can't connect"); // Dialogue avec le serveur 
	// Ajout des infos dans la chaine de requete
	strcat(requete, myProfile.login);
	strcat(requete, "-");
	strcat(requete, myProfile.passwd);
	// Envoi de la requete d'authentification contenant les informations nécessaires au serveur
	fprintf(stderr, "[Envoyé  à %s : %s]\n", admin_address, requete);
	CHECK(write(sock, requete, strlen(requete)+1), "Can't send");
	CHECK(read(sock, reponse,  sizeof (reponse)), "Can't recv");
	fprintf(stderr, "[Reçu : %s]\n", reponse);
	close(sock);
	printf("auth finie\n");
}

void create_chat(char* chat_name, char* host_address) {
	int sock ;
	struct sockaddr_in svc;
	char reponse[MAX_BUFF];
	char requete[MAX_BUFF] = "CREATE:";

	// Création de la socket d’appel et de dialogue  
	CHECK(sock =socket(PF_INET, SOCK_STREAM, 0), " Can't create ");  
	// Préparation de l’adressage du service à contacter  
	svc.sin_family = PF_INET;
	svc.sin_port = htons(SOCK_PUT_INFO);
	svc.sin_addr.s_addr = inet_addr(admin_address);  
	memset(&svc.sin_zero, 0, 8);
	// Demande d’une connexion au service  
	CHECK(connect (sock, (struct sockaddr*)&svc, sizeof svc), "Can't connect"); // Dialogue avec le serveur 
	// Ajout des infos dans la chaine de requete
	strcat(requete, chat_name);
	strcat(requete, "-");
	strcat(requete, host_address);
	// Envoi de la requete d'authentification contenant les informations nécessaires au serveur
	fprintf(stderr, "[Envoyé  à %s : %s]\n", admin_address, requete);
	CHECK(write(sock, requete, strlen(requete)+1), "Can't send");
	CHECK(read(sock, reponse,  sizeof (reponse)), "Can't send");
	fprintf(stderr, "[Reçu : %s]\n", reponse);
	close(sock);
}

void get_host(char* chat_name) {
	int sock ;
	struct sockaddr_in svc;
	char reponse[MAX_BUFF];
	char requete[MAX_BUFF] = "HOST:";

	// Création de la socket d’appel et de dialogue  
	CHECK(sock =socket(PF_INET, SOCK_STREAM, 0), " Can't create ");  
	// Préparation de l’adressage du service à contacter  
	svc.sin_family = PF_INET;
	svc.sin_port = htons(SOCK_REQ_INFO);
	svc.sin_addr.s_addr = inet_addr(admin_address);  
	memset(&svc.sin_zero, 0, 8);
	// Demande d’une connexion au service  
	CHECK(connect (sock, (struct sockaddr*)&svc, sizeof svc), "Can't connect"); // Dialogue avec le serveur 
	// Ajout des infos dans la chaine de requete
	strcat(requete, chat_name);
	// Envoi de la requete d'authentification contenant les informations nécessaires au serveur
	fprintf(stderr, "[Envoyé  à %s : %s]\n", admin_address, requete);
	CHECK(write(sock, requete, strlen(requete)+1), "Can't send");
	CHECK(read(sock, reponse,  sizeof (reponse)), "Can't recv");
	fprintf(stderr, "[Reçu : %s]\n", reponse);
	strtok(reponse, ":");
	strcpy(host_address, strtok(NULL, ":"));
	close(sock);
}

void join_chat() {
	int sock;
	struct sockaddr_in svc;
	char reponse[MAX_BUFF];
	char requete[MAX_BUFF] = "JOIN:";
	char* retour;

	// Création de la socket d’appel et de dialogue  
	CHECK(sock =socket(PF_INET, SOCK_STREAM, 0), " Can't create ");  
	// Préparation de l’adressage du service à contacter  
	svc.sin_family = PF_INET;
	svc.sin_port = htons(SOCK_CHAT_ADMIN);
	svc.sin_addr.s_addr = inet_addr(host_address);  
	memset(&svc.sin_zero, 0, 8);
	//
	printf("Tentative de connexion à %s : %d\n", host_address, SOCK_CHAT);
	// Demande d’une connexion au service  
	CHECK(connect (sock, (struct sockaddr*)&svc, sizeof svc), "Can't connect"); // Dialogue avec le serveur 
	// Envoi de l'acquitement
	fprintf(stderr, "[Envoyé  à %s : %s]\n", admin_address, requete);
	CHECK(write(sock, requete, strlen(requete)+1), "Can't send");
	CHECK(read(sock, reponse,  sizeof (reponse)), "Can't recv");
	fprintf(stderr, "[Reçu : %s]\n", reponse);

	retour = strtok(reponse, ":");
	printf("retour de la demande : %s-\n", retour);
	if(!strcmp(retour, "OK")) {
		svc.sin_port = htons(SOCK_CHAT);
		CHECK(sock_to_host =socket(PF_INET, SOCK_STREAM, 0), " Can't create ");  
		CHECK(connect (sock_to_host, (struct sockaddr*)&svc, sizeof svc), "Can't connect"); // Dialogue avec le serveur 
		printf("socket resultant de la connexion : %d\n", sock_to_host);
		strcpy(requete, "/bonjour");
		fprintf(stderr, "[Envoyé : %s]\n", requete);
		CHECK(write(sock, requete, strlen(requete)+1), "Can't send");
	}
	close(sock);
}

void rcv_msg_client(int fd) {
	do {
		read(fd, buffer, sizeof(buffer));
		printf("-->%s\n", buffer);
	} while (strcmp(buffer, "/fin_chat"));	
}

// Partie serveur : fonctions
void rcv_msg_server(int fd) {
	do {
		read(fd, buffer, sizeof(buffer));
		for (int i = 0 ; i <= indice_last_client ; i++)
			send_msg(chat_clients[i], buffer);
	} while (strcmp(buffer, "/aurevoir"));		
}

int send_msg(int fd, char* message) {
	write(fd , message,  strlen (message)+1);  
}

void administration (int fd, struct sockaddr_in client_addr) {
	char* type, buffer[MAX_BUFF], *contenu;
	int retour = 124;
	do {  
		read(fd, buffer, sizeof(buffer));
		fprintf(stderr, "\t[Reçu : %s]\n", buffer);		
		type = strtok(buffer, ":");
		contenu = strtok(NULL, "\n");
		if (!strcmp(type, "JOIN")) {
			retour = add_client(fd, client_addr);
		}
	} while (atoi(buffer) != 0);
	printf("retour de l'ajout: %d\n", retour);
}

int add_client(int fd, struct sockaddr_in client_addr) {
	FILE* fichier_chats;
	char buffer[MAX_BUFF] = "";
	char* addr;
	int indice_last_client = 0 ;
	printf("ajout de client\n");

    if (nombre_clients > NB_MAX_CLIENTS) {
    	printf("Il y a trop de clients\n");
    	strcpy(buffer, "NOK:");
		printf("envoi :%s\n", buffer);
		write(fd , buffer, strlen (buffer)+1); 
		fprintf(stderr, "\t[Envoyé : %s]\n", buffer);
    	return -1;
    }

    // Renvoi d'un ack
    strcpy(buffer, "OK:");
	printf("envoi :%s\n", buffer);
	write(fd , buffer, strlen (buffer)+1); 
	fprintf(stderr, "\t[Envoyé : %s]\n", buffer);

	int sock_to_client;

	//connexion pour rcv_msg_client
	char reponse[MAX_BUFF], requete[MAX_BUFF];

	// Création de la socket d’appel et de dialogue  
	CHECK(sock_to_client =socket(PF_INET, SOCK_STREAM, 0), " Can't create ");  
	// Préparation de l’adressage du service à contacter  
	client_addr.sin_port = htons(SOCK_CLIENT);
	// Demande d’une connexion au service  
	printf("avant co\n");
	CHECK(connect (sock_to_client, (struct sockaddr*)&svc, sizeof svc), "Can't connect"); // Dialogue avec le serveur 
	printf("apres co\n");
	// Envoi de l'acquitement
	strcpy(requete, "/welcome");
	fprintf(stderr, "[Envoyé  à %s : %s]\n", admin_address, requete);
	CHECK(write(sock_to_client, requete, strlen(requete)+1), "Can't send");
	CHECK(read(sock_to_client, reponse,  sizeof (reponse)), "Can't recv");
	fprintf(stderr, "[Reçu : %s]\n", reponse);

	chat_clients[indice_last_client] = sock_to_client;
	nombre_clients++;
	indice_last_client++;

	return 1;
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
			for (int i = 0 ; i < 4 ; i++) {
				if (mes_services[i].socket != -1)
					CHECK(close(mes_services[i].socket), "can't close");
			}
			close(sock_to_host);
			exit(0);
		break;
	}
}