#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MESSAGE_MAX_LENGTH 256

/* &Int -> Any
 *
 * Reception messages thread routine.
 *
 * @param: - socketDescriptor: Descriptor of the server socket.
 */
void* t_recvMessages(int* socketDescriptor);

/* &Int -> Any
 *
 * Sending messages thread routine.
 *
 * @param: - socketDescriptor: Descriptor of the server socket.
 */
void* t_sendMessages(int* socketDescriptor);

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

	/* Initialize server communication threads  */
	pthread_t thread1;
	pthread_t thread2;

	pthread_t* threads[2] = {
		&thread1,
		&thread2
	};
	/********************/

	/* While the gateway is established, send and recieve message to the gateway */
	int gatewayEstablished = 1;
	int resThreadCreation;

	if((resThreadCreation = pthread_create(threads[0], NULL, (void*) &t_recvMessages, &socketDescriptor)) != 0) {
		perror("Thread Creation Error");
		gatewayEstablished = 0;
	}

	if((resThreadCreation = pthread_create(threads[1], NULL, (void*) &t_sendMessages, &socketDescriptor)) != 0) {
		perror("Thread Creation Error");
		gatewayEstablished = 0;
	}

	if(gatewayEstablished == 1) {
		/* Stop sending messages routine when the recieve messages
		 * routine is finished */
		printf("COMMUNICATION ESTABLISHED\n");

		if(pthread_join(*threads[0], 0) != 0) {
			perror("End Thread Error");
		}

		if(pthread_cancel(*threads[1]) != 0) {
			perror("Thread Canceling error");
		}
		/********************/
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

void* t_recvMessages(int* socketDescriptor) {
	int gatewayEstablished = 1;
	int recvRes;
	char buffer[MESSAGE_MAX_LENGTH];

	while(gatewayEstablished == 1) {
		if((recvRes = recv(*socketDescriptor, &buffer, sizeof(char)*MESSAGE_MAX_LENGTH, 0)) == -1) {
			perror("Message Reception Error");
			printf("COMMUNICATION LOST");
			gatewayEstablished = 0;
		}
		else {
			if(recvRes == 0) {
				gatewayEstablished = 0;
				printf("\nCOMMUNICATION LOST\n");
			}
			else {
				printf("Recipient: %s\n", buffer);
			}
		}
	}
}

void* t_sendMessages(int* socketDescriptor) {
	int gatewayEstablished = 1;
	int sendRes;
	char buffer[MESSAGE_MAX_LENGTH];

	while(gatewayEstablished == 1) {
		fgets(buffer, 255, stdin);
		*strchr(buffer, '\n') = '\0'; /* Remove the end of line character */
		printf("You: %s\n", buffer);

		if((sendRes = send(*socketDescriptor, &buffer, sizeof(char)*strlen(buffer) + 1, 0)) == -1) {
			perror("Message Sending Error");
			printf("\nCOMMUNICATION LOST\n");
			gatewayEstablished = 0;
		}
		else {
			if(sendRes == 0) {
				gatewayEstablished = 0;
				printf("\nCOMMUNICATION LOST\n");
			}
		}
	}
}
