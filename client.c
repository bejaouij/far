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
#include <dirent.h>

#define MESSAGE_MAX_LENGTH 256
#define FILENAME_MAX_LENGTH 100
#define NICKNAME_MAX_LENGTH 15
#define MAX_FILE_SIZE 1048576

#define UPLOAD_FILES_DIR_PATH "medias/files/upload"
#define DOWNLOAD_FILES_DIR_PATH "medias/files/download"

#define GREEN_TEXT_COLOR_CODE "\x1b[32m"
#define RED_TEXT_COLOR_CODE "\x1b[31m"
#define CYAN_TEXT_COLOR_CODE "\x1b[36m"
#define BRIGHT_BLUE_TEXT_COLOR_CODE "\x1b[94m"
#define RESET_TEXT_COLOR_CODE "\x1b[0m"

typedef struct SendMessagesParams {
	int* socketDescriptor;
	pthread_t* recvMessagesThread;
} SendMessagesParams;

typedef struct FileSendingParams {
	char filename[FILENAME_MAX_LENGTH];
	int socketDescriptor;
} FileSendingParams;

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

/* void -> void
 *
 * Chat command.
 * Display the list of available commands.
 */
void tcmd_help();

/* void -> void
 *
 * Chat command.
 * Show the list of sendable files.
 */
void tcmd_file();

/* &char -> int
 *
 * Chat command.
 * Send the specified file to the gateway.
 *
 * @pre: File must exists in the dedicated directory.
 * @param: - filename: name of the file to send.
 *         - socketDescriptor: server side socket descriptor.
 * @return: - -1 if something goes wrong.
 *          - 0 if everything is okay.
 */
int tcmd_fileSending(const char filename[FILENAME_MAX_LENGTH], int socketDescriptor);

/* &Int -> Any
 *
 * Reception messages thread routine.
 *
 * @param: - socketDescriptor: Descriptor of the server socket.
 */
void* t_recvMessages(int* socketDescriptor);

/* &SendMessagesParams -> Any
 *
 * Sending messages thread routine.
 *
 * @param: params: Structure which store function parameters.
 */
void* t_sendMessages(SendMessagesParams* params);

/* &FileSendingParams -> Any
 *
 * File Sending thread routine.
 *
 * @param: params: Structure which store function parameters.
 */
void* t_fileSending(const FileSendingParams* params);

/* &char -> Any
 *
 * File Reception thread routine.
 *
 * @param: fileBuffer: Array which contain file content..
 */
void* t_fileReception(const char fileBuffer[MAX_FILE_SIZE]);

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
		}
	} while(nicknameFeedback != 0 && nicknameFeedback != 1);
	/********************/

	/* Initialize server communication threads  */
	int resThreadCreation;
	SendMessagesParams sendParams;
	pthread_t thread1;
	pthread_t thread2;

	pthread_t* threads[2] = {
		&thread1,
		&thread2
	};

	sendParams.socketDescriptor = &socketDescriptor;
	sendParams.recvMessagesThread = threads[0];
	/********************/

	if(gatewayEstablished == 1) {
		if((resThreadCreation = pthread_create(threads[0], NULL, (void*) &t_recvMessages, &socketDescriptor)) != 0) {
			perror("Thread Creation Error");
			gatewayEstablished = 0;
		}

		if((resThreadCreation = pthread_create(threads[1], NULL, (void*) &t_sendMessages, &sendParams)) != 0) {
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

		pthread_cancel(*threads[1]);
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

	if((resSend = send(socketDescriptor, &nickname, (strlen(nickname)*sizeof(char) + 1), 0)) == -1) {
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

void tcmd_help() {
	printf("\\help - Display the list of available commands.\n");
	printf("\\stop - Disconnect you from the gateway.\n");
	printf("\\file - Show you the list of sendable files..\n");
	printf("\\file [filename] - Send to the gateway the specified file.\n");
}

void tcmd_file() {
	DIR* uploadDir;
	struct dirent* entryUploadDir;

	if((uploadDir = opendir(UPLOAD_FILES_DIR_PATH)) != NULL) {
		int count = 0;

		printf("Uploadable files:\n");

		while((entryUploadDir = readdir(uploadDir)) != NULL ) {
			count++;

			if(strcmp(entryUploadDir->d_name, ".") != 0 && strcmp(entryUploadDir->d_name, "..")) {
				printf("- %s\n", entryUploadDir->d_name);
			}
		}
		/* If the folder contains only "." and ".." directory, consider it is empty. */
		if(count == 2) {
			printf("Your upload directory is empty.\n");
		}
		/********************/

		closedir(uploadDir);
	}
	else {
		printf(RED_TEXT_COLOR_CODE "Unable to open \"%s\" directory.\n" RESET_TEXT_COLOR_CODE, UPLOAD_FILES_DIR_PATH);
	}
}

int tcmd_fileSending(const char filename[FILENAME_MAX_LENGTH], int socketDescriptor) {
	int fileFound;
	DIR* uploadDir;
	FILE* fileToSend;
	char buffer[MAX_FILE_SIZE];
	struct dirent* entryUploadDir;

	if((uploadDir = opendir(UPLOAD_FILES_DIR_PATH)) != NULL) {
		fileFound = 0;

		while((entryUploadDir = readdir(uploadDir)) != NULL && fileFound == 0) {
			if(strcmp(filename, entryUploadDir->d_name) == 0) {
				fileFound = 1;
			}
		}

		closedir(uploadDir);
	}
	else {
		printf(RED_TEXT_COLOR_CODE "Unable to open \"%s\" directory.\n" RESET_TEXT_COLOR_CODE, UPLOAD_FILES_DIR_PATH);

		return -1;
	}

	if(fileFound == 0) {
		printf(RED_TEXT_COLOR_CODE "The file \"%s/%s\" does not exist.\n" RESET_TEXT_COLOR_CODE, UPLOAD_FILES_DIR_PATH, filename);
		
		return -1;
	}
	else {
		char filepath[strlen(UPLOAD_FILES_DIR_PATH) + strlen(filename) + 1];
		strcpy(filepath, UPLOAD_FILES_DIR_PATH);
		strcat(filepath, "/");
		strcat(filepath, filename);

		fileToSend = fopen(filepath, "r");

		if(fileToSend != NULL) {
			int i = 2;
			int resSend;

			/* Necessary to distinguish the type of the message. */
			buffer[0] = '\\';
			buffer[1] = 'f';
			/********************/

			while((buffer[i] = fgetc(fileToSend)) != EOF) {
				i++;
			}

			if((resSend = send(socketDescriptor, &buffer, (i+1)*sizeof(char), 0)) == -1) {
				perror("File Sending Error");
				
				return -1;
			}
			else {
				if(resSend == 0) {
					return -1;
				}
				else {
					printf(GREEN_TEXT_COLOR_CODE "Attachment successfuly sent.\n" RESET_TEXT_COLOR_CODE);
				}
			}

		}
		else {
			printf(RED_TEXT_COLOR_CODE "Unable to open \"%s/%s\" file.\n" RESET_TEXT_COLOR_CODE, UPLOAD_FILES_DIR_PATH, filename);
			
			return -1;
		}
	}

	return 0;
}

void* t_recvMessages(int* socketDescriptor) {
	int gatewayEstablished = 1;
	int recvRes;
	char receptionType[2];
	char messageBuffer[MESSAGE_MAX_LENGTH];
	char fileBuffer[MAX_FILE_SIZE];
	pthread_t fileReceptionThread;

	while(gatewayEstablished == 1) {
		if((recvRes = recv(*socketDescriptor, &receptionType, sizeof(char)*2, 0)) == -1) {
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
				/* File Reception */
				if(strncmp(receptionType, "\\f", 2) == 0) {
					recv(*socketDescriptor, &fileBuffer, MAX_FILE_SIZE, 0);

					if(pthread_create(&fileReceptionThread, NULL, (void*) &t_fileReception, fileBuffer) != 0) {
						perror("File Reception Thread Creation Error");
					}
				}
				/********************/
				/* Message reception */
				else {
					recv(*socketDescriptor, &messageBuffer, MESSAGE_MAX_LENGTH, 0);
					printf(BRIGHT_BLUE_TEXT_COLOR_CODE "%s\n" RESET_TEXT_COLOR_CODE, receptionType);
				}
				/********************/
			}
		}
	}
}

void* t_sendMessages(SendMessagesParams* params) {
	int gatewayEstablished = 1;
	int sendRes;
	char buffer[MESSAGE_MAX_LENGTH];
	char filename[FILENAME_MAX_LENGTH];
	pthread_t fileSendingThread;
	FileSendingParams fileSendingParams;

	while(gatewayEstablished == 1) {
		fgets(buffer, 255, stdin);
		*strchr(buffer, '\n') = '\0'; /* Remove the end of line character */

		/* If the message is a command */
		if(buffer[0] == '\\') {
			if(strcmp(buffer, "\\help") == 0) {
				tcmd_help();
			}
			else if(strcmp(buffer, "\\stop") == 0) {
				gatewayEstablished = 0;

				if(pthread_cancel(*params->recvMessagesThread) != 0) {
					perror("Thread Cancelling Error");
				}
			}
			else if(strncmp(buffer, "\\file", 5) == 0) {
				/* No contains argument */
				if(strlen(buffer) == 5) {
					tcmd_file();
				}

				/* Potentially contains arguments */
				else {
					int i = 5; /* index in buffer array */
					int j = 0; /* Index in filename array */
					int wholeFilenameFound = 0;

					while(buffer[i] != '\0' && wholeFilenameFound == 0) {
						/* We reached the end of the filename */
						if(buffer[i] == ' ' && j > 0) {
							wholeFilenameFound = 1;
						}
						/********************/
						else {
							if(buffer[i] != ' ' && j < FILENAME_MAX_LENGTH) {
								filename[j] = buffer[i];
								j++;
							}
						}

						i++;
					}

					if(j < FILENAME_MAX_LENGTH) {
						filename[j] = '\0';
					}

					strcpy(fileSendingParams.filename, filename);
					fileSendingParams.socketDescriptor = *params->socketDescriptor;

					if(pthread_create(&fileSendingThread, NULL, (void*) &t_fileSending, &fileSendingParams) != 0) {
						perror("File Sending Thread Creation Error");
					}
				}
				/********************/
			}
			else {
				printf(RED_TEXT_COLOR_CODE "Command \"%s\" does not exist. Type \"\\help\" to access commands list.\n" RESET_TEXT_COLOR_CODE, buffer);
			}
		}
		/********************/
		else {
			printf(CYAN_TEXT_COLOR_CODE "You: %s\n" RESET_TEXT_COLOR_CODE, buffer);

			if((sendRes = send(*params->socketDescriptor, &buffer, sizeof(char)*strlen(buffer) + 1, 0)) == -1) {
				perror("Message Sending Error");
				printf(RED_TEXT_COLOR_CODE "\nCOMMUNICATION LOST\n" RESET_TEXT_COLOR_CODE);
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
}

void* t_fileSending(const FileSendingParams* params) {
	tcmd_fileSending(params->filename, params->socketDescriptor);
}

void* t_fileReception(const char fileBuffer[MAX_FILE_SIZE]) {
	char filepath[strlen(DOWNLOAD_FILES_DIR_PATH) + FILENAME_MAX_LENGTH + 1];
	FILE* fileReception;

	strcpy(filepath, DOWNLOAD_FILES_DIR_PATH);
	strcat(filepath, "/");
	strcat(filepath, "recieve");

	fileReception = fopen(filepath, "w");

	if(fileReception != NULL) {
		int i = 0;

		while(fileBuffer[i] != EOF && i < MAX_FILE_SIZE) {
			fputc(fileBuffer[i], fileReception);
			i++;
		}

		if(i == MAX_FILE_SIZE) {
			i--;
		}

		fputc(EOF, fileReception);

		fclose(fileReception);

		printf(GREEN_TEXT_COLOR_CODE "You recieved an attachment\n" RESET_TEXT_COLOR_CODE);
	}
	else {
		perror("File Reception Opening Error");
	}
}
