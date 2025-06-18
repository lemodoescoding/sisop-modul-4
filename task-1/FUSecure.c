#define FUSE_USE_VERSION 28
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <fuse/fuse.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static const char *source_path = "/home/shared_files";

void mkdir_mountpoint(const char *path) {
  struct stat st = {0};
  if (stat(path, &st) == -1) {
    mkdir(path, 0755);
  }
}

static int checkAccessPerm(const char *path, uid_t uid, gid_t gid) {
  char fullPath[PATH_MAX];
  struct stat st;

  snprintf(fullPath, sizeof(fullPath), "%s%s", source_path, path);
  if (lstat(fullPath, &st) == -1)
    return -errno;

  if (strncmp(path, "/private_yuadi/", 14) == 0 ||
      strcmp(path, "/private_yuadi") == 0) {
    if (uid != 1001)
      return -EACCES;
  }

  if (strncmp(path, "/private_iwandi/", 15) == 0 ||
      strcmp(path, "/private_irwandi") == 0) {
    if (uid != 1002)
      return -EACCES;
  }

  return 0;
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
  int res;
  char fpath[PATH_MAX];

  sprintf(fpath, "%s%s", source_path, path);

  int accessGrant =
      checkAccessPerm(path, fuse_get_context()->uid, fuse_get_context()->gid);
  if (accessGrant != 0)
    return accessGrant;

  res = lstat(fpath, stbuf);

  if (res == -1)
    return -errno;

  return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
  char fpath[PATH_MAX];

  if (strcmp(path, "/") == 0) {
    path = source_path;
    sprintf(fpath, "%s", path);
  } else {
    sprintf(fpath, "%s%s", source_path, path);

    int accessGrant =
        checkAccessPerm(path, fuse_get_context()->uid, fuse_get_context()->gid);
    if (accessGrant != 0)
      return accessGrant;
  }

  int res = 0;

  DIR *dp;
  struct dirent *de;
  (void)offset;
  (void)fi;

  dp = opendir(fpath);

  if (dp == NULL)
    return -errno;

  while ((de = readdir(dp)) != NULL) {
    char childPath[PATH_MAX];

    snprintf(childPath, sizeof(childPath), "%s%s", path, de->d_name);

    /* if (checkAccessPerm(childPath, fuse_get_context()->uid, */
    /*                     fuse_get_context()->gid) != 0) */
    /*   continue; */

    struct stat st;

    memset(&st, 0, sizeof(st));

    st.st_ino = de->d_ino;
    st.st_mode = de->d_type << 12;
    res = (filler(buf, de->d_name, &st, 0));

    if (res != 0)
      break;
  }

  closedir(dp);

  return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
  char fpath[PATH_MAX];
  if (strcmp(path, "/") == 0) {
    path = source_path;

    sprintf(fpath, "%s", path);
  } else {
    sprintf(fpath, "%s%s", source_path, path);

    int accessGrant =
        checkAccessPerm(path, fuse_get_context()->uid, fuse_get_context()->gid);
    if (accessGrant != 0)
      return accessGrant;
  }

  int res = 0;
  int fd = 0;

  (void)fi;

  fd = open(fpath, O_RDONLY);

  if (fd == -1)
    return -errno;

  res = pread(fd, buf, size, offset);

  if (res == -1)
    res = -errno;

  close(fd);

  return res;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
  char fullPath[PATH_MAX];

  snprintf(fullPath, sizeof(fullPath), "%s%s", source_path, path);

  if ((fi->flags & O_ACCMODE) != O_RDONLY)
    return -EACCES;

  int res = open(fullPath, fi->flags);
  if (res == -1)
    return -errno;

  close(res);
  return 0;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
  return -EACCES;
}

static int xmp_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi) {
  return -EACCES;
}

static int xmp_unlink(const char *path) { return -EACCES; }

static int xmp_mkdir(const char *path, mode_t mode) { return -EACCES; }

static int xmp_rmdir(const char *path) { return -EACCES; }

static int xmp_rename(const char *from, const char *to) { return -EACCES; }

static int xmp_truncate(const char *path, off_t size) { return -EACCES; }

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
  return -EACCES;
}

static int xmp_chmod(const char *path, mode_t mode) { return -EACCES; }

static int xmp_chown(const char *path, uid_t uid, gid_t gid) { return -EACCES; }

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
    .open = xmp_open,
    .write = xmp_write,
    .create = xmp_create,
    .unlink = xmp_unlink,
    .mkdir = xmp_mkdir,
    .rmdir = xmp_rmdir,
    .rename = xmp_rename,
    .truncate = xmp_truncate,
    .utimens = xmp_utimens,
    .chmod = xmp_chmod,
    .chown = xmp_chown,
};

int main(int argc, char *argv[]) {
  umask(0);

  if (argc < 4) {
    fprintf(stderr, "Usage: %s <mountpoint> [-o allow_other] [-f] [-d]\n",
            argv[0]);
    return 1;
  }

  return fuse_main(argc, argv, &xmp_oper, NULL);
}
