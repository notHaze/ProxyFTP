/*
 * dataChannel.h
 *
 *  Created on: 1 janv. 2023
 *      Author: Florent
 */

#ifndef DATACHANNEL_H_
#define DATACHANNEL_H_

void createDataChannelFTP(session *session, char buffer[], int *nbBit);
int connectDataChannelClient(session *session, char buffer[]);

enum COMMAND {
	RETR, STOR, STOU, APPE, ALLO, REST, RNFR, RNTO, ABOR, DELE, RMD, MKD, LIST, NLST,
}commandDataChannel;

#endif /* DATACHANNEL_H_ */
