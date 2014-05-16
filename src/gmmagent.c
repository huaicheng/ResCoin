#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>       /* errno, perror */
#include <time.h>        /* time(), ctime() */
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PROC_MEMINFO "/proc/meminfo"
#define L_PORT       4588

struct meminfo /* all in kB */
{
    uint32_t memtotal;
    uint32_t memfree;
    uint32_t buffers;
    uint32_t cached;
    /* more can be added here */
};

enum {
    MEMTOTAL,
    MEMFREE,
    BUFFERS,
    CACHED,
    NR_MEMINFO
};

void rd_meminfo(struct meminfo *miptr)
{
    FILE *fp;
    char buff[1024]; /* 1024 is engough for each row of /proc/meminfo */

    if (NULL == (fp = fopen(PROC_MEMINFO, "r"))) {
        perror("fopen");
        exit(errno);
    }
    while (fgets(buff, sizeof(buff), fp)) {
        if (!strncmp("MemTotal:", buff, strlen("MemTotal:")))
            sscanf(buff, "%*s%"PRIu32"", &miptr->memtotal);
        else if (!strncmp("MemFree:", buff, strlen("MemFree:")))
            sscanf(buff, "%*s%"PRIu32"", &miptr->memfree);
        else if (!strncmp("Buffers:", buff, strlen("Buffers:")))
            sscanf(buff, "%*s%"PRIu32"", &miptr->buffers);
        else if (!strncmp("Cached:", buff, strlen("Cached:"))) {
            sscanf(buff, "%*s%"PRIu32"", &miptr->cached);
            /* we stop here because we only need these above info */
            break;
        }
    }
}

float get_mem_usage(struct meminfo *miptr) /* 1~100(%) */
{
    return (100.0 * (miptr->memfree + miptr->buffers \
            + miptr->cached) / miptr->memtotal);
}

int main(int argc, char **argv)
{
    int sockfd, connfd;
    struct sockaddr_in servaddr;
    time_t ticks;
    char buff[1024];
    struct meminfo minfo;
    FILE *fp;
    uint32_t transbuf[NR_MEMINFO];
    int i;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(errno);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(L_PORT);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(errno);
    }

    if (listen(sockfd, 1024) < 0) {
        perror("listen");
        exit(errno);
    }

    for (; ;) {
        if ((connfd = accept(sockfd, (struct sockaddr *)NULL, NULL)) < 0) {
            perror("accept");
            exit(errno);
        }
        
        memset(&minfo, 0, sizeof(minfo));
        rd_meminfo(&minfo);
        transbuf[MEMTOTAL] = htonl(minfo.memtotal);
        transbuf[MEMFREE] = htonl(minfo.memfree);
        transbuf[BUFFERS] = htonl(minfo.buffers);
        transbuf[CACHED] = htonl(minfo.cached);
        for (i = 0; i < NR_MEMINFO; i++)
            printf("%"PRIu32"\n", transbuf[i]);  // transbuf is in network byte order

        printf("writing: %"PRIu32", %"PRIu32", %"PRIu32", %"PRIu32"\n", minfo.memtotal, minfo.memfree, 
                minfo.buffers, minfo.cached);
        //float mem_percentage = get_mem_usage(&minfo);
        write(connfd, transbuf, sizeof(transbuf));

        close(connfd);
    }

    return 0;
}
