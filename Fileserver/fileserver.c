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
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>

#define SERVICE_PORT "50000"
#define SERVICE_DIR "/files/"
#define BACKLOG 10
#define SND_BUFFER_SIZE 10240

void sigchild_handler(int sig) {
	while (waitpid(-1, NULL, WNOHANG))
		;
}

void choppy(char *s) {
	s[strcspn(s, "\n")] = '\0';
}

int main(int argc, char **argv) {
	int sockfd, clientfd, addrstatus;
	struct addrinfo helper, *res, *serv;
	struct sockaddr_in client;
	socklen_t client_len;
	FILE *fd;

	DIR* FD;
	struct dirent* in_file;
	char filename[BUFSIZ], cwd[BUFSIZ], snd_buffer[SND_BUFFER_SIZE];
	struct sigaction handler;

	memset(&helper, 0, sizeof helper);
	helper.ai_family = AF_INET;
	helper.ai_socktype = SOCK_STREAM;
	helper.ai_flags = AI_PASSIVE;

	if ((addrstatus = getaddrinfo(NULL, SERVICE_PORT, &helper, &res)) != 0) {
		fprintf(stderr, "[ERROR] getaddrinfo(): %s\n", gai_strerror(addrstatus));
		return 1;
	}

	for (serv = res; serv != NULL ; serv = serv->ai_next) {
		if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
			perror("[ERROR] Unable to acquire socket\n");
			continue;
		}

		int confirm = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &confirm, sizeof(int)) == -1) {
			perror("[ERROR] Unable to set socket options\n");
			exit(1);
		}

		if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
			close(sockfd);
			perror("[ERROR] Unable to bind to socket\n");
			continue;
		}

		break;
	}

	if (serv == NULL ) {
		fprintf(stderr, "[FAILURE] Failed to bind\n");
		return 2;
	}

	freeaddrinfo(res);

	if (listen(sockfd, BACKLOG) == -1) {
		perror("[ERROR] Unable to listen on socket\n");
		exit(2);
	}

	handler.sa_handler = sigchild_handler;
	sigemptyset(&handler.sa_mask);
	handler.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &handler, NULL ) == -1) {
		perror("[ERROR] Signal handler fault\n");
		exit(3);
	}

	printf("[SERVER] Server started and listening on port %s\n", SERVICE_PORT);

	while (1) {
		client_len = sizeof client;
		if ((clientfd = accept(sockfd, (struct sockaddr *) &client, &client_len)) == -1) {
			perror("[ERROR] Unable to accept client connection\n");
			continue;
		}

		printf("[SERVER] New client connected\n");

		if (!fork()) {
			if (getcwd(cwd, sizeof cwd) == NULL ) {
				perror("[ERROR] Executing getcwd() error\n");
			}

			strncat(cwd, SERVICE_DIR, sizeof cwd);

			if (NULL == (FD = opendir(cwd))) {
				fprintf(stderr, "[Error] Failed to open input directory - %s\n", strerror(errno));
			}

			printf("[DEBUG] CWD is %s\n", cwd);

			while ((in_file = readdir(FD))) {
				if (!strcmp(in_file->d_name, "."))
					continue;
				if (!strcmp(in_file->d_name, ".."))
					continue;

				memset(&filename, 0, sizeof filename);
				strncat(filename, "> ", sizeof filename);
				strncat(filename, in_file->d_name, sizeof filename);
				strncat(filename, "\n", sizeof filename);
				send(clientfd, filename, strlen(filename), 0);
			}

			memset(&filename, 0, BUFSIZ);

			if (recv(clientfd, filename, BUFSIZ, 0) < 0) {
				perror("[ERROR] Unable to receive file name\n");
			}

			choppy(filename);
			strncat(cwd, filename, sizeof cwd);

			printf("[REQUEST] Send file: %s\n", filename);

			if ((fd = fopen(cwd, "r")) == NULL ) {
				fprintf(stderr, "[ERROR] Unable to open file \'%s\' :: Reason : %s\n", filename, strerror(errno));
			}

			memset(&snd_buffer, 0, SND_BUFFER_SIZE);

			int sent_block_size;

			while ((sent_block_size = fread(snd_buffer, sizeof(char), SND_BUFFER_SIZE, fd)) > 0) {
				if (send(clientfd, snd_buffer, sent_block_size, 0) < 0) {
					fprintf(stderr, "[ERROR] Unable to read file: %s\n", filename);
				}
				memset(&snd_buffer, 0, SND_BUFFER_SIZE);
			}
			printf("[SERVER] File sent\n");
			fclose(fd);
			close(clientfd);
		}
	}

	return 0;
}
