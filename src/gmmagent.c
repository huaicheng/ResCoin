#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>        /* errno, perror */
#include <time.h>         /* time(), ctime() */
#include <inttypes.h>     /* standard integer type */
#include <sys/types.h>
#include <sys/socket.h>    /* socket() */
#include <netinet/in.h>    /* struct sockaddr */
#include <sys/resource.h>  /* getrlimit() */
#include <sys/stat.h>
#include <syslog.h>        /* openlog() */
#include <signal.h>        /* sigaction() */
#include <unistd.h>        /* fork() */
#include <fcntl.h>


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

static void daemonize_agent(const char *cmd)
{
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
    int fd0, fd1, fd2;
    int i;

    /*
     * clear file creation mask
     */
    umask(0);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(errno);
    }
    /* 
     * in parent process, terminate it 
     */
    if (pid > 0) {
        exit(errno);
    }

    if (setsid() < 0) {
        perror("setsid");
        exit(errno);
    }

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        perror("sigaction");
        exit(errno);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(errno);
    }
    /* 
     * in parent process, terminate it again 
     */
    if (pid > 0) {
        exit(errno);
    }

    if (chdir("/") < 0) {
        perror("chdir");
        exit(errno);
    }

    /* close all file descriptors */
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        perror("getrlimit");
        exit(errno);
    }
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++)
        close(i);

    /*
     * attach fd 0,1,2 to /dev/null
     */
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
    
    /*
     * initialize the log file 
     */
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        exit(errno);
    }
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

    daemonize_agent("gmmagent started");

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
            printf("%"PRIu32"\n", transbuf[i]);  /* transbuf is in network byte order */

        printf("writing: %"PRIu32", %"PRIu32", %"PRIu32", %"PRIu32"\n", minfo.memtotal, minfo.memfree, 
                minfo.buffers, minfo.cached);
        /* float mem_percentage = get_mem_usage(&minfo); */
        write(connfd, transbuf, sizeof(transbuf));

        close(connfd);
    }

    return 0;
}
