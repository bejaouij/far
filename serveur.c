#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>

int main() {
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

	int listenResult = listen(socketDescriptor, 2);

	if(listenResult == -1) {
		perror("Socket Linstening Error: ");
	}

	struct sockaddr_in clientAddress;
	socklen_t clientAddressLength = sizeof(struct sockaddr_in);

	int clientSocketDescriptor = accept(socketDescriptor, (struct sockaddr*)&clientAddress, &clientAddressLength);

	if(clientSocketDescriptor == -1) {
		perror("Socket Accepting Error: ");
	}

	int socketClosing = close(socketDescriptor);

	if(socketClosing == -1) {
		perror("Socket Closing Error: ");
	}

	return 0;
}
