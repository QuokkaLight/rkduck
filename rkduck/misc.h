#ifndef _MISC_H_
#define _MISC_H_

#include "common.h"

int set_page_rw(ptr_t address);
int set_page_ro(ptr_t address);
char* str_remove_duplicates(char *str);

#endif