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
int auth_client(int fd, char* contenu_requete);
int verif_user(char* login, char* mdp);
void administration (int fd);
int create_chat(int fd, char* contenu_requete);
int new_chat(char* nom_chat);
int list_chats(int fd, char* contenu_requete);
void send_information (int fd);

struct sockaddr_in svc , clt;
socklen_t cltLen ;

static services mes_services[4] = {
	{"administration", SOCK_ADMIN, 1, -1, "NULL"},
	{"send_info", SOCK_REQ_INFO, 2, -1, "NULL"},
	{"recv_info", SOCK_PUT_INFO, 3, -1, "NULL"},
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

	// Création des sockets d'ecoute du serveurs
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
								// Dialogue avec le client
								printf("********** Ouverture d'une session ADMINISTRATION avec [%s:%d]\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port)); 
					            administration(fd);
					            close(fd);
				                fprintf(stderr, "********** Fermeture de la session ADMINISTRATION avec [%s:%d]\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port));
					            exit(0);
					        }
						break;
						case 2 :
							CHECK(pid = fork(), "can't fork");
							if (pid == 0) {
								// Dialogue avec le client
								printf("********** Ouverture d'une session SEND_INFORMATION avec [%s:%d]\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port)); 
					            send_information(fd);
					            close(fd);
				                fprintf(stderr, "********** Fermeture de la session SEND_INFORMATION avec [%s:%d]\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port));
					            exit(0);
					        }
						break;
						case 3 : 
							CHECK(pid = fork(), "can't fork");
							if (pid == 0) {
								// Dialogue avec le client
					            //auth_client(fd, client_addr);
					            close(fd);
					            exit(0);
					        }
						break;
					}
					close(fd);
				}
			}
		}
	}
	  
	return 0;
}  

void administration (int fd) {
	char* type, buffer[MAX_BUFF], *contenu;
	do {  
		read(fd, buffer, sizeof(buffer));
		fprintf(stderr, "\t[Reçu : %s]\n", buffer);		
		type = strtok(buffer, ":");
		contenu = strtok(NULL, "\n");
		if (!strcmp(type, "AUTH")) {
			auth_client(fd, contenu);
		}
		else if (!strcmp(type, "CREATE")) {
			create_chat(fd, contenu);
		}
		else if (!strcmp(type, "LIST")) {
			list_chats(fd, contenu);
		}
	} while (atoi(buffer) != 0);
}

/*
*	Envoi d'inforamtions aux clients
*/
void send_information (int fd) {
	char* type, buffer[MAX_BUFF], *contenu;

	do {  
		read(fd, buffer, sizeof(buffer));
		fprintf(stderr, "\t[Reçu : %s]\n", buffer);		
		type = strtok(buffer, ":");
		contenu = strtok(NULL, "\n");
		if (!strcmp(type, "LIST")) {
			list_chats(fd, contenu);
		}
	} while (atoi(buffer) != 0);
}

int create_chat(int fd, char* contenu_requete) {
	int token = -1;
	char buffer[MAX_BUFF];

	fprintf(stderr, "Création du chat %s \n", contenu_requete);
	token = new_chat(contenu_requete);
	if (token > 0) {
		sprintf(buffer, "OK:%d", token);
		write(fd , buffer, strlen (buffer)+1); 
		fprintf(stderr, "\t[Envoyé : %s]\n", buffer);
	}
	else {
		sprintf(buffer, "NOK:%d", token);
		write(fd , buffer, strlen (buffer)+1); 
		fprintf(stderr, "\t[Envoyé : %d]\n", buffer);
	}
	return token;
}

int auth_client(int fd, char* contenu_requete) {
	char* login, *mdp;
	int token = -1;
	char buffer[MAX_BUFF];

	login = strtok(contenu_requete, "-");
	mdp = strtok(NULL, "-");
	fprintf(stderr, "\tVérification des identifiants %s - %s\n", login, mdp);
	token = verif_user(login, mdp);
	printf("token : %d\n", token);
	if (token > 0) {
		sprintf(buffer, "OK:%d", token);
		write(fd , buffer, strlen (buffer)+1); 
		fprintf(stderr, "\t[Envoyé : %s]\n", buffer);
	}
	else {
		sprintf(buffer, "NOK:%d", token);
		write(fd , buffer, strlen (buffer)+1); 
		fprintf(stderr, "\t[Envoyé : %d]\n", buffer);
	}
	return token;
}

int list_chats(int fd, char* contenu_requete) {
	FILE* fichier_chats;
	char buffer[MAX_BUFF], buffer2[MAX_BUFF];
    struct chat input;
    // Open person.dat for reading 
    fichier_chats = fopen ("../files/chat_groups.dat", "r"); 
    if (fichier_chats == NULL) 
    { 
        fprintf(stderr, "\nError opening file\n"); 
        return -1;
    }
    // read file contents till end of file 
    int indice = 0;
    while(fread(&input, sizeof(struct chat), 1, fichier_chats)) {
        printf ("id = %d name = %s\n", input.id, input.nom);
        sprintf(buffer2, "%s-", input.nom);
        strcat(buffer, buffer2);
        indice++;
    }
    buffer[strlen(buffer)-1] = '\0'; // On enleve le dernier -
    // close file 
    fclose (fichier_chats);

    // Envoi de la liste à l'utilisateur
	sprintf(buffer2, "OK:%s", buffer);
	printf("envoi :%s\n", buffer2);
	write(fd , buffer2, strlen (buffer2)+1); 
	fprintf(stderr, "\t[Envoyé : %s]\n", buffer2);
	return 1;
}

int new_chat(char* nom_chat) {
	FILE* fd;
    struct chat input; 
    // Open person.dat for reading 
    fd = fopen ("../files/chat_groups.dat", "r"); 
    if (fd == NULL) 
    { 
        fprintf(stderr, "\nError opening file\n"); 
        return -1;
    } 
    // read file contents till end of file 
    int compteur = 0;
    while(fread(&input, sizeof(struct chat), 1, fd)) {
        printf ("id = %d name = %s\n", input.id, input.nom);
        compteur++;
        if(!strcmp(input.nom, nom_chat)) {
        	printf("Le chat existe déjà !\n");
        	return 0;
        }
        if (compteur > NB_MAX_CHATS) {
        	printf("Il y a trop de chats !\n");
        	return 0;
        }
    }
    // close file 
    fclose (fd); 
      
    // open file for writing 
    fd = fopen ("../files/chat_groups.dat", "a"); 
    if (fd == NULL) 
    { 
        fprintf(stderr, "\nError open file\n"); 
        return -1;
    } 
  
    struct chat input1 = {ftell(fd), ""}; 
    strcpy(input1.nom, nom_chat);
      
    // write struct to file 
    fwrite(&input1, sizeof(struct chat), 1, fd); 
      
    if(fwrite != 0)  
        printf("Chat ajouté dans le fichier\n"); 
    else {
        printf("Erreur d'ecriture dans le fichier !\n"); 
        return -1;
    }
  
    // close file 
    fclose (fd);
	return input1.id;
}

int verif_user(char* login, char* mdp) {
	char utilisateurs[NB_MAX_USERS];
	char* line;
	size_t len = 2*STR_SIZE;
	size_t rlen = 0;
	char *test_login, *test_mdp;
	FILE* fd;

	fd = fopen("../files/users_registered.txt", "r");
	printf("traitement du fichier users\n");
	if(fd == NULL) {
		printf("ERREUR d'ouverture du fichier\n");
		return -1;
	}
	while((rlen = getline(&line, &len, fd)) != -1) {
		//passer à la ligne suivante
		test_login = strtok(line, "-");
		test_mdp = strtok(NULL, "\n");
		printf("login : %s, mdp : %s\n", test_login, test_mdp);
		if (!strcmp(login, test_login) && !strcmp(mdp, test_mdp)) {
			return (int) ftell(fd);
		}
	}
	fclose(fd);
	return 0;
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