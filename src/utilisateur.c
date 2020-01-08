#include "../libs/streamInc.h"

// Prototypes des fonctions
void deroute();
void dial_clt(int chat_sock, struct sockaddr_in clt);
void create_listening_socket(int* sock, int port);

// Declaration dse variables globales
struct sockaddr_in svc;
socklen_t cltLen ;
int listen_sock;

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

	init_masque();
	create_listening_socket(&listen_sock, PORT_LISTEN);
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