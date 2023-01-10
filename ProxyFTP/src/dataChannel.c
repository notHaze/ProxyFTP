/*
 * ftp.c
 *
 *  Created on: 30 d�c. 2022
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
void* processDataChannelFTP(session *session);
void createDataChannelFTP(session *session, char buffer[], int *nbBit) {
	char ip[20];
	char port[6];
	char ipNonFormate[4][4];
	int port2[2];
	int code;
	int portFormate;
	sscanf(buffer, "227 Entering Passive Mode (%[^,],%[^,],%[^,],%[^,],%d,%d).", ipNonFormate[0], ipNonFormate[1], ipNonFormate[2], ipNonFormate[3], &port2[0], &port2[1]);
	sprintf(ip,"%s.%s.%s.%s", ipNonFormate[0], ipNonFormate[1], ipNonFormate[2], ipNonFormate[3]);
	portFormate = port2[0]*256;
	portFormate = portFormate + port2[1];
	sprintf(port, "%d", portFormate);
	code = connect2Server(ip, port, &session->dataChannelFTP);
	if (code==0) {
		pthread_t id[2];
		pthread_create(&id[0], NULL, (void*)processDataChannelFTP, session);
	} else {
		sprintf(buffer,"425 Impossible de se connecter au Data Channel de %s\r\n", session->ftpName);
		*nbBit = send(session->clientSocket, buffer, strlen(buffer),0);
	}
}

void* processDataChannelFTP(session *session) {
	char buffer[MAXBUFFERLEN];
	int nbBit=1;
	while (nbBit>0) {
		memset(buffer,0, sizeof(buffer));
		nbBit=recv(session->dataChannelFTP,buffer, sizeof(buffer),0);
		if (nbBit>0){
			send(session->dataChannelClient,buffer, strlen(buffer), 0);
		}
	}
	shutdown(session->dataChannelClient, 2);
	close(session->dataChannelClient);
	session->dataChannelClient=-1;
	close(session->dataChannelFTP);
	session->dataChannelFTP=-1;

	return NULL;
}

int connectDataChannelClient(session *session, char buffer[]) {
	char sendBuffer[MAXBUFFERLEN];
	char ip[20];
	char port[6];
	char ipNonFormate[4][4];
	int port2[2];
	int code;
	int portFormate;

	sscanf(buffer, "PORT %[^,],%[^,],%[^,],%[^,],%d,%d", ipNonFormate[0], ipNonFormate[1], ipNonFormate[2], ipNonFormate[3], &port2[0], &port2[1]);
	sprintf(ip,"%s.%s.%s.%s", ipNonFormate[0], ipNonFormate[1], ipNonFormate[2], ipNonFormate[3]);
	portFormate = port2[0]*256;
	portFormate = portFormate + port2[1];
	sprintf(port, "%d", portFormate);
	code = connect2Server(ip, port, &session->dataChannelClient);
	if(code == 0) {
		memset(sendBuffer,0,sizeof(sendBuffer));
		strcpy(sendBuffer,"PASV\r\n");
		code = send(session->ftpSocket, sendBuffer, strlen(sendBuffer),0);
		return 0;
	} else {
		return -1;
	}
}


