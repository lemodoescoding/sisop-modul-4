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

### Task 2 - LawakFS++
**Answer:**

- **Code:**
```c
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
int access_start_min = 0;
int access_end_min = 0;

char full_path[1024];
void make_full_path(const char *path) {
  snprintf(full_path, sizeof(full_path), "%s%s", source_dir, path);
}

int blocking_secret(const char *path) {
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  int hour = tm_info->tm_hour;
  int min = tm_info->tm_min;

  int current_time = hour * 60 + min;
  int start_time = access_start_hour * 60 + access_start_min;
  int end_time = access_end_hour * 60 + access_end_min;

  const char *filename = strrchr(path, '/');
  filename = filename ? filename + 1 : path;

  if (strcasestr(filename, secret_file) != NULL) {
    if (start_time < end_time) {
      if (current_time < start_time || current_time >= end_time) {
        return 1;
      }
    } else {
      if (current_time < start_time && current_time >= end_time) {
        return 1;
      }
    }
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
      access_start_min = m;
    } else if (strncmp(line, "ACCESS_END=", 11) == 0) {
      int h, m;
      sscanf(line + 11, "%d:%d", &h, &m);
      access_end_hour = h;
      access_end_min = m;
    }
  }
  fclose(f);

  for (int i = 0; i < filtered_count; i++) {
    printf("FILTERED: %s\n", filtered_words[i]);
  }

  printf("START = %02d:%02d, END = %02d:%02d\n", access_start_hour,
         access_start_min, access_end_hour, access_end_min);
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
  char result[8192];
  size_t res_idx = 0;
  size_t i = 0;

  while (i < *size) {
    bool matched = false;
    for (int k = 0; k < filtered_count; k++) {
      size_t len = strlen(filtered_words[k]);
      if (i + len <= *size &&
          strncasecmp(&buf[i], filtered_words[k], len) == 0) {
        memcpy(&result[res_idx], "lawak", 5);
        res_idx += 5;
        i += len;
        matched = true;
        break;
      }
    }

    if (!matched) {
      result[res_idx++] = buf[i++];
    }
  }

  memcpy(buf, result, res_idx);
  *size = res_idx;
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

    if (strcasestr(entry->d_name, secret_file) && blocking_secret(entry->d_name)) continue;

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

int main(int argc, char *argv[]) {
  source_dir = "/home/ubuntu/filelawakfs"; // bisa diganti source dir masing-masing

  umask(0);
  read_config();

  return fuse_main(argc, argv, &xmp_oper, NULL);
}

// buat var/log/lawakfs.log dibuat manual, lalu kasih chown 1000:1000
```

- **Explanation:**

  Pertama, deklarasikan variable global yang dibutuhkan dengan ```NULL``` untuk string dan ```0``` untuk int.

  Fungsi ```make_full_path``` digunakan untuk generate path lengkap dari sebuah file.

  Fungsi ```blocking_secret``` digunakan untuk mendeteksi file yang memiliki basename yang sudah ditentukan di ```lawak.conf```, lalu cek apakah file tersebut berada di jam akses yang diperbolehkan atau tidak. Jika di luar jam akses, maka sembuntikan file tersebut. Dalam kode, ambil waktu sekarang, ```start_time```, dan ```end_time```. Ketiganya dikonversikan menjadi total menit dengan mengalikan jam dengan 60 dan ditambah sisa menit. Lalu, cek apakah sebuah file memiliki basename yang ditentukan, lalu cek lagi apakah file itu di luar jam akses atau tidak. Dibuat 2 kondisi, jika ```start_time``` < ```end time```, contohnya: 08.00 - 19.00, dan kondisi kedua jika ```start_time``` > ```end time```, contohnya: 22.00 - 02.30. 

  Pointer array ```filtered_words``` dan int ```filtered_count``` akan digunakan untuk menyimpan kata-kata yang difilter ke dalam array.

  Fungsi ```read_config``` digunakan untuk membaca hasil ```lawak.conf``` dan mengambil string yang dibutuhkan dan disimpan sebagai variabel yang akan digunakan di dalam kode. Pada ```access_start``` dan ```access_end``` diambil 2 variabel yaitu hour dan minute.

  Fungsi ```is_text``` digunakan untuk mendeteksi sebuah file adalah file teks atau bukan. Jika file adalah biner, maka return false.

  Fungsi ```change_filtered_words``` digunakan untuk mengganti kata-kata yang difilter dengan kata ```lawak```. Fungsinya sudah fleksibel menyesuaikan panjang string yang akan diganti.

  Array base64_map digunakan untuk menampung tabel karakter untuk encoding base64. Lalu, fungsi pointer ```base64_encode_binary``` digunakan untuk mengubah isi file, contohnya file gambar, ke dalam format base64. Caranya dengan konversi per 3 byte menjadi 4 karakter.

  Fungsi ```log_record``` digunakan untuk mencatat aktivitas yang dilakukan user ke dalam ```/var/log/lawakfs.log```. Caranya dengan ambil waktu lokal sekarang dan menyimpan ke ```timebuf```, lalu ambil uid dengan ```getuid```, lalu ambil action yang dilakukan oleh user, dalam hal ini hanya ```access``` dan ```read``` saja, lalu buat log dengan format yang sudah ditentukan.

  Fungsi ```xmp_getattr``` digunakan untuk mendapat informasi atribut sebuah file, seperti size, tipe, dll. Pada fungsi, cek apakah file tersebut diblock atau tidak, jika iya maka akan return -ENOENT yang berarti Error: No such file or directory. ```lstat``` akan mengambil stat dari file dan simpan ke variabel res. Jika ```res==-1``` artinya pemanggilan ```lstat``` gagal dan akan mengembalikkan -errno.

  Fungsi ```xmp_readdir```digunakan untuk membaca isi direktori saat ini dan direktori parent. Lalu cek setiap entri file apakah file tersebut termasuk file yang diblock atau tidak, jika iya maka file tersebut tidak akan terlihat saat memanggil ls.

  Fungsi ```xmp_read``` digunakan untuk membaca isi file, contohnya saat kita memanggil cat. Cek apakah file tersebut diblock atau tidak, jika iya maka return -ENOENT. Jika tidak, maka open file tersebut tapi dibatasi ```0_RDONLY``` yang artinya read only. Lalu, setiap kali dijalankan, maka log akan tercatat dengan aktivitas READ. Lalu, file akan dicek apakah isi file itu adalah text atau tidak, jika iya maka fungsi ```change_filtered_words``` akan dipanggil dan mengubah kata-kata yang difilter menjadi ```lawak```. Jika isi file adalah biner, maka fungsi ```base64_encode_binary``` akan dipanggil dan isi file akan dikonversikan ke dalam format base64.

  Fungsi ```xmp_open``` digunakan untuk membuka sebuah file, contohnya saat menjalankan cmd nano,tetapi karena kita membuat read only file system, maka perintah write tidak bisa dijalankan saat nano. Fungsi akan mengecek ```fi->flags```, jika ```0_WRONLY``` yang berarti write only atau ```0_RDWR``` yang berarti read & write, maka akan return -EROFS atau Error: read only file system. Fungsi juga akan mengecek apakah file tersebut diblock atau tidak, jika iya maka return -ENOENT. Open file dengan flag akses ```fi->flags``` lalu simpan ke variabel res. Jika ```res==-1``` maka return open gagal dan return -errno.

  Fungsi ```xmp_access``` digunakan untuk mengecek apakah user boleh mengakses file dengan mode tertentu, seperti write atau read. Cek apakah bitmask ada ```W_OK``` yang berarti boleh write, jika iya maka return -EROFS. Cek juga apakah file diblock atau tidak, jika iya maka return -ENOENT. Lalu, panggil ```access(full_path, mask)``` untuk mengecek izin akses ke file dan simpan hasilnya ke variabel res. Jika ```res==0``` artinya akses diizinkan dan panggil fungsi ```log_record```

  Untuk fungsi lainnya, seperti ```xmp_create```, ```xmp_write```, ```xmp_unlink```, ```xmp_mkdir```, ```xmp_rmdir```, ```xmp_rename```, ```xmp_truncate```, dan ```xmp_utiments``` diblokir dengan cara return -EROFS, sehingga setiap kali user menjalankan command tersebut tidak akan bisa dan menampilkan ```Error: read only file system```.

  Fungsi yang udah kita build sebelumnya akan disimpan ke struct ```fuse_operations xmp_oper```, yang kemudian akan dipakai oleh FUSE untuk menangani setiap operasi filesystem.

  Pada fungsi main, deklarasikan source direktori yang berisi file-file seperti ```secret.txt```, ```image.jpeg```, dll. Lalu, panggil fungsi ```read_config``` .

  Notes:
  Sebelum jalankan kode, ganti source_dir dengan path masing-masing dan buat ```/var/log/lawakfs.log``` dengan cmd ```touch /var/log/lawakfs.log``` dan ubah kepemilikan file dengan user id masing-masing dengan cmd ```chown 1000:1000``` atau id lainnya.

- **Screenshot:**

  ![](/task-2/assets/4b1.png)

  Cmd untuk menjalankan kode.

  ![](/task-2/assets/4b2.png)

  Pada hasil ls, terlihat file dengan basename secret tidak muncul karena masih di luar jam akses, lalu untuk cat sebuah file harus tetap menyertakan ekstensinya, jika tidak ada ekstensinya maka cat file tidak muncul. Saat cat sebuah file image, akan langsung ditampilkan dalam format base64. Saat cat file teks, kata-kata yang difilter akan diganti dengan kata "lawak".

  ![](/task-2/assets/4b5.png)

  Berikut isi diretori asli (sebelum dimount). File dengan basename secret masih ada saat menjalankan ls. Lalu, saat cat ```tekslawak.txt``` masih menampilkan kata-kata yang belum diganti menjadi "lawak". 

  ![](/task-2/assets/4b3.png)

  Berikut hasil log yang dibuat dalam ```/var/log/lawakfs.log```

  ![](/task-2/assets/4b4.png)

  Saat menjalankan fungsi selain read dan ls, contohnya seperti touch dan mkdir, maka akan gagal dan menampilkan read only file system.

