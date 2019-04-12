#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>

#define MESSAGE_MAX_LENGTH 255

typedef struct Client {
	struct sockaddr_in address;
	socklen_t addressLength;
	int socketDescriptor;
} Client;

/* Array(Client*) x int -> int
 * 
 * Close connection with all clients specified in parameters
 *
 * @pre: 'clientsCount' must be equal to the number of clients stored in the array.
 * @post: All client sockets must be closed.
 * @param: - clients: Array which contains all clients to disconnect.
 *         - clientsCount: Number of clients stored in 'clients' array.
 * @return: - 0 if everything is okay
 *          - -1 if something goes wrong
 */
int closeConnection(Client* clients[], int clientsCount);

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

	/* Accept two clients */
	int gatewayEstablished = 0;

	Client* clients[2] = {
		&((Client) { .addressLength = sizeof(struct sockaddr_in) }),
		&((Client) { .addressLength = sizeof(struct sockaddr_in) })
	};

	while(gatewayEstablished == 0) {
		if(initConnection(clients, 2, socketDescriptor) == 0) {
			gatewayEstablished = 1;
		}
	}
	/********************/

	/* Make the gateway between both clients to make the communication works */
	char msg[MESSAGE_MAX_LENGTH];
	int recvResult, sendResult;
	int targetIndex = 0;

	while(1) {
		recvResult = recv(clients[targetIndex%2]->socketDescriptor, &msg, sizeof(char)*MESSAGE_MAX_LENGTH, 0);

		if(recvResult == -1) {
			perror("Data Reception Error");
		}
		else {
			if(recvResult == 0) {
				gatewayEstablished = 0;
			}
			else {
				if(strcmp(msg, "fin") == 0) {
					gatewayEstablished = 0;
				}
				else {
					sendResult = send(clients[(targetIndex + 1)%2]->socketDescriptor, &msg, sizeof(char)*((int)strlen(msg) + 1), 0);

					if(sendResult == -1) {
						perror("Data Sending Error");
					}
					else {
						if(sendResult == 0) {
							gatewayEstablished = 0;
						}
					}
				}
			}
		}

		/* Switch the target */
		targetIndex = (targetIndex + 1)%2;
		/********************/

		while(gatewayEstablished == 0) {
			if(closeConnection(clients, 2) == 0) {
				gatewayEstablished = 1;
			}
			
			if(gatewayEstablished == 1) {
				if(initConnection(clients, 2, socketDescriptor) == 0) {
					gatewayEstablished = 1;
				}
				
				targetIndex = 0;
			}
		}
	}
	/********************/

	/* Close the server side socket */
	if(close(socketDescriptor) == -1) {
		perror("Socket Closing Error");
	}
	/********************/

	return 0;
}

int closeConnection(Client* clients[], int clientsCount) {
	int i;

	for(i = 0; i < clientsCount; i++) {
		if(close(clients[i]->socketDescriptor) == -1) {
			perror("Client Socket Closing Error");
			return -1;
		}
	}

	return 0;
}

int initConnection(Client* clients[], int clientsCount, int gatewaySocketDescriptor) {
	int i;
	int sendResult = 0;

	for(i = 0; i < clientsCount; i++) {
		if((clients[i]->socketDescriptor = accept(gatewaySocketDescriptor, (struct sockaddr*)&(clients[i]->address), &(clients[i]->addressLength))) == -1) {
			perror("Client Socket Accepting Error");
			return -1;
		}
	}

	for(i = 0; i < clientsCount; i++) {
		if((sendResult = send(clients[i]->socketDescriptor, &((int) {(i+1)}), 1*sizeof(int), 0)) == -1) {
			perror("Client Index Sending Error");
			return -1;
		}
		else {
			if(sendResult == 0) {
				return -1;
			}
		}
	}

	return 0;
}
