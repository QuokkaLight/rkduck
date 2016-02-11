#ifndef _LIST_H_
#define _LIST_H_

#include "common.h"

struct hook {
    void *target;
    unsigned char hijacked_code[CODE_SIZE];
    unsigned char original_code[CODE_SIZE];
    struct list_head list;
};

struct hidden_file {
    char *path;
    struct list_head list;
};


#endif /* _LIST_H_ */