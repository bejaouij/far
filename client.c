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

#define MESSAGE_MAX_LENGTH 1024
#define NICKNAME_MAX_LENGTH 15

#define GREEN_TEXT_COLOR_CODE "\x1b[32m"
#define RED_TEXT_COLOR_CODE "\x1b[31m"
#define CYAN_TEXT_COLOR_CODE "\x1b[36m"
#define BRIGHT_BLUE_TEXT_COLOR_CODE "\x1b[94m"
#define RESET_TEXT_COLOR_CODE "\x1b[0m"

/* int -> int
 *
 * Trigger the nickname picking process once.
 *
 * @params: socketDescriptor: server side socket descriptor.
 * @returns: - -1 if the nickname is too long.
 *           - -2 if the nickname has a wrong format.
 *           - -3 if the nickname is already taken in the gateway.
 *           - -4 if the room does not exist.
 *           - -5 if the room is full.
 *           - 2 if nothing has been done.
 *           - 1 if something goes wrong.
 *           - 0 if everything is okay.
 */
int nicknamePicking(int socketDescriptor);

/* void -> void
 *
 * Chat command.
 * Display the list of available commands.
 */
void tcmd_help();

/* &char x &char x int x char[ x int] -> intt
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
				printf(RED_TEXT_COLOR_CODE "Your nickname cannot exceed %i characters.\n" RESET_TEXT_COLOR_CODE, NICKNAME_MAX_LENGTH);
				break;
			case -2:
				printf(RED_TEXT_COLOR_CODE "Your nickname can only contain alphanumeric characters ([a-z | A-Z | 0-9]).\n" RESET_TEXT_COLOR_CODE);
				break;
			case -3:
				printf(RED_TEXT_COLOR_CODE "This nickname is already taken.\n" RESET_TEXT_COLOR_CODE);
				break;
			case -4:
				printf(RED_TEXT_COLOR_CODE "This room does not exist.\n" RESET_TEXT_COLOR_CODE);
				break;
			case -5:
				printf(RED_TEXT_COLOR_CODE "This room is full.\n" RESET_TEXT_COLOR_CODE);
				break;
			case -6:
				printf(RED_TEXT_COLOR_CODE "Invalid command.\n" RESET_TEXT_COLOR_CODE);
				break;
			case -7:
				printf(RED_TEXT_COLOR_CODE "Invalid room specifications.\n" RESET_TEXT_COLOR_CODE);
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
		printf(GREEN_TEXT_COLOR_CODE "COMMUNICATION ESTABLISHED\n" RESET_TEXT_COLOR_CODE);

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

	printf(RED_TEXT_COLOR_CODE "CONNECTION CLOSED\n" RESET_TEXT_COLOR_CODE);
	/********************/

    return 0;
}

void tcmd_help() {
	printf("\\help - Display the list of available commands.\n");
	printf("\\stop - Disconnect you from the gateway.\n");
	printf("\\join [room_index] [nickname] - Try to access a specific room with a nickname.\n");
	printf("\\create_room \"[room_name]\" \"[room_description]\" \"[max_client_number]\" - Create a room with the specified specifications. (max_client_number must be greater than 0)");
	printf("\\rooms - Retrieve the rooms list.");
}

int nicknamePicking(int socketDescriptor) {
	int resSend, resRecv, nicknameFeedback;
	char buffer[MESSAGE_MAX_LENGTH];
	char nickname[NICKNAME_MAX_LENGTH];
	regex_t alphanumReg;

	printf("Join or create a room (type \"\\stop\" to cancel or \"\\help\" to access the commands list):\n");
	fgets(buffer, (MESSAGE_MAX_LENGTH), stdin);
	*strchr(buffer, '\n') = '\0';

	if(strlen(buffer) > MESSAGE_MAX_LENGTH) {
		return -1;
	}

	if(strcmp("\\stop", buffer) == 0) {
		return 1;
	}
	if(strcmp("\\help", buffer) == 0) {
		tcmd_help();

		return 2;
	}
	if(strncmp("\\join", buffer, 5) == 0) {
		if(retrieveArgs(nickname, buffer, 2, ' ') != 0) {
			return -2;
		}
		else {
			/* CHeck the nickname */
			if(regcomp(&alphanumReg, "^[[:alnum:]]+$", REG_EXTENDED) != 0) {
				perror("Regex Compilation Error");
				return 1;
			}

			if(regexec(&alphanumReg, nickname, 0, NULL, 0) == REG_NOMATCH) {
				return -2;
			}
			/********************/
			
			if((resSend = send(socketDescriptor, &buffer, (strlen(buffer)*sizeof(char) + 1), 0)) == -1) {
				perror("Room Pick Sending Error");
				return 1;
			}
			else {
				if(resSend == 0) {
					return 1;
				}
				else {
					if((resRecv = recv(socketDescriptor, &nicknameFeedback, 1*sizeof(int), 0)) == -1) {
						perror("Room Pick Reception Error");
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
	}
	if(strncmp("\\create_room", buffer, 12) == 0) {
		if((resSend = send(socketDescriptor, &buffer, (strlen(buffer)*sizeof(char) + 1), 0)) == -1) {
			perror("Room Pick Sending Error");
			return 1;
		}
		else {
			if(resSend == 0) {
				return 1;
			}
			else {
				if((resRecv = recv(socketDescriptor, &nicknameFeedback, 1*sizeof(int), 0)) == -1) {
					perror("Room Pick Reception Error");
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

		return 2;
	}
	if(strcmp("\\rooms", buffer) == 0) {
		if((resSend = send(socketDescriptor, &buffer, (strlen(buffer)*sizeof(char) + 1), 0)) == -1) {
			perror("Room Pick Sending Error");
			return 1;
		}
		else {
			if(resSend == 0) {
				return 1;
			}
			else {
				if((resRecv = recv(socketDescriptor, &buffer, MESSAGE_MAX_LENGTH*sizeof(char), 0)) == -1) {
					perror("Room Pick Reception Error");
					return 1;
				}
				else {
					if(resRecv == 0) {
						return 1;
					}
					else {
						printf("%s\n", buffer);

						return 2;
					}
				}
			}
		}

		return 2;
	}

	return -6;
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

void* t_recvMessages(int* socketDescriptor) {
	int gatewayEstablished = 1;
	int recvRes;
	char buffer[MESSAGE_MAX_LENGTH];

	while(gatewayEstablished == 1) {
		if((recvRes = recv(*socketDescriptor, &buffer, sizeof(char)*MESSAGE_MAX_LENGTH, 0)) == -1) {
			perror("Message Reception Error");
			printf(RED_TEXT_COLOR_CODE "COMMUNICATION LOST" RESET_TEXT_COLOR_CODE);
			gatewayEstablished = 0;
		}
		else {
			if(recvRes == 0) {
				gatewayEstablished = 0;
				printf(RED_TEXT_COLOR_CODE "\nCOMMUNICATION LOST\n" RESET_TEXT_COLOR_CODE);
			}
			else {
				printf(BRIGHT_BLUE_TEXT_COLOR_CODE "%s\n" RESET_TEXT_COLOR_CODE, buffer);
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
		printf(CYAN_TEXT_COLOR_CODE "You: %s\n" RESET_TEXT_COLOR_CODE, buffer);

		if((sendRes = send(*socketDescriptor, &buffer, sizeof(char)*strlen(buffer) + 1, 0)) == -1) {
			perror(RED_TEXT_COLOR_CODE "Message Sending Error" RESET_TEXT_COLOR_CODE);
			printf("\nCOMMUNICATION LOST\n");
			gatewayEstablished = 0;
		}
		else {
			if(sendRes == 0) {
				gatewayEstablished = 0;
				printf(RED_TEXT_COLOR_CODE "\nCOMMUNICATION LOST\n" RESET_TEXT_COLOR_CODE);
			}
		}
	}
}
