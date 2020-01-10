#include "../libs/streamInc.h"

void dialogueSrv(int, struct sockaddr_in);

int main(int argc, char** argv) {
	int sock ;
	struct sockaddr_in svc;  
	// Création de la socket d’appel et de dialogue  
	CHECK( sock =socket(PF_INET, SOCK_STREAM, 0), " Can't create ");  
	// Préparation de l’adressage du service à contacter  
	svc.sin_family = PF_INET;
	if (argc == 3) {
		svc.sin_port = htons(atoi(argv[2]));
		svc.sin_addr.s_addr = inet_addr(argv[1]);
	}
	else if (argc == 2) {
		svc.sin_port = htons(atoi(argv[1]));
		svc.sin_addr.s_addr = inet_addr(INADDR_SVC);
	}
	else if (argc == 1) {
		svc.sin_port = htons(PORT_TALK);
		svc.sin_addr.s_addr = inet_addr(INADDR_SVC); 
	}
	else {
		printf("Trop d'arguments\n");
		exit(0);
	} 
	memset(&svc.sin_zero, 0, 8);  
	// Demande d’une connexion au service  
	CHECK(connect (sock, (struct sockaddr*)&svc, sizeof svc), "Can't connect"); // Dialogue avec le serveur  
	dialogueSrv(sock, svc);  
	return 0;  
} 

void dialogueSrv(int sd, struct sockaddr_in srv) {  
	char reponse[MAX_BUFF];  
	// Envoi du message MSG au serveur : la réponse sera OK
	fprintf(stderr, "[Envoyé : %s]\n", MSG);
	CHECK(write(sd, MSG, strlen(MSG)+1), "Can't send");
	CHECK(read(sd, reponse,  sizeof (reponse)), "Can't send");
	fprintf(stderr, "[Reçu : %s]\n", reponse);
	//getchar();
	// Envoi du message ERR au serveur : la réponse sera NOK
	fprintf(stderr, "[Envoyé : %s]\n", ERR);
	CHECK(write(sd, ERR, strlen(ERR)+1), "Can't send"); 
	CHECK(read(sd, reponse,  sizeof(reponse)), "Can't send");
	fprintf(stderr, "[Reçu : %s]\n", reponse);  
	// Envoi du message BYE au serveur : la réponse sera la fin du dialogue
	fprintf(stderr, "[Envoyé : %s]\n", BYE);
	CHECK(write(sd, BYE, strlen (BYE)+1), "Can't send"); 
	close(sd);
} 