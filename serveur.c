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

	int bindResult = bind(socketDescriptor, (struct sockaddr*)&address, sizeof(struct sockaddr_in));

	if(bindResult == -1) {
		perror("Socket Binding Error: ");
	}
	/********************/

	/* Make server side socket listen connection request */
	int listenResult = listen(socketDescriptor, 2);

	if(listenResult == -1) {
		perror("Socket Linstening Error: ");
	}	
	/********************/

	/* Accept the first client */
	Client client1;

	client1.addressLength = sizeof(struct sockaddr_in);
	client1.socketDescriptor = accept(socketDescriptor, (struct sockaddr*)&client1.address, &client1.addressLength);

	if(client1.socketDescriptor == -1) {
		perror("Socket Accepting Error: ");
	}
	/********************/

	/* Accept the second client */
	Client client2;

	client2.addressLength = sizeof(struct sockaddr_in);
	client2.socketDescriptor = accept(socketDescriptor, (struct sockaddr*)&client2.address, &client2.addressLength);

	if(client2.socketDescriptor == -1) {
		perror("Socket Accepting Error: ");
	}
	/********************/

	/* Make the gateway between both clients to make the communication works */
	char msg[MESSAGE_MAX_LENGTH];
	int recvResult, sendResult;
	int gatewayEstablished = 1;

	while(gatewayEstablished == 1) {
		recvResult = recv(client1.socketDescriptor, &msg, sizeof(char)*MESSAGE_MAX_LENGTH, 0);

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
					sendResult = send(client2.socketDescriptor, &msg, sizeof(char)*((int)strlen(msg) + 1), 0);

					if(sendResult == -1) {
						perror("Data Sending Error: ");
					}
					else {
						if(sendResult == 0) {
							gatewayEstablished = 0;
						}
						else {
							recvResult = recv(client2.socketDescriptor, &msg, sizeof(char)*MESSAGE_MAX_LENGTH, 0);

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
										sendResult = send(client1.socketDescriptor, &msg, sizeof(char)*((int)strlen(msg) + 1), 0);

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
			if(close(client1.socketDescriptor) == -1) {
				perror("Client 1 Socket Closing Error: ");
			}

			if(close(client2.socketDescriptor) == -1) {
				perror("Client 2 Socket Closing Error: ");
			}

			if((client1.socketDescriptor = accept(socketDescriptor, (struct sockaddr*)&client1.address, &client1.addressLength)) == -1) {
				perror("Client 1 Socket Accepting Error: ");
			}

			if((client2.socketDescriptor = accept(socketDescriptor, (struct sockaddr*)&client2.address, &client2.addressLength)) == -1) {
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
