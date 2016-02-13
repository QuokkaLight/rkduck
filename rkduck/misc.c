#include "misc.h"

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

char* str_remove_duplicates(char *str) {
    char *w = str;
    char *r = str;
    char last_char = 'a';

    while (*r) {
        if (*r == '/') {
            if (last_char != '/') {
                *w++ = *r;
                last_char = *r;
            }

            r++;
        } else {
            *w++ = *r;
            last_char = *r++;
        }
    }

    *w = 0;

    return str;
}