/*
 * client.h
 *
 *  Created on: 29 déc. 2022
 *      Author: Florent
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <pthread.h>
#include <stdbool.h>
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




#endif /* CLIENT_H_ */

typedef struct pSession_t {
	int socketCom;
	struct sockaddr_storage info;
} pSession;

typedef struct session_t{
	char clientName[25];
	int clientSocket;
	char ftpName[50];
	int ftpSocket;
	int dataChannelFTP;
	int dataChannelClient;
	boolean connectedToFTP;
	pthread_mutex_t lock;
} session;

//typedef struct pSession_t pSession;
//typedef struct session_t session;


int createClient(int socketCom, struct sockaddr_storage info);
