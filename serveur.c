#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>

#define MESSAGE_MAX_LENGTH 255

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

	int bindResult = bind(socketDescriptor, (struct sockaddr*)&address, sizeof(struct sockaddr_in));

	if(bindResult == -1) {
		perror("Socket Binding Error: ");
	}
	/********************/

	/* Make server side socket listen connexion request */
	int listenResult = listen(socketDescriptor, 2);

	if(listenResult == -1) {
		perror("Socket Linstening Error: ");
	}	
	/********************/

	/* Accept the first client */
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLength = sizeof(struct sockaddr_in);

	int clientSocketDescriptor = accept(socketDescriptor, (struct sockaddr*)&clientAddress, &clientAddressLength);

	if(clientSocketDescriptor == -1) {
		perror("Socket Accepting Error: ");
	}
	/********************/

	/* Accept the second client */
	struct sockaddr_in clientAddress2;
	socklen_t clientAddressLength2 = sizeof(struct sockaddr_in);

	int clientSocketDescriptor2 = accept(socketDescriptor, (struct sockaddr*)&clientAddress2, &clientAddressLength2);

	if(clientSocketDescriptor2 == -1) {
		perror("Socket Accepting Error: ");
	}
	/********************/

	/* Make the gateway between both clients to make the communication works */
	char msg[MESSAGE_MAX_LENGTH];
	int recvResult, sendResult;
	int gatewayEstablished = 1;

	while(gatewayEstablished == 1) {
		recvResult = recv(clientSocketDescriptor, &msg, sizeof(char)*MESSAGE_MAX_LENGTH, 0);

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
					sendResult = send(clientSocketDescriptor2, &msg, sizeof(char)*((int)strlen(msg) + 1), 0);

					if(sendResult == -1) {
						perror("Data Sending Error: ");
					}
					else {
						if(sendResult == 0) {
							gatewayEstablished = 0;
						}
						else {
							recvResult = recv(clientSocketDescriptor2, &msg, sizeof(char)*MESSAGE_MAX_LENGTH, 0);

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
										sendResult = send(clientSocketDescriptor, &msg, sizeof(char)*((int)strlen(msg) + 1), 0);

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
						}
					}
				}
			}
		}

		if(gatewayEstablished == 0) {
			if(close(clientSocketDescriptor) == -1) {
				perror("Client 1 Socket Closing Error: ");
			}

			if(close(clientSocketDescriptor2) == -1) {
				perror("Client 2 Socket Closing Error: ");
			}

			if((clientSocketDescriptor = accept(socketDescriptor, (struct sockaddr*)&clientAddress, &clientAddressLength)) == -1) {
				perror("Client 1 Socket Accepting Error: ");
			}

			if((clientSocketDescriptor2 = accept(socketDescriptor, (struct sockaddr*)&clientAddress2, &clientAddressLength2)) == -1) {
				perror("Client 2 Socket Accepting Error: ");
			}

			gatewayEstablished = 1;
		}
	}
	/********************/

	/* Close the server side socket */
	int socketClosing = close(socketDescriptor);

	if(socketClosing == -1) {
		perror("Socket Closing Error: ");
	}
	/********************/

	return 0;
}
