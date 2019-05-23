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

#define MESSAGE_MAX_LENGTH 1024
#define CLIENTS_MAX_COUNT 50
#define NICKNAME_MAX_LENGTH 15
#define ROOM_MAX_COUNT 3
#define ROOM_NAME_MAX_LENGTH 30
#define ROOM_DESCRIPTION_MAX_LENGTH 200

typedef struct Client {
	int roomIndex;
	int isConnected;
	int index;
	struct sockaddr_in address;
	socklen_t addressLength;
	int socketDescriptor;
	pthread_t messagesReceptionThread;
	pthread_t nicknamePickingThread;
	char nickname[NICKNAME_MAX_LENGTH];
} Client;

typedef struct Room {
	int maxClientNumber;
	int clientNumber;
	char name[ROOM_NAME_MAX_LENGTH];
	char description[ROOM_DESCRIPTION_MAX_LENGTH];
	Client* clients[CLIENTS_MAX_COUNT]; /* The number will be limited by the stored "maxClientNumber" value */
} Room;

typedef struct Gateway {
	int clientsCount;
	int socketDescriptor;
	int roomNumber;
	Room* rooms[ROOM_MAX_COUNT];
} Gateway;

typedef struct MessageTransmissionParams {
	Client* senderClient;
	Gateway* gateway;
} MessageTransmissionParams, NicknamePickingParams;

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

/* &Gateway x int -> int
 *
 * Remove the client from the gateway at the specified index.
 *
 * @params: - gateway: server inormation container.
 *          - clientIndex: index of the client to remove.
 * @return: - 0 if everything is okay.
 *          - -1 if something goes wrong.
 */
int removeClient(Gateway* gateway, Client* client);

/* &Client x &Gateway -> int
 *
 * Check if the client nickname is available in the gateway.
 *
 * @params: - client: pointer to the client whose nickname is the one to check.
 *          - gateway: pointer to the gateway where check.
 * @returns: - 1 if the nickname is available.
 *           - 0 if the nickname is not available.
 *           - -1 if something goes wrong.
 */
int nicknameAvailable(Client* client, Gateway* gateway);

/* &char x &char x int x char -> int
 *
 * Retrieve and store a specific argument from a string.
 *
 * @params: - argument: variable where store the argument.
 *          - buffer: haystack where search the argument. 
 *          - n: the index of the wanted argument (start at 0).
 *          - separator: the argument separator.
 * @pre: consecutiveSeparator must be equal to 0 or 1.
 * @post: the argument must be stored in the buffer variable if found.
 * @return: - 0 if everything is okay.
 *          - 1 if the argument does not exist.
 *          - -1 if something goes wrong.
 */
int retrieveArgs(char* argument, char* buffer, int n, char separator);

/* &Client x &Room
 *
 * Retrieve and store a specific argument from a string.
 *
 * @params: - argument: variable where store the argument.
 *          - buffer: haystack where search the argument. 
 *          - n: the index of the wanted argument (start at 0).
 *          - separator: the argument separator.
 * @pre: consecutiveSeparator must be equal to 0 or 1.
 * @post: the argument must be stored in the buffer variable if found.
 * @return: - 0 if everything is okay.
 *          - 1 if the argument does not exist.
 *          - -1 if something goes wrong.
 */
int addClientToRoom(Client* client, Room* room);

/* *NicknamePickingParams -> Any
 *
 * Allow user to pick a nickname.
 *
 * @param: - params: structure which store all function parameters.
 * @return: - 0 if everything is okay
 *          - -1 if something goes wrong
 */
void* t_nicknamePicking(NicknamePickingParams* params);

/* *MessageTransmissionParams -> Any
 *
 * Broadcast client messages.
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
	gateway.roomNumber = 0;

	Room room;
	room.maxClientNumber = 10;
	room.clientNumber = 0;
	strcpy(room.name, "Ouistiti");
	strcpy(room.description, "Ce projet me pète les burnes");

	gateway.rooms[0] = &room;
	gateway.roomNumber++;

	Room room2;
	room2.maxClientNumber = 10;
	room2.clientNumber = 0;
	strcpy(room.name, "Ouistitiiiiiiiii");
	strcpy(room.description, "Ce projet me pète les burnes v2");
	
	gateway.rooms[1] = &room2;
	gateway.roomNumber++;

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

	/* Make the gateway between all clients to make the communication works */
	int resThreadCreation;
	NicknamePickingParams* params;

	while(1) {
		if(gateway.clientsCount != CLIENTS_MAX_COUNT) {
			Client* newClient = (Client*) malloc(sizeof(Client));
			newClient->isConnected = 0;
			newClient->addressLength = sizeof(struct sockaddr_in);

			if((newClient->socketDescriptor = accept(gateway.socketDescriptor, (struct sockaddr*)&(newClient->address), &(newClient->addressLength))) == -1) {
				perror("Client Socket Accepting Error");
				return -1;
			}

			/* Start nickname picking thread routine */
			params = (NicknamePickingParams*) malloc(sizeof(NicknamePickingParams));
			params->senderClient = newClient;
			params->gateway = &gateway;

			resThreadCreation = pthread_create(&newClient->nicknamePickingThread, NULL, (void*) &t_nicknamePicking, params);

			if(resThreadCreation != 0) {
				perror("Thread Creation Error");
			}
			/********************/
		}
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
	}
	else {
		return 1;
	}

	return 0;
}

int nicknameAvailable(Client* client, Gateway* gateway) {
	int nicknameFound = 0;
	int i = 0;

	while(i < gateway->rooms[client->roomIndex]->clientNumber && nicknameFound == 0) {
		if(strcmp(gateway->rooms[client->roomIndex]->clients[i]->nickname, client->nickname) == 0) {
			nicknameFound = 1;
		}

		i++;
	}

	if(nicknameFound == 1) {
		return 0;
	}
	else {
		return 1;
	}
}

int removeClient(Gateway* gateway, Client* client) {
	/* Check whether the client index value is in the range */
	if(client->index < 0 || client->index >= gateway->rooms[client->roomIndex]->clientNumber) {
		return -1;
	}
	/********************/

	/* Free the client memory */
	free(gateway->rooms[client->roomIndex]->clients[client->index]);
	/********************/

	/* Remove the client from the array */
	int i;

	for(i = client->index; i < (gateway->rooms[client->roomIndex]->clientNumber - 1); i++) {
		gateway->rooms[client->roomIndex]->clients[i] = gateway->rooms[client->roomIndex]->clients[i + 1];
		gateway->rooms[client->roomIndex]->clients[i]->index = i;
	}

	if(gateway->rooms[client->roomIndex]->clientNumber > 1) {
		gateway->rooms[client->roomIndex]->clientNumber--;
	}
	else {
		/*free(gateway->rooms[client->roomIndex]);*/
		gateway->roomNumber--;
	}
	
	gateway->clientsCount--;
	/********************/

	return 0;
}

int addClientToRoom(Client* client, Room* room) {
	if(room->clientNumber < room->maxClientNumber) {
		room->clients[room->clientNumber] = client;
		client->index = room->clientNumber;
		room->clientNumber++;
	}
	else {
		return 1;
	}

	return 0;
}

int retrieveArgs(char* argument, char* buffer, int n, char separator) {
	int found = 0;
	int argNumber = 0;
	int argIndex = 0;
	int i = 0;

	while(found == 0 && i < strlen(buffer)) {
		if(buffer[i] == separator) {
			if(argNumber == n) {
				found = 1;
			}
			else {
				argNumber++;
				argIndex = 0;
			}
		}
		else {
			argument[argIndex] = buffer[i];
			argIndex++;
		}

		i++;
	}

	if(argNumber == n) {
		found = 1;
	}

	if(found == 1) {
		argument[argIndex] = '\0';
		return 0;
	}
	else {
		return 1;
	}	
}

void* t_nicknamePicking(NicknamePickingParams* params) {
	char buffer[MESSAGE_MAX_LENGTH];
	char nickname[NICKNAME_MAX_LENGTH];
	char roomName[ROOM_NAME_MAX_LENGTH];
	char arg[MESSAGE_MAX_LENGTH];
	int nicknamePicked = 0;
	int gatewayEstablished = 1;
	int nicknameFeedback;
	int resThreadCreation, resRecv, resSend;

	while(nicknamePicked == 0 && gatewayEstablished == 1) {
		if((resRecv = recv(params->senderClient->socketDescriptor, &buffer, sizeof(char)*MESSAGE_MAX_LENGTH, 0)) == -1) {
			perror("Room Pick Reception Error");
			gatewayEstablished = 0;
		}
		else {
			if(resRecv == 0) {
				gatewayEstablished = 0;
			}
			else {
				if(retrieveArgs(arg, buffer, 1, ' ') != 0) {
					nicknameFeedback = -4;
				}
				else {
					if(atoi(arg) < params->gateway->roomNumber) {
						if(params->gateway->rooms[atoi(arg)]->clientNumber >= params->gateway->rooms[atoi(arg)]->maxClientNumber) {
							nicknameFeedback = -5;
						}
						else {
							retrieveArgs(params->senderClient->nickname, buffer, 2, ' ');
							params->senderClient->roomIndex = atoi(arg);

							if(nicknameAvailable(params->senderClient, params->gateway) != 1) {
								nicknameFeedback = -3;
							}
							else {
								if(addClientToRoom(params->senderClient, params->gateway->rooms[atoi(arg)]) == 0) {
									params->gateway->clientsCount++;
									params->senderClient->isConnected = 1;
									nicknameFeedback = 0;
									nicknamePicked = 1;
								}
								else {
									nicknameFeedback = -5;
								}
							}
						}
					}
					else {
						nicknameFeedback = -4;
					}
				}

				resSend = send(params->senderClient->socketDescriptor, &nicknameFeedback, 1*sizeof(int), 0);
			}
		}
	}

	if(nicknamePicked == 1 && gatewayEstablished == 1) {
		/* Start communication threads routine  */
		resThreadCreation = pthread_create(&params->senderClient->messagesReceptionThread, NULL, (void*) &t_messageTransmission, params);

		if(resThreadCreation != 0) {
			perror("Thread Creation Error");
		}

		params->senderClient->isConnected = 1;
		/********************/
	}
	else {
		removeClient(params->gateway, params->senderClient);

		free(params);
	}
}

void* t_messageTransmission(struct MessageTransmissionParams* params) {
	char sendingMsg[MESSAGE_MAX_LENGTH];
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
					strcpy(sendingMsg, params->senderClient->nickname);
					strcat(sendingMsg, ": ");
					strcat(sendingMsg, msg);

					i = 0;

					while(i < (params->gateway->rooms[params->senderClient->roomIndex]->clientNumber)) {
						if(i != params->senderClient->index && params->gateway->rooms[params->senderClient->roomIndex]->clients[i]->isConnected == 1) {
							if((sendRes = send(params->gateway->rooms[params->senderClient->roomIndex]->clients[i]->socketDescriptor, &sendingMsg, sizeof(char)*((int)strlen(sendingMsg) + 1), 0)) == -1) {
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
	while(removeClient(params->gateway, params->senderClient) == -1) {}

	free(params);
}