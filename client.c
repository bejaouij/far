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
#include <regex.h>

#define MESSAGE_MAX_LENGTH 256
#define NICKNAME_MAX_LENGTH 15
/* int -> int
 *
 * Trigger the nickname picking process once.
 *
 * @params: socketDescriptor: server side socket descriptor.
 * @returns: - -1 if the nickname is too long.
 *           - -2 if the nickname has a wrong format.
 *           - -3 if the nickname is already taken in the gateway.
 *           - 1 if something goes wrong.
 *           - 0 if everything is okay.
 */
int nicknamePicking(int socketDescriptor);

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
	int gatewayEstablished = 1;

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

	/* Nickname picking */
	int nicknameFeedback;

	do {
		nicknameFeedback = nicknamePicking(socketDescriptor);

		switch(nicknameFeedback) {
			case 1:
				gatewayEstablished = 0;
				break;
			case -1:
				printf("Your nickname cannot exceed %i characters.\n", NICKNAME_MAX_LENGTH);
				break;
			case -2:
				printf("Your nickname can only contain alphanumeric characters ([a-z | A-Z | 0-9]).\n");
				break;
			case -3:
				printf("This nickname is already taken.\n");
				break;
		}
	} while(nicknameFeedback != 0 && nicknameFeedback != 1);
	/********************/

	/* Initialize server communication threads  */
	int resThreadCreation;
	pthread_t thread1;
	pthread_t thread2;

	pthread_t* threads[2] = {
		&thread1,
		&thread2
	};
	/********************/

	if(gatewayEstablished == 1) {
		if((resThreadCreation = pthread_create(threads[0], NULL, (void*) &t_recvMessages, &socketDescriptor)) != 0) {
			perror("Thread Creation Error");
			gatewayEstablished = 0;
		}

		if((resThreadCreation = pthread_create(threads[1], NULL, (void*) &t_sendMessages, &socketDescriptor)) != 0) {
			perror("Thread Creation Error");
			gatewayEstablished = 0;
		}
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

int nicknamePicking(int socketDescriptor) {
	int resSend, resRecv, nicknameFeedback;
	char nickname[MESSAGE_MAX_LENGTH];
	regex_t alphanumReg;

	printf("Pick a nickname (type \"\\stop\" to cancel):\n");
	fgets(nickname, (MESSAGE_MAX_LENGTH), stdin);
	*strchr(nickname, '\n') = '\0';

	if(strcmp("\\stop", nickname) == 0) {
		return 1;
	}

	if(strlen(nickname) > NICKNAME_MAX_LENGTH) {
		return -1;
	}

	if(regcomp(&alphanumReg, "^[[:alnum:]]+$", REG_EXTENDED) != 0) {
		perror("Regex Compilation Error");
		return 1;
	}

	if(regexec(&alphanumReg, nickname, 0, NULL, 0) == REG_NOMATCH) {
		return -2;
	}

	if((resSend = send(socketDescriptor, &nickname, strlen(nickname)*sizeof(char), 0)) == -1) {
		perror("Nickname Sending Error");
		return 1;
	}
	else {
		if(resSend == 0) {
			return 1;
		}
		else {
			if((resRecv = recv(socketDescriptor, &nicknameFeedback, 1*sizeof(int), 0)) == -1) {
				perror("Nickname Feedback Reception Error");
				return 1;
			}
			else {
				if(resRecv == 0) {
					return 1;
				}
				else {
					return nicknameFeedback;
				}
			}
		}
	}
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
				printf("%s\n", buffer);
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
