#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include "common.h"
#include "hide_module.h"

ptr_t find_syscall_table(void);

extern asmlinkage ssize_t (*original_write)(int fd, const char __user *buff, ssize_t count);
asmlinkage ssize_t duck_write(int fd, const char __user *buff, ssize_t count);

extern asmlinkage ssize_t (*original_kill)(pid_t pid, int sig);
asmlinkage ssize_t duck_kill(pid_t pid, int sig);

#endif /* _SYSCALLS_H_ */