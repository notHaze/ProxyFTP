/*
 * client.c
 *
 *  Created on: 29 d�c. 2022
 *      Author: Florent
 */
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
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "client.h"
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données

#include "simpleSocketAPI.h"
#include "dataChannel.h"

void connectToFTP(session* client, char buffer[], int *nbBit);
void* startClient(void*p);
void getCommand(session *client);
void* threadFTP(void* pClient);
bool isPasvResponse(char buffer[]);
bool isPortCommand(char buffer[]);
bool isEndDC(char buffer[]);
bool isOPT(char buffer[]);

int createClient(int socketCom, struct sockaddr_storage info) {

	//Start new Thread
	pSession arg;
	arg.info = info;
	arg.socketCom = socketCom;

	pthread_t id[2];
	pthread_create(&id[0], NULL, (void*)startClient, &arg);



	return 0;
}

void* startClient(void*p) {
	pSession arg = *(pSession*)p;
	session client;
	char clientAddr[MAXHOSTLEN];     // Adresse du serveur
	char clientPort[MAXPORTLEN];     // Port du server
	int ecode;

	client.clientSocket = arg.socketCom;
	client.connectedToFTP=false;
	client.dataChannelFTP=-1;
	client.dataChannelClient=-1;


	arg.info.ss_family = AF_INET;
	ecode = getnameinfo((struct sockaddr*)&(arg.info), sizeof(arg.info), clientAddr,MAXHOSTLEN,
			clientPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
	 if (ecode != 0) {
			 fprintf(stderr, "error in getnameinfo: %s code %d\n", gai_strerror(ecode), ecode);
			 exit(4);
	 }


	 if (pthread_mutex_init(&client.lock, NULL) != 0){
		 perror("mutex init failed\n");
		 exit(8);
	 }


	strcpy(client.clientName, clientAddr);
	strcat(client.clientName,":");
	strcat(client.clientName,clientPort);

	printf("New client %s \n",client.clientName);
	fflush(stdout);

	getCommand(&client);

	shutdown(client.clientSocket, 2);
	close(client.clientSocket);
	if(client.connectedToFTP) {
		shutdown(client.ftpSocket, 2);
		close(client.ftpSocket);
	}
	if(client.dataChannelClient!=-1) {
		shutdown(client.dataChannelClient, 2);
		close(client.dataChannelClient);
	}
	if(client.dataChannelFTP!=-1) {
		shutdown(client.dataChannelFTP, 2);
		close(client.dataChannelFTP);
	}

	pthread_mutex_destroy(&(client.lock));


	pthread_exit((void*)1);
	return NULL;
}


void getCommand(session *client) {

	char bufferInput[MAXBUFFERLEN];
	char input[MAXBUFFERLEN];
	char bufferOutput[MAXBUFFERLEN];
	char username[MAXBUFFERLEN];
	char ftpAddr[MAXBUFFERLEN];
	char affichage[MAXBUFFERLEN];
	int nbBit = 1;
	int nbBitRecv;
	int nbBitFTP;
	//Send first message

	strcpy(bufferOutput, "Connected to PROXY \r\n Login with USERNAME@MACHINE\r\n");
	nbBit = send(client->clientSocket, bufferOutput, strlen(bufferOutput),0);
	memset(bufferOutput, 0, sizeof(bufferOutput));
	//While the connection is active
	while (nbBit != -1 && nbBit != 0) {
		//We wait for a complete input
		nbBitRecv=0;
		memset(input, 0, sizeof(input));
		while(strchr(input,'\n')==NULL && nbBit>0) {
			nbBit = recv(client->clientSocket, bufferInput, sizeof(bufferInput),0);
			strcat(input, bufferInput);
			nbBitRecv += nbBit;
		}
		if(nbBit>0) {
			//If is not connected
			pthread_mutex_lock(&(client->lock));
			if (!client->connectedToFTP) {
				//On regarde si la commande a pour format 'USER ...@...'
				//Si c'est le cas on se connecte au serveur
				if (sscanf(input, "USER %[^@]@%[^\n\r]", username, ftpAddr)==2) {
					strcpy(client->ftpName,ftpAddr);
					pthread_mutex_unlock(&(client->lock));
					connectToFTP(client, bufferOutput, &nbBit);
					sprintf(bufferOutput, "USER %s\r\n",username);
					nbBitFTP = send(client->ftpSocket, bufferOutput, strlen(bufferOutput),0);
					memset(bufferOutput,0,sizeof(bufferOutput));
				} else if (isOPT(input)) {
					input[strlen(input)-2] = '\0';
					sprintf(bufferOutput, "407 %s command cant be send, connect to proxy before\r\n",input);
					nbBit = send(client->clientSocket, bufferOutput, strlen(bufferOutput),0);
				} else {
					//Sinon on demande a ce qu'il se log
					strcpy(bufferOutput, "407 To continue login into an ftp with 'USER USERNAME@FTP'\r\n");
					nbBit = send(client->clientSocket, bufferOutput, strlen(bufferOutput),0);
				}
				memset(affichage,0,MAXBUFFERLEN);
				strcpy(affichage, input);
				affichage[strlen(affichage)-2] = '\0';
				printf("From %s : %s\tnb bits : %d\n",client->clientName, affichage, nbBitRecv);
				fflush(stdout);
			} else {
				//On transmet les commandes au ftp
				memset(affichage,0,MAXBUFFERLEN);
				strcpy(affichage, input);
				affichage[strlen(affichage)-2] = '\0';
				printf("%s to %s : %s\n\t nbBit : %d\n", client->clientName, client->ftpName, affichage,nbBitRecv);
				fflush(stdout);
				if (isPortCommand(input)) {
					connectDataChannelClient(client, input);
				} else {
					nbBitFTP = send(client->ftpSocket, input, strlen(input),0);
				}
			}
			if (nbBitFTP<=0) {
				client->connectedToFTP=false;
			}
			pthread_mutex_unlock(&(client->lock));

			memset(bufferInput, 0, sizeof(bufferInput));
		} else {
			printf("%s disconnected\n",client->clientName);
			fflush(stdout);
		}
	}
}

void connectToFTP(session* client, char buffer[], int *nbBit) {
	int code;
	char *port="21";
	pthread_t id[2];
	client->ftpSocket=-1;
	code = connect2Server(client->ftpName, port, &(client->ftpSocket));
	if(code==0) {

		pthread_mutex_lock(&(client->lock));

		client->connectedToFTP=true;
		/*sprintf(buffer,"Connecte a %s\r\n", client->ftpName);
		*nbBit = send(client->clientSocket, buffer, strlen(buffer),0);*/
		pthread_mutex_unlock(&(client->lock));

		pthread_create(&id[1], NULL, (void*)threadFTP, client);

	} else {
		sprintf(buffer,"Impossible de se connecter au server %s\r\n", client->ftpName);
		*nbBit = send(client->clientSocket, buffer, strlen(buffer),0);
	}
}

void* threadFTP(void* pClient) {
	session *client;
	char buffer[MAXBUFFERLEN];
	char affichage[MAXBUFFERLEN];
	int nbBit = 1;

	client = (session*)pClient;
	while (nbBit>0) {
		memset(buffer, 0, sizeof(buffer));
		nbBit = recv(client->ftpSocket, buffer, sizeof(buffer)-1,0);
		if (isPasvResponse(buffer)) {
			createDataChannelFTP(client, buffer, &nbBit);
			send(client->clientSocket, "200 PORT command successful\r\n", strlen("200 PORT command successful\r\n"),0);
		} else {
			memset(affichage,0,MAXBUFFERLEN);
			strcpy(affichage, buffer);
			affichage[strlen(affichage)-2] = '\0';
			nbBit = send(client->clientSocket, buffer, strlen(buffer), 0);
			printf("%s to %s : %s \n\t nbBit : %d\n", client->ftpName, client->clientName, affichage,nbBit);
			fflush(stdout);
		}
	}
	pthread_mutex_lock(&(client->lock));

	close(client->ftpSocket);
	client->connectedToFTP=false;
	pthread_mutex_unlock(&(client->lock));

	return NULL;
}

bool isPasvResponse(char buffer[]) {
	char model[] = "227 Entering Passive Mode (";
	if(strncmp(model, buffer, strlen(model))==0) {
		return true;
	}
	return false;
}

bool isPortCommand(char buffer[]) {
	char model[] = "PORT ";
	if(strncmp(model, buffer, strlen(model))==0) {
		return true;
	}
	return false;
}

bool isEndDC(char buffer[]) {
	char model[] = "226 ";
	if(strncmp(model, buffer, strlen(model))==0) {
		return true;
	}
	return false;
}

bool isOPT(char buffer[]) {
	char model[] = "OPTS ";
	if(strncmp(model, buffer, strlen(model))==0) {
		return true;
	}
	return false;
}


