#ifndef _HIDE_MODULES_H_
#define _HIDE_MODULES_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/version.h>

extern unsigned short mod_hidden;
extern struct list_head *mod_prev;

void module_hide(void);
void module_show(void);

#endif /* _HIDE_MODULES_H */