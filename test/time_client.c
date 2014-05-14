#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAXLINE 8192

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;
    int n;
    char recvline[MAXLINE];
    float recvnum;

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
    servaddr.sin_port = htons(13); /* port 13 is used for time server */
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
    while ((n = read(sockfd, &recvnum, MAXLINE)) > 0) {
        if (fprintf(stdout, "%.2f", recvnum) == EOF) {
            fprintf(stderr, "fprintf error\n");
            exit(errno);
        }
    }
    if (n < 0)
        printf("read error\n");

    return 0;
}
