#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
	/* Initialize the client side socket and connect it to the gateway */
        int socketDescriptor = socket(PF_INET, SOCK_STREAM, 0);

        if(socket < 0) {
                perror("Socket Creation Error");
        }

        struct sockaddr_in adServ;
        adServ.sin_family = AF_INET;
        adServ.sin_port = htons(32456);
        
	if(inet_pton(AF_INET, "127.0.0.1", &(adServ.sin_addr)) == -1) {
		perror("Address Conversion Error");
	}

        socklen_t IgA = sizeof(struct sockaddr_in);

       	if(connect(socketDescriptor, (struct sockaddr*) &adServ, IgA) == -1) {
		perror("Connection Error");
	}
	/********************/

	int gatewayEstablished = 1;

	/* Catch the client number */
	int clientIndex;
	int recvRes = recv(socketDescriptor, &clientIndex, 1*sizeof(int), 0);

	if(recvRes == -1) {
		perror("Message Reception Error");
	}
	else {
		if(recvRes == 0) {
			gatewayEstablished = 0;
			printf("CONNECTION LOST\n");
		}
		else {
			printf("CONNECTION ESTABLISHED\n");
		}
	}
	/********************/

	char msg[255];
	int sendRes;
	
	/* If the client is second one of the gateway, first be in a listen
	 * state to be syncronized with the first client */
	if(clientIndex == 2) {
		if((recvRes = recv(socketDescriptor, &msg, 255, 0)) == -1) {
			perror("Message Reception Error");
		}
		else {
			if(recvRes == 0) {
				gatewayEstablished = 0;
				printf("CONNECTION LOST\n");
			}
			else {
				printf("Recipient: %s\n", msg);
			}
		}
	}
	/********************/

	/* While the gateway is established, send and recieve message to the gateway */
	while(gatewayEstablished == 1) {
		printf("You: ");	
		fgets(msg, 255, stdin);
        	*strchr(msg, '\n') = '\0'; /* Remove the end of line character */

		if((sendRes = send(socketDescriptor, &msg, strlen(msg) + 1, 0)) == -1) {
			perror("Message Sending Error");
		}
		else {
			if(sendRes == 0) {
				gatewayEstablished = 0;
				printf("\nCONNECTION LOST\n");
			}
			else {
				if((recvRes = recv(socketDescriptor, &msg, 255*sizeof(char), 0)) == -1) {
					perror("Message Reception Error");
				}
				else {
					if(recvRes == 0) {
						gatewayEstablished = 0;
						printf("CONNECTION LOST\n");
					}
					else {
						printf("Recipient: %s\n", msg);
					}
				}
			}
		}
	}
	/********************/

	/* Close the server side socket */
        if(close(socketDescriptor) == -1) {
		perror("Socket Closing Error");
	}

	printf("CONNECTION CLOSED\n");
	/********************/

        return 0;
}
