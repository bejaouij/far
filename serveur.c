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

int main() {
	/* Initialize the server side socket */
	int socketDescriptor = socket(PF_INET, SOCK_STREAM, 0);

	if(socketDescriptor == -1) {
		perror("Socket Creation Error: ");
	}

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(32456);

	if(bind(socketDescriptor, (struct sockaddr*)&address, sizeof(struct sockaddr_in)) == -1) {
		perror("Socket Binding Error: ");
	}
	/********************/

	/* Make server side socket listen connection request */
	if(listen(socketDescriptor, 2) == -1) {
		perror("Socket Linstening Error: ");
	}	
	/********************/

	/* Accept two clients */
	Client* clients[2] = {
		&((Client) { .addressLength = sizeof(struct sockaddr_in) }),
		&((Client) { .addressLength = sizeof(struct sockaddr_in) })
	};

	if((clients[0]->socketDescriptor = accept(socketDescriptor, (struct sockaddr*)&clients[0]->address, &clients[0]->addressLength)) == -1) {
		perror("Socket Accepting Error: ");
	}

	if((clients[1]->socketDescriptor = accept(socketDescriptor, (struct sockaddr*)&clients[1]->address, &clients[1]->addressLength)) == -1) {
		perror("Socket Accepting Error: ");
	}
	/********************/

	/* Make the gateway between both clients to make the communication works */
	char msg[MESSAGE_MAX_LENGTH];
	int recvResult, sendResult;
	int gatewayEstablished = 1;
	int targetIndex = 0;

	while(1) {
		recvResult = recv(clients[targetIndex%2]->socketDescriptor, &msg, sizeof(char)*MESSAGE_MAX_LENGTH, 0);

		if(recvResult == -1) {
			perror("Data Reception Error: ");
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
						perror("Data Sending Error: ");
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

		if(gatewayEstablished == 0) {
			if(close(clients[0]->socketDescriptor) == -1) {
				perror("Client 1 Socket Closing Error: ");
			}

			if(close(clients[1]->socketDescriptor) == -1) {
				perror("Client 2 Socket Closing Error: ");
			}

			if((clients[0]->socketDescriptor = accept(socketDescriptor, (struct sockaddr*)&clients[0]->address, &clients[0]->addressLength)) == -1) {
				perror("Client 1 Socket Accepting Error: ");
			}

			if((clients[1]->socketDescriptor = accept(socketDescriptor, (struct sockaddr*)&clients[1]->address, &clients[1]->addressLength)) == -1) {
				perror("Client 2 Socket Accepting Error: ");
			}

			gatewayEstablished = 1;
			targetIndex = 0;
		}
	}
	/********************/

	/* Close the server side socket */
	if(close(socketDescriptor) == -1) {
		perror("Socket Closing Error: ");
	}
	/********************/

	return 0;
}