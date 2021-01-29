#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <sys/select.h>
#include <assert.h>
#include "shared.h"

void usage(const char * p) {
    fprintf(stderr, "%s -p port\n", p);
}

void *echo_worker(void *pfd) {
	int fd = *((int *) pfd);
	int ret;
	char * client_payload;
	int client_payload_length = strlen(PAYLOAD);

	client_payload = (char *) malloc(client_payload_length + 1);

	while(1) {
		// read data
        ret = read(fd, client_payload, client_payload_length);
        if (ret < 0) {
			printf("[%d] ", fd);
            perror("read");
            break;
        } else if (ret == 0) {
			break;
		} else if (ret != client_payload_length) {
            fprintf(stderr, "failure reading data from connection, response size does not match.\n");
            break;
        }

		ret = write(fd, client_payload, client_payload_length);
		if (ret < 0) {
			perror("write");
			break;
		}
		if (ret != strlen(PAYLOAD)) {
			fprintf(stderr, "buffer exhausted?\n");
			break;
		}
		printf(".");
	}

	close(fd);
	free(pfd);
	return NULL;
}

int main(int argc, char **argv) {
    int opt;
	int port = 0;
	int sockfd;
	int connfd;
	struct sockaddr_in servaddr = { 0 };
	pthread_t pthread;
	int * pfd;

	setvbuf(stdout, NULL, _IONBF, 0);

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch(opt) {
            case 'p':
                port = atoi(optarg);
                break;
            default:
                usage(argv[0]);
                exit(-1);
        }
    }

	if (!port) {
        usage(argv[0]);
        exit(-1);
    }

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(-1);
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
		perror("bind");
		exit(-1);
	}

	if ((listen(sockfd, INT_MAX)) != 0) {
		perror("listen");
		exit(-1);
	}

	while(1) {
		connfd = accept(sockfd, NULL, NULL);
		pfd = (int *) malloc(sizeof(int));
		if (!pfd) {
			perror("malloc");
			exit(-1);
		}
		*pfd = connfd;
		if (connfd < 0) {
			perror("acccept");
			exit(-1);
		}
		if (pthread_create(&pthread, NULL, echo_worker, pfd) != 0) {
			perror("pthread_create");
			exit(-1);
		}
		if (pthread_detach(pthread) != 0) {
			perror("pthread_detach");
			exit(-1);
		}
	}
}
