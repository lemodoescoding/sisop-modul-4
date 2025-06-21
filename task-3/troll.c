#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

static const char *file1 = "very_spicy_info.txt";
static const char *file2 = "upload.txt";
static const char *trigger_flag = "/tmp/troll_trigger";

const char *ascii_art =
"     ______     ____   ____              _ __                       _                                           __\n"
"    / ____/__  / / /  / __/___  _____   (_) /_   ____ _____ _____ _(_)___     ________ _      ______ __________/ /\n"
"   / /_  / _ \\/ / /  / /_/ __ \\/ ___/  / / __/  / __ `/ __ `/ __ `/ / __ \\   / ___/ _ \\ | /| / / __ `/ ___/ __  / \n"
"  / __/ /  __/ / /  / __/ /_/ / /     / / /_   / /_/ / /_/ / /_/ / / / / /  / /  /  __/ |/ |/ / /_/ / /  / /_/ /  \n"
" /_/    \\___/_/_/  /_/  \\____/_/     /_/\\__/   \\__,_/\\__, /\\__,_/_/_/ /_/  /_/   \\___/|__/|__/\\__,_/_/   \\__,_/  \n"
"                                                    /____/                                                        \n";

int trigger(){
    return access(trigger_flag, F_OK) == 0;
}

static int xmp_getattr(const char *path, struct stat *stbuf){
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0){
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(path, "/very_spicy_info.txt") == 0 || strcmp(path, "/upload.txt") == 0){
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_nlink = 1;
        stbuf->st_size = 1024;
    } else return -ENOENT;

    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, file1, NULL, 0);
    filler(buf, file2, NULL, 0);

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi){
    if (strcmp(path, "/very_spicy_info.txt") == 0 || strcmp(path, "/upload.txt") == 0)
        return 0;
    return -ENOENT;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    (void) fi;

    uid_t uid = fuse_get_context()->uid;
    struct passwd *pw = getpwuid(uid);
    const char *user = pw->pw_name;

    if (strstr(path, ".txt") && trigger() && strcmp(user, "DainTontas") == 0) {
        size_t len = strlen(ascii_art);
        if (offset < len){
            if (offset + size > len) size = len - offset;
            memcpy(buf, ascii_art + offset, size);
        } else size = 0;

        return size;
    }

    if (strcmp(path, "/very_spicy_info.txt") == 0){
        const char *content;

        if (strcmp(user, "DainTontas") == 0){
            content = "Very spicy internal developer information: leaked roadmap.docx\n";
        } else {
            content = "DainTontas' personal secret!!.txt\n";
        }

        size_t len = strlen(content);
        if (offset < len){
            if (offset + size > len) size = len - offset;
            memcpy(buf, content + offset, size);
        } else size = 0;

        return size;

    } else if (strcmp(path, "/upload.txt") == 0){
        return 0;
    }

    return -ENOENT;
}

void trigger_trap(){
    FILE *f = fopen(trigger_flag, "w");
    if (f) fclose(f)
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    (void) fi;
    (void) offset;

    uid_t uid = fuse_get_context()->uid;
    struct passwd *pw = getpwuid(uid);
    const char *user = pw->pw_name;

    if (strcmp(path, "/upload.txt") == 0 && !trigger() && strcmp(user, "DainTontas") == 0){
        trigger_trap();
        return size;
    }

    return -EACCES;
}

static int xmp_truncate(const char *path, off_t size){
    if (strcmp(path, "/upload.txt") == 0) return 0;
    return -EACCES;
}

static struct fuse_operations troll_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open    = xmp_open,
    .read    = xmp_read,
    .write   = xmp_write,
    .truncate = xmp_truncate,
};

int main(int argc, char *argv[]){
    return fuse_main(argc, argv, &troll_oper, NULL);
}
