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
void create_chat(char* chat_name);
void cmd_list(char* command);
void cmd_create(char* command);
// Declaration dse variables globales
struct sockaddr_in svc;
socklen_t cltLen ;
int listen_sock;
utilisateur myProfile;
char admin_address[STR_SIZE] = "0.0.0.0";

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
		Partie pour des tests plus rapides
	*/
	command(input);

	while(1) {
		printf(">");
		fgets(input, MAX_BUFF, stdin);
		input[strlen(input)-1] = '\0';	// supprime le \n
		if (input[0] == '/') {
			printf("commande reçue : %s\n", input);
			command(input);
		}
		else {
			printf("message reçu : %s\n", input);
			message(input);
		}
	}

	create_listening_socket(&listen_sock, SOCK_HOST);
	// Boucle permanente de service
	while  (1) {  
		struct sockaddr_in clt;
		cltLen = sizeof (clt);
		//  Attente d’un appel
		int chat_sock;
		CHECK(chat_sock = accept(listen_sock, (struct sockaddr*)&clt, &cltLen), "Can't connect");
		// Création d'un processus fils à chaque connexion
		CHECK(pid = fork(), "can't fork");
		if (pid == 0) {
			// Ferùeture du socket d'écoute pour le client
			close(listen_sock);
			// Dialogue avec le client
            dial_clt(chat_sock, clt);
            close(chat_sock);
            exit(0);
        }
        // Fermeture du socket de traitement pour le père
        close(chat_sock);
	}
	close(listen_sock);
	exit(0);

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
		cmd_list(command);
	}
}

void message(char input[MAX_BUFF]) {
	printf("\n");
}

void cmd_create(char* command) {
	char chat_name[STR_SIZE];
	strtok(command, " ");	// On ne sauvegarde pas le nom de la commande
	strcpy(chat_name, strtok(NULL, " "));
	printf("Création du chat %s\n", chat_name);
	create_chat(chat_name);
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

void cmd_create(char* command) {
	char chat_name[STR_SIZE];
	strtok(command, " ");	// On ne sauvegarde pas le nom de la commande
	strcpy(chat_name, strtok(NULL, " "));
	printf("Création du chat %s\n", chat_name);
	join_chat(chat_name);
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
	CHECK(read(sock, reponse,  sizeof (reponse)), "Can't send");
	fprintf(stderr, "[Reçu : %s]\n", reponse);
	close(sock);
}

void create_chat(char* chat_name) {
	int sock ;
	struct sockaddr_in svc;
	char reponse[MAX_BUFF];
	char requete[MAX_BUFF] = "CREATE:";

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
	strcat(requete, chat_name);
	// Envoi de la requete d'authentification contenant les informations nécessaires au serveur
	fprintf(stderr, "[Envoyé  à %s : %s]\n", admin_address, requete);
	CHECK(write(sock, requete, strlen(requete)+1), "Can't send");
	CHECK(read(sock, reponse,  sizeof (reponse)), "Can't send");
	fprintf(stderr, "[Reçu : %s]\n", reponse);
	close(sock);
}

void join_chat(char* chat_name) {
	int sock ;
	struct sockaddr_in svc;
	char reponse[MAX_BUFF];
	char requete[MAX_BUFF] = "JOIN:";

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
	strcat(requete, chat_name);
	// Envoi de la requete d'authentification contenant les informations nécessaires au serveur
	fprintf(stderr, "[Envoyé  à %s : %s]\n", admin_address, requete);
	CHECK(write(sock, requete, strlen(requete)+1), "Can't send");
	CHECK(read(sock, reponse,  sizeof (reponse)), "Can't send");
	fprintf(stderr, "[Reçu : %s]\n", reponse);
	close(sock);
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
			CHECK(close(listen_sock), "can't close");
			exit(0);
		break;
	}
}