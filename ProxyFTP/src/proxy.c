#include  <stdio.h>
#include  <stdlib.h>

#define WIN "win"
#define UNIX "unix"
//Include Socket API for Windows
#ifdef WIN32
	#include <winsock2.h>
	#include <sys/types.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	#define OS WIN
//Include Socket API for Unix
#else
	#include  <sys/socket.h>
	#include  <netdb.h>
	#define OS UNIX
#endif
#include  <unistd.h>
#pragma comment(lib, "Ws2_32.lib")
#include  <string.h>




#include  <stdbool.h>
#include "./simpleSocketAPI.h"
#include "./client.h"



#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port


int main(){
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    int descSockCOM;                 // Descripteur de socket de communication
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les 
				                     // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur
    
    boolean run = true;
    int error;

    printf("START OF PROG\n");
    fflush(stdout);
    if(strcmp(OS,WIN)==0) {
    	WSADATA WSAData;
    	WSAStartup(MAKEWORD(2,0), &WSAData);
    }

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par 
				                      // la fonction getaddrinfo

     // Récupération des informations du serveur
     ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
     if (ecode) {
         fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
         exit(1);
     }
     // Publication de la socket
     ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
     if (ecode == -1) {
         perror("Erreur liaison de la socket de RDV");
         exit(3);
     }
     // Nous n'avons plus besoin de cette liste chainée addrinfo
     freeaddrinfo(res);

     // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
     len=sizeof(struct sockaddr_storage);
     ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
     if (ecode == -1)
     {
         perror("SERVEUR: getsockname");
         exit(4);
     }
     ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, 
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
     if (ecode != 0) {
             fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
             exit(4);
     }
     printf("L'adresse d'ecoute est: %s\n", serverAddr);
     printf("Le port d'ecoute est: %s\n", serverPort);
     fflush(stdout);
     // Definition de la taille du tampon contenant les demandes de connexion
     ecode = listen(descSockRDV, LISTENLEN);
     if (ecode == -1) {
         perror("Erreur initialisation buffer d'écoute");
         exit(5);
     }

	len = sizeof(struct sockaddr_storage);


	//Tant que le serveur run
	while(run) {
		// Attente connexion du client
		// Lorsque demande de connexion, creation d'une socket de communication avec le client
		 descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
		 if (descSockCOM == -1){
			 perror("Erreur accept\n");
			 exit(6);
		 }

		 //Creation du client
		 error = createClient(descSockCOM, from);
		 if (error == -1) {
			 perror("Erreur creattion thread client");
			 exit(7);
		 }
	}


    // Echange de données avec le client connecté

    /*****
     * Testez de mettre 220 devant BLABLABLA ...
     * **/
    //strcpy(buffer, "BLABLABLA\n");
    //write(descSockCOM, buffer, strlen(buffer));

	/*
    int nbBit = 1;
    strcpy(buffer, "Connected to PROXY Login with USER USERNAME@MACHINE");
    nbBit = send(descSockCOM, buffer, strlen(buffer),0);
    memset(buffer, 0, sizeof(buffer));

    while (nbBit != -1 && nbBit != 0) {
    	nbBit = recv(descSockCOM, buffer, sizeof(buffer),0);
    	printf("From telnet : %s\n\terror : %d\n", buffer, nbBit);
    	fflush(stdout);
    }*/


    //Fermeture de la connexion
    //close(descSockCOM);
    close(descSockRDV);
    if (strcmp(OS,WIN)==0) {
    	WSACleanup();
    }
}

