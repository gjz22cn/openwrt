#include "datatypes.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <fcntl.h>

#include "controlers.h"
#include "log.h"

#define L_CTL_MSG_ADDR "/var/l_ctl_msg_addr"
#define MAX_LCTLC_ID_LEN 16
#define MAX_CTL_MSG_LEN 512

typedef enum {
    CTLMSG_REGISTER=0,
} CtlMsgType;

typedef struct ctl_msg_header
{
   unsigned char crlId[MAX_LCTLC_ID_LEN];
   CtlMsgType  type;
   unsigned int seq;
   struct ctl_msg_header *next;
   unsigned int wordData;
   int dataLength;
} CtlMsgHeader;

typedef struct {
    int fd_;
    char id[MAX_LCTLC_ID_LEN];
} controler_t;

typedef slist_t controlers_t;

static int ctlListenFd=-1;
static controlers_t controlers;

static void controlers_delete_element(void* e)
{
    if(!e)
        return;

    controler_t* element = (controler_t*)e;
    if(element->fd_ >= 0)
        close(element->fd_);

    free(e);
}

static int controlers_init(controlers_t* list)
{
    return slist_init(list, &controlers_delete_element);
}

static void controlers_clear(controlers_t* list)
{
    slist_clear(list);
}

static int controlers_add(int fd)
{
    controlers_t* list=&controlers;

    if(!list)
        return -1;

    controler_t* element = malloc(sizeof(controler_t));
    if(!element) {
        close(fd);
        return -2;
    }

    int i;
    element->fd_ = fd;
    memset(element->id, 0, sizeof(element->id));

    if(slist_add(list, element) == NULL) {
        close(element->fd_);
        free(element);
        return -2;
    }

    return 0;
}

static int initUnixDomainServerSocket()
{
    struct sockaddr_un serverAddr;
    int fd=-1, rc;

    unlink(L_CTL_MSG_ADDR);


    if ((fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
    {
        printf("Could not create socket");
        return fd;
    }

    /*
     * Bind my server address and listen.
     */
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sun_family = AF_LOCAL;
    strncpy(serverAddr.sun_path, L_CTL_MSG_ADDR, sizeof(serverAddr.sun_path));

    rc = bind(fd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (rc != 0)
    {
        printf("bind to %s failed, rc=%d errno=%d", L_CTL_MSG_ADDR, rc, errno);
        close(fd);
        return -1;
    }

    rc = listen(fd, 3);
    if (rc != 0)
    {
        printf("listen to %s failed, rc=%d errno=%d", L_CTL_MSG_ADDR, rc, errno);
        close(fd);
        return -1;
    }


    printf("msg socket opened and ready (fd=%d)", fd);

    return fd;
}

static void handle_msg(CtlMsgHeader *msg)
{
}

static int ctlmsg_receive(int fd, CtlMsgHeader **buf)
{
    CtlMsgHeader *msg;
    char *inBuf;
    int rc;

    if (buf == NULL)
    {
        log_printf(ERROR, "buf is NULL!");
        return -4;
    }
    else
    {
        *buf = NULL;
    }

    /*
     * Read just the header in the first read.
     * Do not try to read more because we might get part of 
     * another message in the TCP socket.
     */
    msg = (CtlMsgHeader *) malloc(sizeof(CtlMsgHeader) + MAX_CTL_MSG_LEN);
    if (msg == NULL)
    {
        log_printf(ERROR, "alloc of msg header failed");
        return -2;
    }

    rc = read(fd, msg, sizeof(CtlMsgHeader));
    if ((rc == 0) || ((rc == -1) && (errno == 131)))
    {
        /* broken connection */
        free(msg);
        return -3;
    }
    else if (rc < 0 || rc != sizeof(CtlMsgHeader))
    {
        log_printf(ERROR, "bad read, rc=%d errno=%d", rc, errno);
        free(msg);
        return -1;
    }

    if (msg->dataLength > MAX_CTL_MSG_LEN)
    {
        msg = (CtlMsgHeader *)realloc(msg, sizeof(CtlMsgHeader) + msg->dataLength - MAX_CTL_MSG_LEN);
        if (msg == NULL)
        {
            log_printf(ERROR, "realloc to %d bytes failed", sizeof(CtlMsgHeader) + msg->dataLength);
            free(msg);
            return -2;
        }
    }

    /* there is additional data in the message */
    inBuf = (char *) (msg + 1);
    rc = read(fd, inBuf, msg->dataLength);
    if (rc <= 0)
    {
        log_printf(ERROR, "bad data read, rc=%d errno=%d", rc, errno);
        free(msg);
        return -1;
    }
    else if (rc < msg->dataLength) {
        log_printf(ERROR, "bad data read, rc=%d expected=%d", rc, msg->dataLength);
        free(msg);
        return -1;
    }

    *buf = msg;

    return 0;
}

void controlers_read_fds(fd_set* set, int* max_fd)
{
    controlers_t* list=&controlers;
    if(ctlListenFd<0)
        return;

    FD_SET(ctlListenFd, set);
    *max_fd = *max_fd > ctlListenFd ? *max_fd : ctlListenFd;

    if(!list)
        return;

    slist_element_t* tmp = list->first_;
    while(tmp) {
        controler_t* l = (controler_t*)tmp->data_;
        FD_SET(l->fd_, set);
        *max_fd = *max_fd > l->fd_ ? *max_fd : l->fd_;
        tmp = tmp->next_;
    }
}

int controlers_handle_accept(fd_set* set)
{
    struct sockaddr_un clientAddr;
    unsigned int sockAddrSize=sizeof(clientAddr);
    int fd;

    if(!FD_ISSET(ctlListenFd, set)) {
        return 0;
    }

    if ((fd = accept(ctlListenFd, (struct sockaddr *)&clientAddr, &sockAddrSize)) < 0)
    {
        printf("accept IPC connection failed. errno=%d", errno);
        return -1;
    }

    return controlers_add(fd);
}

int controlers_read(fd_set* set)
{
    controlers_t* list=&controlers;

    if(!list)
        return -1;

    slist_element_t* tmp = list->first_;
    while(tmp) {
        CtlMsgHeader *msg=NULL;
        controler_t* c = (controler_t*)tmp->data_;
        tmp = tmp->next_;
        if(!FD_ISSET(c->fd_, set)) {
            continue;
        }

        if (ctlmsg_receive(c->fd_, &msg) < 0) {
            log_printf(INFO, "client %d read error, removing it", c->fd_);
            slist_remove(list, c);
            break;
        }

        handle_msg(msg);

        free(msg);
    }

    return 0;
}

int ctlers_fd()
{
    return ctlListenFd;
}

int ctlers_init()
{
    if (controlers_init(&controlers) < 0)
    {
        return -1;
    }

    if ((ctlListenFd=initUnixDomainServerSocket()) < 0)
    {
        return -1;
    }

    return ctlListenFd;
}

void ctlers_clean()
{
    controlers_clear(&controlers);
}