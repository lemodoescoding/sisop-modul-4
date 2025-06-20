[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/V7fOtAk7)
| NRP | Name |
| :--------: | :------------: |
| 5025241054 | Andie Azril Alfrianto |
| 5025241060 | Christina Tan |
| 5025241061 | Ahmad Satrio Arrohman |

# Praktikum Modul 4 _(Module 4 Lab Work)_

</div>

### Daftar Soal _(Task List)_

- [Task 1 - FUSecure](/task-1/)

- [Task 2 - LawakFS++](/task-2/)

- [Task 3 - Drama Troll](/task-3/)

- [Task 4 - LilHabOS](/task-4/)

### Laporan Resmi Praktikum Modul 4 _(Module 4 Lab Work Report)_

Tulis laporan resmi di sini!

_Write your lab work report here!_

#### Task 1 - FUSecure

**- Answer:**

Membuat folder mountpoint serta source dan users

```bash
sudo mkdir -p /home/shared_files/{public,private_yuadi,private_irwandi}
sudo chown 1000:1000 /home/shared_files/{pubic,private_yuadi,private_irwandi}

useradd yuadi
useradd irwandi
sudo passwd yuadi # -> set password user yuadi
sudo passwd irwandi # -> set password user irwandi

id yuadi # -> get UID dan GID yuadi (misal 1001)
id irwandi # -> get UID dan GID irwandi (misal 1002)

sudo chown 1001:1001 /home/shared_files/private_yuadi
sudo chown 1002:1002 /home/shared_files/private_irwandi

sudo mkdir -p /mnt/task-2
sudo chown 1000:1000 /mnt/task-2
```

FUSecure.c

```FUSecure.c
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

  if (strncmp(path, "/private_irwandi/", 16) == 0 ||
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

  int accessGrant =
      checkAccessPerm(path, fuse_get_context()->uid, fuse_get_context()->gid);
  if (accessGrant != 0)
    return accessGrant;

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
```

Menjalankan FUSE

```bash
gcc -Wall FUSecure.c -o FUSecure `pkg-config fuse --cflags --libs`
./FUSecure /mnt/securefs -f -o allow_other -d
```

**- Explanation**

Pertama adalah setup direktori serta pembuatan user, untuk direktori seperti yang dijelaskan di soal diberikan kebebasan untuk memilih source directory sebagai tempat penyimpanan semua file, disini kami memilih untuk menggunakan direktori di `/home/shared_files/` yang di buat melalui command bash. Kemudian untuk mempermudah penyelesaian, di file `FUSecure.c` ditambahkan sebuah konstanta string berisi `/home/shared_files/` yang dijadikan source directory dan langsung digunakan oleh FUSE ketika dijalankan.

Kemudian di dalam direktori tersebut juga dibuat 3 direktori yakni `public, private_yuadi, private_irwandi`. Kemudian untuk user, digunakan commmand `useradd` untuk membuat user baru di sistem host dan menggunakan `passwd` untuk mengset masing-masing password dari user yang dibuat.
Kemudian untuk mendapatkan UID dan GID dari user `yuadi` dan `irwandi` yang baru saja dibuat, digunakan command `id` yang akan menampilkan UID serta GID dari suatu user. UID dan GID ini berguna untuk memberikan hak akses ke direktori private.

// insert image here

Direktori private seperti `private_yuadi` dan `private_irwandi` akan diganti ownership nya yang semula milik `1000:1000` (UID:GID) menjadi milik masing-masing user yang sesuai menggunakan command `chown`, sebagai contoh `1001:1001` untuk yuadi dan `1002:1002` untuk irwandi.
Kemudian membuat direktori tempat mounting FUSE, disini kami membuat direktori baru di `/mnt/task-2` dengan menggunakan command `sudo mkdir`, dan menggunakan `chown` untuk dapat diakses oleh FUSE-nya.

// insert image here

Kemudian di task-1 subsoal c disebutkan untuk FUSE ini dijadikan read-only untuk semua user irwandi dan yuadi. Untuk meng-setup sebuah FUSE yang hanya read-only maka untuk fungsi-fungsi yang memberikan akses untuk menuliskan file atau membuat direktori harus direject dengan mengembalikan error `-EACCES`.
Beberapa contoh fungsi yang harus direject dengan pengembalian error `-EACCES` adalah seperti `xmp_write`, `xmp_create`, `xmp_mkdir`, `xmp_rename` dan sebagainya. Untuk fungsi-fungsi yang nantinya dipakai untuk melihat isi folder dan membuka file hanya kondisi tertentu saja yang mengembalikan nilai `-EACCES`.

Kemudian di task-1 subsoal d disebutkan bahwa khusus direktori `public` dapat diakses oleh siapa saja. Untuk hal ini sebenarnya tidak perlu mengubah konfigurasi default dari fungsi-fungsi yang sudah disediakan dari modul 4 Sistem Operasi, dimana secara default user manapun jika tidak dilakukan pengecekan tambahan seperti membandingkan UID atau GID akan dapat mengakses secara default sebuah direktori atau file selama file atau direktori tersebut di buat oleh user biasa (bukan root).

Kemudian di task-1 subsoal e dijelaskan untuk masing-masing **_isi_** direktori `private_irwandi` dan `private_yuadi` hanya boleh diakses oleh masing-masing pemilik direktori, satu hanya dapat diakses oleh irwandi dan satu lagi hanya dapat diakses oleh yuadi. Sedangkan public boleh diakses oleh user manapun. Untuk mengecek user yang mengakses direktori yang terdapat awalan `private_` perlu dicek terlebih dahulu apa UID dari user yang ingin mengakses dengan mendapatkan konteks user sekarang menggunakan fungsi `fuse_get_context()` yang nantinya akan mengembalikan beberapa informasi mengenai user yang sedang mengakses seperti UID dan GID.

Kemudian menggunakan bantuan fungsi tambahan `checkAccessPerm` akan dicek direktori apa yang ingin diakses serta UID user yang ingin mengakses. Untuk kasus direktori `private_yuadi` atau dengan UID 1001 akan dicek ketika akan mengakses path dengan awalan `/private_yuadi`, apabila UID tidak sama dengan 1001 maka akan dikembalikan `-EACCES`. Hal serupa juga diterapkan pada kasus direktori `private_irwandi` atau dengan UID 1002.

Fungsi `checkAccessPerm` akan dipanggil di masing-masing fungsi `xmp_read` dan `xmp_open`. Untuk fungsi `xmp_readdir` dan `xmp_getattr` memang tidak dicek untuk kesamaan UID karena setiap user dapat mengakses direktori-direktori private namun tidak dapat mengakses file didalamnya, dalam hal ini seperti meng-invoke command seperti `cat`.

// insert gambar di sini
