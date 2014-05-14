#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char **argv)
{
    int sockfd, connfd;
    struct sockaddr_in servaddr;
    time_t ticks;
    char buff[1024];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket error\n");
        exit(errno);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(13);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        //fprintf(stderr, "bind error\n");
        perror("bind error");
        exit(errno);
    }

    if (listen(sockfd, 1024) < 0) {
        fprintf(stderr, "listen error\n");
        exit(errno);
    }

    for (; ;) {
        if ((connfd = accept(sockfd, (struct sockaddr *)NULL, NULL)) < 0) {
            fprintf(stderr, "accept error\n");
            exit(errno);
        }

        ticks = time(NULL);
        printf("%s\n", ctime(&ticks));
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        printf("buff: %s\n", buff);
        write(connfd, buff, strlen(buff));

        close(connfd);
    }

    return 0;
}
