#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#define MESSAGE_MAX_LENGTH 255
#define CLIENTS_MAX_COUNT 50

typedef struct Client {
	int isConnected;
	int index;
	struct sockaddr_in address;
	socklen_t addressLength;
	int socketDescriptor;
	pthread_t messagesReceptionThread;
} Client;

typedef struct Gateway {
	int socketDescriptor;
	int clientsCount;
	Client* clients[CLIENTS_MAX_COUNT];
} Gateway;

typedef struct MessageTransmissionParams {
	Client* senderClient;
	Gateway* gateway;
} MessageTransmissionParams;

/* &Client -> int
 *
 * Close specified client connection.
 *
 * @params: client: Client to disconnect.
 * @return: - 0 if everything is okay.
 *          - 1 if the client was already disconnected.
 *          - -1 if something goes wrong.
 */
int closeClientConnection(Client* client);

/* *MessageTransmissionParams -> Any
 *
 * Transmit message from client 1 to client 2.
 *
 * @param: - params: structure which store all function parameters.
 * @return: - 0 if everything is okay
 *          - -1 if something goes wrong
 */
void* t_messageTransmission(struct MessageTransmissionParams* params);

int main() {
	/* Initialize the server side socket */
	Gateway gateway;
	gateway.clientsCount = 0;
	gateway.socketDescriptor = socket(PF_INET, SOCK_STREAM, 0);

	if(gateway.socketDescriptor == -1) {
		perror("Socket Creation Error");
	}

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(32456);

	if(bind(gateway.socketDescriptor, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) == -1) {
		perror("Socket Binding Error");
	}
	/********************/

	/* Make server side socket listen connection request */
	if(listen(gateway.socketDescriptor, 2) == -1) {
		perror("Socket Linstening Error");
	}	
	/********************/

	/* Make the gateway between both clients to make the communication works */
	int resThreadCreation;
	int i;
	MessageTransmissionParams* params;

	while(1) {
		i = gateway.clientsCount;
		gateway.clients[i] = (Client*) malloc(sizeof(Client));
		gateway.clients[i]->isConnected = 0;
		gateway.clients[i]->index = i;
		gateway.clients[i]->addressLength = sizeof(struct sockaddr_in);

		if((gateway.clients[i]->socketDescriptor = accept(gateway.socketDescriptor, (struct sockaddr*)&(gateway.clients[i]->address), &(gateway.clients[i]->addressLength))) == -1) {
			perror("Client Socket Accepting Error");
			return -1;
		}

		/* Start communication threads routine  */
		params = (MessageTransmissionParams*) malloc(sizeof(MessageTransmissionParams));
		params->senderClient = gateway.clients[i];
		params->gateway = &gateway;

		resThreadCreation = pthread_create(&gateway.clients[i]->messagesReceptionThread, NULL, (void*) &t_messageTransmission, params);

		if(resThreadCreation != 0) {
			perror("Thread Creation Error");
		}

		gateway.clients[i]->isConnected = 1;
		gateway.clientsCount++;
		/********************/
	}
	/********************/

	/* Close the server side socket */
	if(close(gateway.socketDescriptor) == -1) {
		perror("Socket Closing Error");
	}
	/********************/

	return 0;
}

int closeClientConnection(Client* client) {
	if(client->isConnected == 1) {
		if(close(client->socketDescriptor) == -1) {
			perror("Client Socket Closing Error");
			return -1;
		}

		client->isConnected = 0;
		free(client);
	}
	else {
		return 1;
	}

	return 0;
}

void* t_messageTransmission(struct MessageTransmissionParams* params) {
	char msg[MESSAGE_MAX_LENGTH];
	int connectionEstablished = 1;
	int recvRes, sendRes;
	int i;

	while(connectionEstablished == 1) {
		if((recvRes = recv(params->senderClient->socketDescriptor, &msg, sizeof(char)*MESSAGE_MAX_LENGTH, 0)) == -1) {
			perror("Message Reception Error");
			connectionEstablished = 0;
		}
		else {
			if(recvRes == 0) {
				connectionEstablished = 0;
			}
			else {
				if(strcmp(msg, "fin") == 0) {
					connectionEstablished = 0;
				}
				else {
					i = 0;

					while(i < (params->gateway->clientsCount)) {
						if(i != params->senderClient->index) {
							if((sendRes = send(params->gateway->clients[i]->socketDescriptor, &msg, sizeof(char)*((int)strlen(msg) + 1), 0)) == -1) {
								perror("Message Sending Error");
							}
							else {
								if(sendRes == 0) {
									connectionEstablished = 0;
								}
							}
						}

						i++;
					}
				}
			}
		}
	}

	while(closeClientConnection(params->senderClient) == -1) {}

	free(params);
}
