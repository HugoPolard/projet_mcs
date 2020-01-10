#include "../libs/streamInc.h"

// Prototypes des fonctions
void deroute();
void dial_clt(int chat_sock, struct sockaddr_in clt);
void create_listening_socket(int* sock, int port);
void init_masque();
void create_listening_socket(int* sock, int port);
void authenticate(char* admin_addr);

// Declaration dse variables globales
struct sockaddr_in svc;
socklen_t cltLen ;
int listen_sock;
utilisateur myProfile;

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
	char admin_addr[20];

	printf("**************** Telegramm2i ****************\n");
	printf("Veuillez renseigner l'adresse de l'administrateur du réseau :\n");
	//scanf("%s", admin_addr);
	strcpy(admin_addr, "127.0.0.1");
	printf("\tAuthentification sur le réseau auprès de %s ...\n", admin_addr);
	printf("login : \n");
	//scanf("%s", myProfile.login);
	strcpy(myProfile.login, "user1");
	printf("mot de passe : \n");
	//scanf("%s", myProfile.passwd);
	strcpy(myProfile.passwd, "mdp1");
	authenticate(admin_addr);
	getchar();

	init_masque();
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

void authenticate(char* admin_addr) {
	int sock ;
	struct sockaddr_in svc;
	char reponse[MAX_BUFF];
	char requete[MAX_BUFF] = "AUTH:";

	// Création de la socket d’appel et de dialogue  
	CHECK(sock =socket(PF_INET, SOCK_STREAM, 0), " Can't create ");  
	// Préparation de l’adressage du service à contacter  
	svc.sin_family = PF_INET;
	svc.sin_port = htons(SOCK_AUTH);
	svc.sin_addr.s_addr = inet_addr(admin_addr);  
	memset(&svc.sin_zero, 0, 8);  
	// Demande d’une connexion au service  
	CHECK(connect (sock, (struct sockaddr*)&svc, sizeof svc), "Can't connect"); // Dialogue avec le serveur 
	// Ajout des infos dans la chaine de requete
	strcat(requete, myProfile.login);
	strcat(requete, "-");
	strcat(requete, myProfile.passwd);
	// Envoi de la requete d'authentification contenant les informations nécessaires au serveur
	fprintf(stderr, "[Envoyé : %s]\n", requete);
	CHECK(write(sock, requete, strlen(requete)+1), "Can't send");
	CHECK(read(sock, reponse,  sizeof (reponse)), "Can't send");
	fprintf(stderr, "[Reçu : %s]\n", reponse);
	getchar();
	close(sock);
}

void dial_clt(int chat_sock, struct sockaddr_in clt) {

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