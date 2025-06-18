#define FUSE_USE_VERSION 28
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

static const char *source_dir = NULL;
char secret_file[100] = "";
int access_start_hour = 0;
int access_end_hour = 0;

char full_path[1024];
void make_full_path(const char *path) {
  snprintf(full_path, sizeof(full_path), "%s%s", source_dir, path);
}

int blocking_secret(const char *path) {
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  int hour = tm_info->tm_hour;

  const char *filename = strrchr(path, '/');
  filename = filename ? filename + 1 : path;

  if (strncmp(filename, secret_file, strlen(secret_file)) == 0 &&
      (filename[strlen(secret_file)] == '\0' ||
       filename[strlen(secret_file)] == '.')) {
    if (hour < access_start_hour || hour >= access_end_hour)
      return 1;
  }
  return 0;
}

char *filtered_words[100];
int filtered_count = 0;

void read_config() {
  FILE *f = fopen("lawak.conf", "r");
  if (!f)
    return;

  char line[512];
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "FILTER_WORDS=", 13) == 0) {
      char *words = line + 13;
      words[strcspn(words, "\n")] = '\0';
      char *token = strtok(words, ",");
      while (token && filtered_count < 100) {
        filtered_words[filtered_count++] = strdup(token);
        token = strtok(NULL, ",");
      }
    } else if (strncmp(line, "SECRET_FILE_BASENAME=", 21) == 0) {
      sscanf(line + 21, "%s", secret_file);
    } else if (strncmp(line, "ACCESS_START=", 13) == 0) {
      int h, m;
      sscanf(line + 13, "%d:%d", &h, &m);
      access_start_hour = h;
    } else if (strncmp(line, "ACCESS_END=", 11) == 0) {
      int h, m;
      sscanf(line + 11, "%d:%d", &h, &m);
      access_end_hour = h;
    }
  }
  fclose(f);

  for (int i = 0; i < filtered_count; i++) {
    printf("FILTERED: %s\n", filtered_words[i]);
  }

  printf("START = %d, END = %d\n", access_start_hour, access_end_hour);
  printf("SECRET FILE BASENAME = %s\n", secret_file);
}

bool is_text(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    unsigned char c = data[i];
    if ((c < 0x09) || (c > 0x0D && c < 0x20)) {
      return false; // biner
    }
  }
  return true;
}

void change_filtered_words(char *buf, size_t *size) {
  for (int i = 0; i < filtered_count; i++) {
    char *p = buf;
    while ((p = strcasestr(p, filtered_words[i])) != NULL) {
      memcpy(p, "lawak", 5);
      size_t len = strlen(filtered_words[i]);
      if (len > 5)
        memmove(p + 5, p + len, *size - (p - buf) - len + 1);
      *size -= (len > 5) ? len - 5 : 0;
      p += 5;
    }
  }
}

char base64_map[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
                     'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
                     'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                     'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                     's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
                     '3', '4', '5', '6', '7', '8', '9', '+', '/'};

char *base64_encode_binary(const unsigned char *input, size_t input_len,
                           size_t *output_len) {
  size_t olen = 4 * ((input_len + 2) / 3);
  char *output = malloc(olen + 1);
  if (!output)
    return NULL;

  size_t i, j;
  for (i = 0, j = 0; i < input_len;) {
    uint32_t octet_a = i < input_len ? input[i++] : 0;
    uint32_t octet_b = i < input_len ? input[i++] : 0;
    uint32_t octet_c = i < input_len ? input[i++] : 0;

    uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

    output[j++] = base64_map[(triple >> 18) & 0x3F];
    output[j++] = base64_map[(triple >> 12) & 0x3F];

    int remaining = input_len - (i - 3);

    output[j++] = (remaining < 2) ? '=' : base64_map[(triple >> 6) & 0x3F];
    output[j++] = (remaining < 3) ? '=' : base64_map[triple & 0x3F];
  }

  output[j] = '\0';
  if (output_len)
    *output_len = j;
  return output;
}

void log_record(const char *action, const char *path) {
  FILE *log = fopen("/var/log/lawakfs.log", "a");
  if (!log) {
    perror("fopen");
    return;
  }

  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  char timebuf[64];
  strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

  uid_t uid = getuid();
  fprintf(log, "[%s] [%d] [%s] %s\n", timebuf, uid, action, path);
  fclose(log);
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
  if (blocking_secret(path) == 1) {
    return -ENOENT;
  }
  int res;
  make_full_path(path);
  res = lstat(full_path, stbuf);
  if (res == -1)
    return -errno;
  return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
  if (!source_dir)
    return -EINVAL;

  DIR *dir;
  struct dirent *entry;

  (void)offset;
  (void)fi;

  dir = opendir(source_dir);
  if (!dir)
    return -errno;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    if (strncmp(entry->d_name, secret_file, strlen(secret_file)) == 0 &&
        blocking_secret(entry->d_name))
      continue;

    char display_name[256];
    strncpy(display_name, entry->d_name, sizeof(display_name));
    display_name[sizeof(display_name) - 1] = '\0';

    char *ext = strrchr(display_name, '.');
    if (ext)
      *ext = '\0';

    filler(buf, display_name, NULL, 0);
  }

  closedir(dir);
  return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
  if (blocking_secret(path) == 1) {
    return -ENOENT;
  }

  int fd;
  (void)fi;

  make_full_path(path);
  fd = open(full_path, O_RDONLY);
  if (fd == -1)
    return -errno;

  log_record("READ", path);

  char tmp_buf[8192];
  ssize_t bytes = pread(fd, tmp_buf, sizeof(tmp_buf), 0);
  close(fd);
  if (bytes <= 0)
    return bytes;

  if (is_text(tmp_buf, bytes)) {
    size_t s = bytes;
    change_filtered_words(tmp_buf, &s);
    if (offset >= s)
      return 0;
    size_t len = (offset + size > s) ? s - offset : size;
    memcpy(buf, tmp_buf + offset, len);
    return len;
  } else {
    size_t out_len;
    char *encoded =
        base64_encode_binary((unsigned char *)tmp_buf, bytes, &out_len);
    if (!encoded)
      return -EIO;
    if (offset >= out_len) {
      free(encoded);
      return 0;
    }
    size_t len = (offset + size > out_len) ? out_len - offset : size;
    memcpy(buf, encoded + offset, len);
    free(encoded);
    return len;
  }
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
  int res;

  if ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR))
    return -EROFS;
  if (blocking_secret(path) == 1) {
    return -ENOENT;
  }

  make_full_path(path);
  res = open(full_path, fi->flags);
  if (res == -1)
    return -errno;
  close(res);
  return 0;
}

static int xmp_access(const char *path, int mask) {
  if (mask & W_OK)
    return -EROFS;
  if (blocking_secret(path) == 1) {
    return -ENOENT;
  }

  int res;
  make_full_path(path);
  res = access(full_path, mask);

  if (res == 0) {
    log_record("ACCESS", path);
    return 0;
  }

  return -errno;
}

static int xmp_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi) {
  return -EROFS;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
  return -EROFS;
}

static int xmp_unlink(const char *path) { return -EROFS; }

static int xmp_mkdir(const char *path, mode_t mode) { return -EROFS; }

static int xmp_rmdir(const char *path) { return -EROFS; }

static int xmp_rename(const char *from, const char *to) { return -EROFS; }

static int xmp_truncate(const char *path, off_t size) { return -EROFS; }

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
  return -EROFS;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
    .open = xmp_open,
    .access = xmp_access,
    .write = xmp_write,
    .create = xmp_create,
    .unlink = xmp_unlink,
    .mkdir = xmp_mkdir,
    .rmdir = xmp_rmdir,
    .rename = xmp_rename,
    .truncate = xmp_truncate,
    .utimens = xmp_utimens,
};

void mkdir_mountpoint(const char *dir) {
  struct stat st = {0};
  if (stat(dir, &st) == -1) {
    mkdir(dir, 0755);
  }
}

int main(int argc, char *argv[]) {
  source_dir = "/home/ubuntu/filelawakfs"; //source dir masing-masing 
  size_t len = strlen(source_dir);
  if (len > 1 && source_dir[len - 1] == '/') {
    char *clean_path = strdup(source_dir);
    clean_path[len - 1] = '\0';
    source_dir = clean_path;
  }

  umask(0);
  read_config();

  return fuse_main(argc, argv, &xmp_oper, NULL);
}
