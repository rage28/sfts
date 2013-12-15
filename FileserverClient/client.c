/*
 * client.c
 *
 *  Created on: 07-Dec-2013
 *      Author: rage
 */
/*
 * fileserver.c
 *
 *  Created on: 07-Dec-2013
 *      Author: rage
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <unistd.h>

#define SERVICE_PORT "50000"
#define SERVICE_DIR "/recv/"
#define SND_BUFFER_SIZE 10240

void choppy(char *s) {
	s[strcspn(s, "\n")] = '\0';
}

int main(int argc, char **argv) {
	int sockfd, addrstatus;
	struct addrinfo helper, *res, *serv;
	char filename[BUFSIZ], buffer[SND_BUFFER_SIZE], cwd[BUFSIZ];

	FILE *fd;
	int recvbytes, write_block_sz;

	if (argc != 2) {
		fprintf(stderr, "Usage: client <Server IP Address>\n");
		exit(1);
	}

	memset(&helper, 0, sizeof helper);
	helper.ai_family = AF_INET;
	helper.ai_socktype = SOCK_STREAM;

	if ((addrstatus = getaddrinfo("127.0.0.1", SERVICE_PORT, &helper, &res)) == -1) {
		fprintf(stderr, "[ERROR] Unable to get socket information: %s\n", gai_strerror(addrstatus));
		return 1;
	}

	for (serv = res; serv != NULL ; serv = serv->ai_next) {
		if ((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) {
			perror("[ERROR] Unable to create scoket\n");
			continue;
		}

		if (connect(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) {
			perror("[ERROR] Unable to connect to server");
			close(sockfd);
			continue;
		}

		break;
	}

	if (serv == NULL ) {
		fprintf(stderr, "[FAILURE] Failed to bind\n");
		return 2;
	}

	freeaddrinfo(res);
	memset(&filename, 0, sizeof filename);

	if ((recvbytes = recv(sockfd, filename, BUFSIZ - 1, 0)) == -1) {
		perror("[ERROR] Unable to receive data from server");
		close(sockfd);
		exit(4);
	}

	filename[recvbytes] = '\0';

	printf("File List:\n");
	printf("%s", filename);
	printf("\n-----------------------\n");
	printf("Enter filename to download: ");

	memset(&filename, 0, sizeof filename);

	if (fgets(filename, BUFSIZ, stdin) != NULL ) {
		choppy(filename);
		send(sockfd, filename, strlen(filename), 0);
	}

	memset(&cwd, 0, sizeof cwd);

	if (getcwd(cwd, sizeof cwd) == NULL ) {
		perror("[ERROR] Executing getcwd() error\n");
	}

	strncat(cwd, SERVICE_DIR, sizeof cwd);
	strncat(cwd, filename, sizeof cwd);

	if ((fd = fopen(cwd, "wb")) == NULL ) {
		fprintf(stderr, "[ERROR] Unable to open file: %s\n", filename);
		exit(1);
	}

	memset(&buffer, 0, SND_BUFFER_SIZE);

	recvbytes = 0;
	while ((recvbytes = recv(sockfd, buffer, SND_BUFFER_SIZE, 0)) > 0) {
		write_block_sz = fwrite(buffer, sizeof(char), recvbytes, fd);

		if (write_block_sz < recvbytes) {
			perror("[ERROR] Unable to write to file\n");
			break;
		}
		memset(&buffer, 0, SND_BUFFER_SIZE);

		if (recvbytes == 0 || recvbytes != SND_BUFFER_SIZE) {
			break;
		}
	}

	printf("\n");
	close(sockfd);

	return 0;
}
