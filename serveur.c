#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <pthread.h>

#define MESSAGE_MAX_LENGTH 255

typedef struct Client {
	int isConnected;
	struct sockaddr_in address;
	socklen_t addressLength;
	int socketDescriptor;
} Client;

typedef struct MessageTransmissionParams {
	int serverSocketDescriptor;
	Client* senderClient;
	Client* recipientClient;
} MessageTransmissionParams;

/* Array(Client*) x int -> int
 * 
 * Init connection with all clients specified in parameters
 *
 * @pre: 'clientsCount' must be equal to the number of clients stored in the array.
 * @post: All client sockets must be connected to the gateway.
 * @param: - clients: Array which contains all clients to disconnect.
 *         - clientsCount: Number of clients stored in 'clients' array.
 *         - gatewaySocketDescriptor: Descriptor of server socket.
 * @return: - 0 if everything is okay
 *          - -1 if something goes wrong
 */
int initConnection(Client* clients[], int clientsCount, int gatewaySocketDescriptor);

/* *MessageTransmissionParams -> Any
 *
 * Transmit message from client 1 to client 2.
 *
 * @param: - params: structure which store all functio parameters.
 * @return: - 0 if everything is okay
 *          - -1 if something goes wrong
 */
void* t_messageTransmission(struct MessageTransmissionParams* params);

int main() {
	/* Initialize the server side socket */
	int socketDescriptor = socket(PF_INET, SOCK_STREAM, 0);

	if(socketDescriptor == -1) {
		perror("Socket Creation Error");
	}

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(32456);

	if(bind(socketDescriptor, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) == -1) {
		perror("Socket Binding Error");
	}
	/********************/

	/* Make server side socket listen connection request */
	if(listen(socketDescriptor, 2) == -1) {
		perror("Socket Linstening Error");
	}	
	/********************/

	/* Initialize two clients */
	Client* clients[2] = {
		&((Client) { .isConnected = 0, .addressLength = sizeof(struct sockaddr_in) }),
		&((Client) { .isConnected = 0, .addressLength = sizeof(struct sockaddr_in) })
	};
	/********************/

	/* Initialize clients communication threads  */
	pthread_t thread1;
	pthread_t thread2;

	pthread_t* threads[2] = {
		&thread1,
		&thread2
	};
	/********************/

	/* Make the gateway between both clients to make the communication works */
	int resThreadCreation;
	int gatewayEstablished;

	while(1) {
		/* Accept two clients */
		gatewayEstablished = 0;

		while(gatewayEstablished == 0) {
			if(initConnection(clients, 2, socketDescriptor) == 0) {
				gatewayEstablished = 1;
			}
		}
		/********************/

		/* Start communication threads routine  */
		resThreadCreation = pthread_create(threads[0], NULL, (void*) &t_messageTransmission, &((MessageTransmissionParams) {
					.serverSocketDescriptor = socketDescriptor,
					.senderClient = clients[0],
					.recipientClient = clients[1]
				}));

		if(resThreadCreation != 0) {
			perror("Thread Creation Error");
		}

		resThreadCreation = pthread_create(threads[1], NULL, (void*) &t_messageTransmission, &((MessageTransmissionParams) {
					.serverSocketDescriptor = socketDescriptor,
					.senderClient = clients[1],
					.recipientClient = clients[0]
				}));

		if(resThreadCreation != 0) {
			perror("Thread Creation Error");
		}
		/********************/

		/* Wait for the communication threds routine end */
		if(pthread_join(*threads[0], 0) != 0) {
			perror("End Thread Error");
		}

		if(pthread_join(*threads[1], 0) != 0) {
			perror("End Thread Error");
		}
		/********************/
	}
	/********************/

	/* Close the server side socket */
	if(close(socketDescriptor) == -1) {
		perror("Socket Closing Error");
	}
	/********************/

	return 0;
}

int initConnection(Client* clients[], int clientsCount, int gatewaySocketDescriptor) {
	int i;

	for(i = 0; i < clientsCount; i++) {
		if((clients[i]->socketDescriptor = accept(gatewaySocketDescriptor, (struct sockaddr*)&(clients[i]->address), &(clients[i]->addressLength))) == -1) {
			perror("Client Socket Accepting Error");
			return -1;
		}
		
		clients[i]->isConnected = 1;
	}

	return 0;
}


void* t_messageTransmission(struct MessageTransmissionParams* params) {
	char msg[MESSAGE_MAX_LENGTH];
	int connectionEstablished = 1;
	int recvRes, sendRes;

	while(connectionEstablished == 1) {
		if((recvRes = recv(params->senderClient->socketDescriptor, &msg, sizeof(char)*MESSAGE_MAX_LENGTH, 0)) == -1) {
			perror("Message Reception Error");
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
					if((sendRes = send(params->recipientClient->socketDescriptor, &msg, sizeof(char)*((int)strlen(msg) + 1), 0)) == -1) {
						perror("Message Sending Error");
					}
					else {
						if(sendRes == 0) {
							connectionEstablished = 0;
						}
					}
				}
			}
		}
	}
	if(params->senderClient->isConnected == 1) {
		if(close(params->senderClient->socketDescriptor) == -1) {
			perror("Client Socket Closing Error");
		}

		params->senderClient->isConnected = 0;
	}

	if(params->recipientClient->isConnected == 1) {
		if(close(params->recipientClient->socketDescriptor) == -1) {
			perror("Client Socket Closing Error");
		}

		params->recipientClient->isConnected = 0;
	}
}
