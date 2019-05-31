/********************************************************
* Filename: timerfd.c
* Author: zhangwj
* Desprition: a sample program of timerfd
* Date: 2017-04-17
* Warnning:
********************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#if 0
struct timespec {
    time_t tv_sec;                /* Seconds */
    long   tv_nsec;               /* Nanoseconds */
};

struct itimerspec {
    struct timespec it_interval;  /* Interval for periodic timer */
    struct timespec it_value;     /* Initial expiration */
};
#endif

#define EPOLL_LISTEN_CNT        256
#define EPOLL_LISTEN_TIMEOUT    500

#define LOG_DEBUG_ON 1

#ifdef LOG_DEBUG_ON 
#define LOG_DEBUG(fmt, args...) \
    do {  \
        printf("[DEBUG]:");\
        printf(fmt "\n", ##args); \
    } while(0);
#define LOG_INFO(fmt, args...) \
    do { \
        printf("[INFO]:");\
        printf(fmt "\n", ##args); \
    } while(0);
#define LOG_WARNING(fmt, args...) \
    do { \
        printf("[WARNING]:");\
        printf(fmt "\n", ##args); \
    } while(0);
#else
#define LOG_DEBUG(fmt, args...) 
#define LOG_INFO(fmt, args...) 
#define LOG_WARNING(fmt, args...) 
#endif
#define LOG_ERROR(fmt, args...) \
    do{ \
        printf("[ERROR]:");\
        printf(fmt "\n", ##args);\
    }while(0);

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

// 静态文件变量
static int g_epollfd = -1;
static int g_timerfd = -1;
uint64_t tot_exp = 0;

static void help(void)
{
    exit(0);
}

static void print_elapsed_time(void)
{
    // 函数内静态变量，存储开始的事件
    static struct timespec start;
    struct timespec curr;   // 现在的时间
    static int first_call = 1;
    int secs, nsecs;
    
    // 首次执行一次
    if (first_call) {
        first_call = 0;
        if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) 
            handle_error("clock_gettime");
    }   
    
    if (clock_gettime(CLOCK_MONOTONIC, &curr) == -1) 
        handle_error("clock_gettime");
    
    secs = curr.tv_sec - start.tv_sec;
    nsecs = curr.tv_nsec - start.tv_nsec;
    if (nsecs < 0) {
        secs--;
        nsecs += 1000000000;
    }   
    printf("%d.%03d: ", secs, (nsecs + 500000) / 1000000);
}

// 时间处理回调函数
void timerfd_handler(int fd)
{
    uint64_t exp = 0;
    
    read(fd, &exp, sizeof(uint64_t)); 
    tot_exp += exp;
    print_elapsed_time();
    printf("read: %llu, total: %llu\n", (unsigned long long)exp, (unsigned long long)tot_exp);

    return;
}

void epoll_event_handle(void)
{
    int i = 0;
    int fd_cnt = 0;
    int sfd;
    struct epoll_event events[EPOLL_LISTEN_CNT];    

    memset(events, 0, sizeof(events));
    while(1) 
    {   
        /* wait epoll event */
        // epoll 监听 并将触发的事件存入事件数组
        fd_cnt = epoll_wait(g_epollfd, events, EPOLL_LISTEN_CNT, EPOLL_LISTEN_TIMEOUT); 
        for(i = 0; i < fd_cnt; i++) 
        {   
            sfd = events[i].data.fd;
            if(events[i].events & EPOLLIN) // 是否是读取事件
            {   
                if (sfd == g_timerfd) 
                {
                    timerfd_handler(sfd); 
                }   
            }   
        } 
    }   
}

int epoll_add_fd(int fd)
{
    int ret;
    struct epoll_event event;

    memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET; // ET 模式下的 读取事件
    
    // epoll 添加定时fd
    ret = epoll_ctl(g_epollfd, EPOLL_CTL_ADD, fd, &event);
    if(ret < 0) {
        LOG_ERROR("epoll_ctl Add fd:%d error, Error:[%d:%s]", fd, errno, strerror(errno));
        return -1;
    }

    LOG_DEBUG("epoll add fd:%d--->%d success", fd, g_epollfd);
    return 0;    
}

int epollfd_init()
{
    int epfd;

    /* create epoll fd */
    epfd = epoll_create(EPOLL_LISTEN_CNT); 
    if (epfd < 0) {
        LOG_ERROR("epoll_create error, Error:[%d:%s]", errno, strerror(errno));
        return -1;
    }
    g_epollfd = epfd;
    LOG_DEBUG("epoll fd:%d create success", epfd);

    return epfd;
}

int timerfd_init()
{
    int tmfd;
    int ret;
    struct itimerspec new_value;
//     struct itimerspec
//   {
//     struct timespec it_interval;
//     struct timespec it_value;
//   };
    new_value.it_value.tv_sec = 2;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = 1;
    new_value.it_interval.tv_nsec = 0;
    
    tmfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tmfd < 0) {
        LOG_ERROR("timerfd_create error, Error:[%d:%s]", errno, strerror(errno));
        return -1;
    }
    //  flags：参数flags为1代表设置的是绝对时间（TFD_TIMER_ABSTIME 表示绝对定时器）；为0代表相对时间。
    ret = timerfd_settime(tmfd, 0, &new_value, NULL);
    if (ret < 0) {
        LOG_ERROR("timerfd_settime error, Error:[%d:%s]", errno, strerror(errno));
        close(tmfd);
        return -1;
    }

    if (epoll_add_fd(tmfd)) {
        close(tmfd);
        return -1;
    }
    g_timerfd = tmfd;

    return 0;
}

int main(int argc, char **argv)
{
    // 创建 epoll fd
    if (epollfd_init() < 0) {
        return -1;
    }

    // 创建 timerfd 并设置定时时间，添加到epoll中进行监听
    if (timerfd_init()) {
        return -1;
    }

    /* event handle */
    epoll_event_handle();

    return 0;
}