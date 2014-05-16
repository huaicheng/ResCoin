#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MAXLINE 8192
#define L_PORT  4588

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;
    int n;
    uint32_t recvbuf[4];
    float recvnum;
    int i;

    if (argc != 2) {
        fprintf(stderr, "Usage: a.out <IP>\n");
        exit(errno);
    }

    /* socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket error\n");
        exit(errno);
    }

    /* initialize servaddr value of (sin_family, sin_port, sin_addr) */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(L_PORT); /* port 13 is used for time server */
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s", argv[1]);
        exit(errno);
    }

    /* connect */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "connect error\n");
        exit(errno);
    }

    /* 
     * after calling connect successfully, the connection to server is established,
     * we can operate on sockfd to get the information we want
     */
    unsigned long total, free, buffers, cached;
    if ((n = read(sockfd, recvbuf, sizeof(recvbuf))) > 0) {
        for (i = 0; i < 4; i++)
            printf("%"PRIu32"\n", ntohl(recvbuf[i]));
        for (i = 0; i < 4; i++)
            printf("%lu\n", (unsigned long)ntohl(recvbuf[i]));
    }
    //printf("total: %lu\nfree: %lu\nbuffers: %lu\ncached: %lu\n", 
            //total, free, buffers, cached);

    return 0;
}
