#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "wrap.h"
#include "http.h"

#define SERV_PORT 8998
#define MAX_EVENTS 1024
#define BUFLEN 4096

void startsock(int efd, short port);
void clientconn(int lfd, int events, void *arg);
void accept_request(int cfd, int events, void *arg);
void http_response(int cfd, int events, void *arg);
int retype(char *buf, int len);

struct myevent {
    int fd;             //要监听的文件描述符
    int events;         //对应的监听事件
    void *arg;          //泛型参数
    void (*call_back)(int fd, int events, void *arg);  //回调函数
    int status;        //是否在监听:1->在红黑树上(监听), 0->不在(不监听)
    char buf[BUFLEN];
    int len;
    long last_active;   
    int flag;  //保存请求方式
};

int g_efd;   //保存全局的epoll创建的文件描述符
struct myevent g_events[MAX_EVENTS+1];  //自定义结构体数组

//将结构体myevent成员变量初始化
void eventset(struct myevent *ev, int fd, void (*call_back)(int, int, void *), void *arg)
{
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    ev->last_active = time(NULL);   //调用eventset函数的时间
  
    return;
}

//向epoll监听的红黑树 添加一个 文件描述符
void eventadd(int efd, int events, struct myevent *ev)
{
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = ev;
    epv.events = ev->events = events;   //EPOLLIN 或 EPOLLOUT

    if (ev->status == 1) {    //已经在红黑树 g_efd 里
        op = EPOLL_CTL_MOD;   //修改其属性
    } else {                  //不在红黑树里
        op = EPOLL_CTL_ADD;   //将其加入红黑树 g_efd, 并将status置1
        ev->status = 1;
    }

    if (epoll_ctl(efd, op, ev->fd, &epv) < 0)       //实际添加/修改
        printf("event add failed [fd=%d], op = %d, events[%d]\n", ev->fd, op, events);
    else
        printf("event add OK [fd=%d], op = %d, events[%0X]\n", ev->fd, op, events);

    return ;
}

//从epoll 监听的 红黑树中删除一个 文件描述符
void eventdel(int efd, struct myevent *ev)
{
    struct epoll_event epv = {0, {0}};

    if (ev->status != 1)            //不在红黑树上
        return ;

    epv.data.ptr = ev;
    ev->status = 0;   //修改状态
    //从红黑树 efd 上将 ev->fd 摘除
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv); 
    return ;
}

int retype(char* buf, int len){
    char content[255];
    int i = 0;
    while (!isspace((int)buf[i]) && (i < sizeof(content) - 1)) { //不为空，以及未到达上限，将buf中Get或Post拷贝进method
        content[i] = buf[i];
        i++;
    }
    content[i] = '\0';

    if (strcasecmp(content, "GET") && strcasecmp(content, "POST"))  //content 大于 “GET”为真，既不是Get，也不是Post
        return 0;
    else return 1;  //是http请求
}

//接收请求
void accept_request(int cfd, int events, void *arg)
{
    int len;
    struct myevent *ev = (struct myevent *)arg;
    len = Readline(cfd, ev->buf, sizeof(ev->buf));

    eventdel(g_efd, ev);  //将该节点从红黑树上摘除

    if(len > 0){
        ev->len = len;
        ev->buf[len] = '\0'; //手动添加字符串结束标记

        if(retype(ev->buf, len)){  //是http请求
            eventset(ev, cfd, http_response, ev);
            eventadd(g_efd, EPOLLOUT, ev);
        }
        else{  
            char buf[1024];
	        // 向客户端回写500状态,不可以执行CGI程序
	        sprintf(buf, "HTTP/1.0 Data Error\r\n");
            send(ev->fd, buf, strlen(buf), 0);
            Close(ev->fd);
            printf("it's not http request, [fd=%d] pos[%ld], closed\n", cfd, ev-g_events);
        }
    } else if (len == 0) {
        Close(ev->fd);
        /* ev-g_events 地址相减得到偏移元素位置 */
        printf("[fd=%d] pos[%ld], closed\n", cfd, ev-g_events);
    } else {
        Close(ev->fd);
        printf("read[fd=%d] error[%d]:%s\n", cfd, errno, strerror(errno));
    }
    return;
}

void http_response(int cfd, int events, void *arg){

    struct myevent *ev = (struct myevent *)arg;
    disposal(cfd, ev->buf, ev->len);

    printf("response[fd=%d] succeed\n", cfd);
    eventdel(g_efd, ev);     //从红黑树g_efd中移除
    eventset(ev, cfd, accept_request, ev);  //将该fd的 回调函数改为 accept_request
    eventadd(g_efd, EPOLLIN, ev);    //从新添加到红黑树上， 设为监听读事件
}

//初始化socket和红黑树
void startsock(int efd, short port){
    int on = 1;
    int lfd = Socket(AF_INET, SOCK_STREAM, 0);

    fcntl(lfd, F_SETFL, O_NONBLOCK);    //将socket设为非阻塞
    //将lfd添加到以就绪文件描述符中
    eventset(&g_events[MAX_EVENTS], lfd, clientconn, &g_events[MAX_EVENTS]);
    eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]);  //设置lfd为可读事件

    struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin)); //将整个结构体清零,bzero(&sin, sizeof(sin));  
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
    //端口复用
    if ((setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)  {  
        perror("setsockopt failed");
        exit(1);
    }

	Bind(lfd, (struct sockaddr *)&sin, sizeof(sin));
	Listen(lfd, 32);

    return ;
}

//当有lfd文件描述符就绪, epoll返回, 调用该函数 与客户端建立链接 
void clientconn(int lfd, int events, void *arg) {
    struct sockaddr_in cin;
    socklen_t len = sizeof(cin);
    int i;

    int cfd = Accept(lfd, (struct sockaddr *)&cin, &len);

    do {
        for (i = 0; i < MAX_EVENTS; i++) //从全局数组g_events中找一个空闲元素
            if (g_events[i].status == 0) //类似于select中找值为-1的元素
                break;                 //跳出 for

        if (i == MAX_EVENTS) {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;            //跳出do while(0) 不执行后续代码
        }

        //将cfd也设置为非阻塞
        int flag = 0;
        if ((flag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0) {            
            printf("%s: fcntl nonblocking failed, %s\n", __func__, strerror(errno));
            break;
        }

        // 给cfd设置一个 myevent 结构体, 回调函数 设置为 accept_request
        eventset(&g_events[i], cfd, accept_request, &g_events[i]);   
        //将cfd添加到红黑树g_efd中,监听读事件
        eventadd(g_efd, EPOLLIN, &g_events[i]);       
    } while(0);

    printf("new connect [%s:%d][time:%ld], pos[%d]\n\n", 
            inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), g_events[i].last_active, i);
    return ;
}

int main(void) {
    unsigned short port = SERV_PORT; 
    /*保存已经满足就绪事件的文件描述符数组，数组最后一个是监听事件lfd*/
    struct epoll_event events[MAX_EVENTS+1]; 
    g_efd = epoll_Create(MAX_EVENTS+1);  //创建红黑树,返回其根节点，并创建了一个双向链表，用来保存就绪的socket

    startsock(g_efd, port);   //初始化socket和红黑树
    printf("server running: port[%d]\n", port);
 
    int checkpos = 0, i;
    while (1) {
        /* 超时验证，每次测试100个链接，不测试listenfd 
        当客户端60秒内没有和服务器通信，则关闭此客户端链接 */
        long now = time(NULL);     //当前时间
        //一次循环检测100个。 使用checkpos控制检测对象
        for (i = 0; i < 100; i++, checkpos++) {  
            if (checkpos == MAX_EVENTS)
                checkpos = 0;
            if (g_events[checkpos].status != 1) //不在红黑树 g_efd 上
                continue;

            //客户端不活跃的世间
            long duration = now - g_events[checkpos].last_active;       
            if (duration >= 60) {
                Close(g_events[checkpos].fd);   //关闭与该客户端链接
                printf("[fd = %d] timeout\n", g_events[checkpos].fd);
                eventdel(g_efd, &g_events[checkpos]); //将该客户端 从红黑树 g_efd移除
            }
        }
 
        /*监听红黑树g_efd, 将满足的事件的文件描述符加至events数组中, 
        阻塞等待1s，1s没有事件满足, 返回 0*/
        int nfd = epoll_wait(g_efd, events, MAX_EVENTS+1, 1000);
        if (nfd < 0) {
            printf("epoll_wait error, exit\n");
            break;
        }
 
        for (i = 0; i < nfd; i++) {
            /*使用自定义结构体myevent类型指针, 
            接收联合体data的void *ptr成员*/
            struct myevent *ev = (struct myevent *)events[i].data.ptr;  

            //读就绪事件
            if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) { 
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
            //写就绪事件
            if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {         
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
        }
    }
 
    /* 退出前释放所有资源 */
    return 0;

}
