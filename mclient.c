#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>
#include "shared.h"

void usage(const char * p) {
    fprintf(stderr, "%s -h host -p port -n number_connections -l connection_length_secs -i interval_tx_secs [-v]\n", p);
}

struct connection_args {
    struct in_addr ip;
    int port;
    int tinterval;
    int tlength;
};

void *connection(void *p) {
    int sockfd;
    struct sockaddr_in dest;
    int telapsed = 0;
    char * response;
    int response_length = strlen(PAYLOAD);
    fd_set rset;
    int ret;
    struct timeval rtimeout;

    struct connection_args * ca = (struct connection_args *) p;
    const struct in_addr ip = ca->ip;
    const int port = ca->port;
    const int tinterval = ca->tinterval;
    const int tlength = ca->tlength;

    response = (char *) malloc(response_length + 1);
    if (!response) {
        perror("malloc");
        exit(-1);
    }

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("sockfd");
		exit(-1);
	}

	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = ip.s_addr;
	dest.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
		perror("connect");
		exit(-1);
	}

    while(1) {
        // send data
        ret = write(sockfd, PAYLOAD, strlen(PAYLOAD));
        if (ret < 0) {
            perror("write");
            break;
        }
        if (ret != strlen(PAYLOAD)) {
            fprintf(stderr, "buffer exhausted?\n");
            break;
        }

        // wait for data or timeout
        FD_ZERO(&rset);
        FD_SET(sockfd, &rset);
        rtimeout.tv_sec = 5;
        rtimeout.tv_usec = 0;
        ret = select(sockfd + 1, &rset, NULL, NULL, &rtimeout);
        if (ret == 0) {
            fprintf(stderr, "read timeout\n");
            break;
        } else if (ret < 0) {
            perror("select");
            break;
        }

        // read data
        ret = read(sockfd, response, response_length);
        if (ret < 0) {
            perror("read");
            break;
        } else if (ret != response_length) {
            fprintf(stderr, "failure reading data from connection: %d\n", ret);
            break;
        }
        response[response_length] = '\0';
        if (strncmp(PAYLOAD, response, response_length) != 0) {
            fprintf(stderr, "failure reading data from connection, response content does not match.\n");
            break;
        }

        telapsed += tinterval; // does not take into account the elapsed read timeout
        if (telapsed >= tlength)
            break;
        else
            sleep(tinterval);
    }
	close(sockfd);
    free(response);
    return NULL;
}

int main(int argc, char **argv) {
    int opt;
    int nconns = 0;
    pthread_t * threads;
    int i;
    int verbose = 0;
    struct connection_args ca = { 0 };

    while ((opt = getopt(argc, argv, "h:p:n:l:i:v")) != -1) {
        switch(opt) {
            case 'h':
                if (inet_aton(optarg, &ca.ip) == 0) {
                    fprintf(stderr, "Invalid address\n");
                    exit(-1);
                }
                break;
            case 'p':
                ca.port = atoi(optarg);
                break;
            case 'n':
                nconns = atoi(optarg);
                break;
            case 'l':
                ca.tlength = atoi(optarg);
                break;
            case 'i':
                ca.tinterval = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                usage(argv[0]);
                exit(-1);
        }
    }

    if (!ca.ip.s_addr || !ca.port || !nconns || !ca.tlength || !ca.tinterval) {
        usage(argv[0]);
        exit(-1);
    }

    if (ca.tlength < ca.tinterval) {
        fprintf(stderr, "warning: total connection length is lower than interval\n");
    }

    threads = (pthread_t *) malloc(sizeof(pthread_t) * nconns);
    if (!threads) {
        perror("malloc");
        exit(-1);
    }
    for(i=0; i<nconns; i++) {
        if (pthread_create(threads + i, NULL, connection, (void *) &ca) != 0) {
            perror("pthread_create");
            exit(-1);
        }
        if (verbose)
            printf("connection %d launched\n", i);
    }
    for(i=0; i<nconns; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(-1);
        }
    }
    return 0;
}
