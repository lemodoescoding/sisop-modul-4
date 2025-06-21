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

#### Task 4 - LilHabOS

**- Answer:**

kernel.asm

```asm
global _putInMemory
global _interrupt

; void putInMemory(int segment, int address, char character)
_putInMemory:
  push bp
  mov bp,sp
  push ds
  mov ax,[bp+4]
  mov si,[bp+6]
  mov cl,[bp+8]
  mov ds,ax
  mov [si],cl
  pop ds
  pop bp
  ret

; int interrupt(int number, int AX, int BX, int CX, int DX)
_interrupt:
  push bp
  mov bp,sp
  mov ax,[bp+4]
  push ds
  mov bx,cs
  mov ds,bx
  mov si,intr
  mov [si+1],al
  pop ds
  mov ax,[bp+6]
  mov bx,[bp+8]
  mov cx,[bp+10]
  mov dx,[bp+12]

intr: int 0x00

  mov ah,0
  pop bp
  ret
```

bootloader.asm

```asm
; bootloader.asm
bits 16

KERNEL_SEGMENT equ 0x1000 ; kernel will be loaded at 0x1000:0x0000
KERNEL_SECTORS equ 15     ; kernel will be loaded in 15 sectors maximum
KERNEL_START   equ 1      ; kernel will be loaded in sector 1

; bootloader code
bootloader:
  ; load kernel to memory
  mov ax, KERNEL_SEGMENT    ; load address of kernel
  mov es, ax                ; buffer address are in ES:BX
  mov bx, 0x0000            ; set buffer address to KERNEL_SEGMENT:0x0000

  mov ah, 0x02              ; read disk sectors
  mov al, KERNEL_SECTORS    ; number of sectors to read

  mov ch, 0x00              ; cylinder number
  mov cl, KERNEL_START + 1  ; sector number
  mov dh, 0x00              ; head number
  mov dl, 0x00              ; read from drive A

  int 0x13                  ; call BIOS interrupts

  ; set up segment registers
  mov ax, KERNEL_SEGMENT
  mov ds, ax
  mov es, ax
  mov ss, ax

  ; set up stack pointer
  mov ax, 0xFFF0
  mov sp, ax
  mov bp, ax

  ; jump to kernel
  jmp KERNEL_SEGMENT:0x0000

  ; padding to make bootloader 512 bytes
  times 510-($-$$) db 0
  dw 0xAA55
```

std_lib.h

```c
#include "std_type.h"

int div(int a, int b);
int mod(int a, int b);

void memcpy(byte *src, byte *dst, unsigned int size);
unsigned int strlen(char *str);
bool strcmp(char *str1, char *str2);
bool strncmp(char *str1, char *str2, unsigned int check);
void strcpy(char *src, char *dst);
void clear(byte *buf, unsigned int size);
```

std_type.h

```c
#ifndef __STD_TYPE_H__
#define __STD_TYPE_H__

typedef unsigned char byte;

#ifndef bool
typedef char bool;
#define true 1
#define false 0
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* __STD_TYPE_H__ */
```

kernel.h

```c
extern void putInMemory(int segment, int address, char character);
extern int interrupt(int number, int AX, int BX, int CX, int DX);

void printString(char *str);
void readString(char *buf);
void clearScreen();
```

kernel.c

```c
#include "kernel.h"
#include "std_lib.h"
#include "std_type.h"

// Masukkan variabel global dan prototipe fungsi di sini
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define SHELL_OFFSET 3

void clearScreen();
void printString(char *str);
void readString(char *buf);

void intToStr(unsigned int num, char *str);

void updateCursorPos();
void putChar(char c);

void handleEcho(char *buf, char *charBuf, bool print);
void handleGrep(char *buf, char *prevBuf, char *outBuf, bool print);
void handleWc(char *buf, char *prevBuf);

unsigned int cursorRow = 0;
unsigned int cursorCol = 0;

int main() {
  char buf[128];

  char *echoPart;
  char *grepPart;
  char *wcPart;

  unsigned int i = 0;

  int firstPipe = -1;
  int secondPipe = -1;

  unsigned int wcOrGrep = 0;

  unsigned int pipeCount = 0;
  unsigned int stringLen = 0;

  char echoBuf[128];
  char grepBuf[128];

  interrupt(0x10, 0x0100, 0, (0x06 << 8) | 0x07, 0);

  clearScreen();
  printString("LilHabOS - B06");
  printString("\n");

  while (1) {

    wcOrGrep = 0;
    pipeCount = 0;
    stringLen = 0;
    firstPipe = -1;
    secondPipe = -1;
    clear((byte *)grepBuf, 128);
    clear((byte *)echoBuf, 128);

    printString("$> ");
    readString(buf);

    if (strlen(buf) > 0) {
      // checks for echo
      stringLen = strlen(buf);

      for (i = 0; i < strlen(buf); i++) {
        if (buf[i] == '|') {
          pipeCount++;
        }
      }

      if (pipeCount == 0) {
        echoPart = buf;
      }

      if (pipeCount == 1) {
        for (i = 0; buf[i] != '\0'; i++) {
          if (buf[i] == '|') {
            buf[i - 1] = '\0';
            echoPart = buf;

            if (strncmp(buf + i + 2, "grep", 4) == 1) {
              grepPart = buf + i + 2;
              wcOrGrep = 1; // grep
            }

            if (strncmp(buf + i + 2, "wc", 2) == 1) {
              wcPart = buf + i + 2;
              wcOrGrep = 2;
            }
          }
        }

        if (wcOrGrep == 1) {
          grepPart[i] = '\0';
        } else {
          wcPart[i] = '\0';
        }
      }

      if (pipeCount == 2) {

        for (i = 0; buf[i] != '\0'; i++) {
          if (buf[i] == '|') {
            if (firstPipe == -1) {
              echoPart = buf;

              grepPart = buf + i + 2;
              firstPipe = i;
            } else {
              wcPart = buf + i + 2;
              secondPipe = i;
            }
          }
        }

        buf[firstPipe - 1] = '\0';
        buf[secondPipe - 1] = '\0';
        buf[i] = '\0';
      }
    }
    // checks echo

    if (pipeCount == 0) {
      handleEcho(echoPart, echoBuf, 1);
    }

    else if (pipeCount == 1 && wcOrGrep == 1) {
      handleEcho(echoPart, echoBuf, 0);
      handleGrep(grepPart, echoBuf, grepBuf, 1);
    }

    else if (pipeCount == 1 && wcOrGrep == 2) {
      handleEcho(echoPart, echoBuf, 0);
      handleWc(wcPart, echoBuf);
    }

    else if (pipeCount == 2) {
      handleEcho(echoPart, echoBuf, 0);
      handleGrep(grepPart, echoBuf, grepBuf, 0);
      handleWc(wcPart, grepBuf);
    }
  }
}

// Masukkan fungsi di sini

void handleEcho(char *buf, char *outBuf, bool print) {
  unsigned int len;
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int k = 0;

  bool hasOpeningQuote = false;
  bool hadClosingQuote = false;

  len = strlen(buf);

  clear((byte *)outBuf, 128);

  if (strncmp(buf, "echo", 4) == 0 && buf[4] != ' ') {
    return;
  }

  while (buf[i] != '\0') {
    if (buf[i] == '"') {
      hasOpeningQuote = true;
      i++;
      break;
    }
    i++;
  }

  if (hasOpeningQuote == false) {
    if (print == 1) {
      printString("Error: Missing operator\n");
    }

    return;
  }

  k = i;
  while (buf[i] != '\0') {
    if (buf[i] == '"') {
      hadClosingQuote = true;
      break;
    }

    i++;
  }

  if (hadClosingQuote == false) {
    if (print == 1) {
      printString("Error: Missing closing quote\n");
    }

    return;
  }

  i = k;

  while (buf[i] != '\0' && buf[i] != '"' && j < 127) {
    if (buf[i] == '\\') {
      i++;

      if (buf[i] == 'n') {
        outBuf[j++] = '\n';
        i++;
      } else {
        outBuf[j++] = '\\';

        if (buf[i] != '\0') {
          outBuf[j++] = buf[i++];
        }
      }
    } else {

      outBuf[j++] = buf[i++];
    }
  }

  outBuf[j] = '\0';

  if (print) {
    printString(outBuf);
    printString("\n");
  }
}

void handleGrep(char *buf, char *prevBuf, char *outBuf, bool print) {
  unsigned int len = 0;
  bool foundAny = false;
  bool match = false;
  bool isMatch = false;
  unsigned int lineLen;
  unsigned int lineBufLen;
  unsigned int inputLen;
  unsigned int outBufLen;
  unsigned int prevBufLen = 0;
  unsigned int keywordLen;
  char line[128];
  char keywordGrep[128];
  unsigned int lineIdx = 0;
  unsigned int i;
  unsigned int j;
  unsigned int k;
  unsigned int keywordStart = 5;
  bool insideQuote = false;

  clear((byte *)outBuf, 128);
  len = strlen(buf);
  prevBufLen = strlen(prevBuf);

  if (strncmp(buf, "grep", 4) == 0 && buf[4] != ' ') {
    return;
  }

  if (prevBufLen < 1) {
    printString("NULL\n");
    return;
  }

  keywordLen = 0;
  for (i = keywordStart; i < len && keywordLen < 127; i++) {
    if (buf[i] == ' ' || buf[i] == '\0') {
      break;
    }

    keywordGrep[keywordLen++] = buf[i];
  }

  keywordGrep[keywordLen] = '\0';

  if (keywordLen == 0) {
    if (print == 1) {
      printString("NULL\n");
    }

    return;
  }

  for (i = 0; i <= prevBufLen; i++) {
    if (prevBuf[i] != '\0' && prevBuf[i] != '\n') {
      if (lineIdx < 127) {
        line[lineIdx++] = prevBuf[i];
      }
    } else {
      line[lineIdx] = '\0';
      match = false;

      for (j = 0; j <= (strlen(line) - keywordLen); j++) {
        isMatch = true;
        for (k = 0; k < keywordLen; k++) {
          if (line[j + k] != keywordGrep[k]) {

            isMatch = false;
            break;
          }
        }

        if (isMatch == true) {
          foundAny = true;
          match = true;
          break;
        }
      }

      if (match == 1 && print == 1) {
        printString(line);
        printString("\n");
        updateCursorPos();
      }

      if (match == 1 && print == 0) {
        lineLen = strlen(line);
        outBufLen = strlen(outBuf);

        memcpy((byte *)line, (byte *)outBuf + outBufLen, lineLen);

        outBuf[outBufLen + lineLen] = '\n';
        outBuf[outBufLen + lineLen + 1] = '\0';
      }

      lineIdx = 0;
      clear((byte *)line, 128);
    }
  }

  if (foundAny == false && print == 1) {
    printString("NULL\n");
  }
}

void intToStr(unsigned int num, char *str) {
  unsigned int i = 0;
  unsigned int j;
  char temp[6];

  if (num == 0) {
    str[0] = '0';
    str[1] = '\0';
    return;
  }

  while (num > 0 && i < 5) {
    temp[i++] = '0' + mod(num, 10);
    num = div(num, 10);
  }

  for (j = 0; j < i; j++) {
    str[j] = temp[i - j - 1];
  }

  str[i] = '\0';
}

void handleWc(char *buf, char *prevBuf) {
  unsigned int len = 0;
  unsigned int lines = 0;
  unsigned int words = 0;
  unsigned int chars = 0;
  unsigned int bufLen = 0;
  bool insideWord = false;
  char lineStr[6];
  char wordStr[6];
  char charsStr[6];
  unsigned int i;

  len = strlen(buf);

  if (strncmp(buf, "wc", 2) == 0 && buf[2] != '\0') {
    return;
  }

  bufLen = strlen(prevBuf);
  if (bufLen < 1) {
    printString("NULL\n");
    return;
  }

  for (i = 0; i < bufLen; i++) {
    if (buf[i] != '\n') {
      chars++;
    }

    if (prevBuf[i] == '\n') {
      lines++;
    }

    if (prevBuf[i] != ' ' &&
        (i == 0 || prevBuf[i - 1] == ' ' || prevBuf[i - 1] == '\n') &&
        !insideWord) {
      insideWord = true;
      words++;
    } else {
      insideWord = false;
    }
  }

  if (chars > 0 && prevBuf[chars - 1] != '\n') {
    lines++;
  }

  intToStr(lines, lineStr);
  intToStr(words, wordStr);
  intToStr(chars, charsStr);

  printString(lineStr);
  printString(" ");
  printString(wordStr);
  printString(" ");
  printString(charsStr);
  printString("\n");
}

void updateCursorPos() {
  int pos = (cursorRow * SCREEN_WIDTH + cursorCol);
  interrupt(0x10, 0x0200, 0, 0, pos);
}

void putChar(char c) {
  int offset = 0;

  offset = 2 * (cursorRow * SCREEN_WIDTH + cursorCol);

  putInMemory(0xB800, offset, c);
  putInMemory(0xB800, offset + 1, 0x0F);

  cursorCol++;

  if (cursorCol >= SCREEN_WIDTH) {
    cursorCol = 0;
    cursorRow++;
  }

  if (cursorRow >= SCREEN_HEIGHT) {
    clearScreen();
  }
}

void printString(char *str) {
  int i = 0;
  while (str[i] != '\0') {
    if (str[i] == '\n') {
      cursorRow++;

      if (cursorRow >= SCREEN_HEIGHT) {
        clearScreen();
      }
      cursorCol = 0;
    } else {
      putChar(str[i]);
    }

    i++;
  }

  updateCursorPos();
}

void clearScreen() {
  int i;
  for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    putInMemory(0xB800, i * 2, ' ');
    putInMemory(0xB800, i * 2 + 1, 0x0F);
  }

  cursorCol = 0;
  cursorRow = 0;

  interrupt(0x10, 0x06 << 8 | 0x00, 0x00, 0x00, 0x18 << 8 | 0x4F);

  interrupt(0x10, 0x02 << 8 | 0x00, 0, 0, 0);

  updateCursorPos();
}

void readString(char *buf) {
  int i = 0;
  char c = 0;
  clear((byte *)buf, 128);

  while (1) {
    c = interrupt(0x16, 0x0000, 0, 0, 0) & 0xFF;

    if (c == '\r') {
      buf[i] = '\0';
      printString("\n");
      break;
    } else if (c == '\b') { // Backspace
      if (i > 0 && cursorCol > SHELL_OFFSET) {
        i--;
        cursorCol--;
        updateCursorPos();
        putChar(' ');
        cursorCol--;
        updateCursorPos();
      }
    } else {
      buf[i++] = c;
      putChar(c); // Echo character
    }
  }
}
```

Makefile

```makefile
SRC = src
BIN = bin

NASM = nasm
BCC = bcc
LD86 = ld86
DD = dd

prepare:
	mkdir -p $(BIN)
	$(DD) if=/dev/zero of=$(BIN)/floppy.img bs=512 count=2880 conv=notrunc

bootloader:
	$(NASM) -f bin $(SRC)/bootloader.asm -o $(BIN)/bootloader.bin

stdlib:
	$(BCC) -Iinclude -ansi -c $(SRC)/std_lib.c -o $(BIN)/std_lib.o

kernel:
	$(BCC) -Iinclude -ansi -c $(SRC)/kernel.c -o $(BIN)/kernel.o
	$(NASM) -f as86 $(SRC)/kernel.asm -o $(BIN)/kernel_asm.o

link:
	$(LD86) -o $(BIN)/kernel.bin -d $(BIN)/kernel.o $(BIN)/kernel_asm.o $(BIN)/std_lib.o
	$(DD) if=$(BIN)/bootloader.bin of=$(BIN)/floppy.img bs=512 conv=notrunc
	$(DD) if=$(BIN)/kernel.bin of=$(BIN)/floppy.img bs=512 seek=1 conv=notrunc

build: prepare bootloader stdlib kernel link

run:
	bochs -f bochsrc.txt
```

**- Explanation**

Sebelum masuk ke soal, diminta di awal task-4 untuk mengsetup struktur direktori yang sudah ditentukan, dengan direktori `bin` untuk menyimpan hasil compile dari file-file yang terdapat di direktori `include` dan `src`. Kemudian masing-masing file `include/std_type.h`, `include/std_lib.h`, `include/kernel.h`, `src/kernel.c`, `src/bootloader.asm`, `src/kernel.asm`, `src/kernel.c` dan `src/std_lib.c` diisi dengan kode default yang diberikan dalam task-4 dan sebagian dari modul 4 Sistem Operasi (untuk bootloader.asm).

Kemudian dalam subsoal a diberikan perintah untuk menerapkan implementasi fungsi `printString`, `clearScreen` dan `readString` di file `src/kernel.c`. Untuk pengimplementasian masing-masing fungsi pertama dimulai dari `readString`. Fungsi `readString` bekerja secara continous (infinite loop) yang setiap loop-nya akan mengecek apakah keyboard sudah menekan suatu key atau belum dengan memanfaatkan BIOS interupt call `16h` dengan AH `00h` yang berarti `Read Character`. Interrupt dicek setiap loop melalui bantuan fungsi `interrupt` yang didefinisikan di `kernel.asm`. Setelah terdapat interrupt kalau ada tombol keyboard yang ditekan, nilai atau value dari tombol yang ditekan disimpan di sebuah variabel char `c` yang nantinya diproses apakah karakter yang disimpan adalah backspace, return, atau alfanumerik. Kemudian setiap karakter akan disimpan di buffer `buf` yang memiliki size 128 yang setiap terdapat karakter yang terdeteksi akan langsung dimasukkan ke buf (tidak mengganti, namun seperti di queue dari belakang). Jika karakter yang ditekan adalah karakter `\n` maka terindikasi sebuah line sudah selesai diketik dan penulisan buffer pun berhenti. Apabila yang ditekan adalah karakter alfanumerik biasa akan meng-invoke fungsi `putChar` yang kami definisikan sendiri untuk mencetak sebuah karakter pada layar bosch. Untuk menghindari konflik cursor untuk peletakan karakter di layar bosch, didefinisikan 2 variabel untuk meng-track masing-masing posisi cursor di koordinat X dan Y relatif terhadap space screen (80 x 25). Setiap fungsi `putChar` dipanggil akan mengupdate koordinat X dan Y untuk posisi cursor dengan memanggil fungsi custom lain bernama `updateCursorPos` yang meng-interrupt sistem agar menempatkan cursor pada posisi yang tepat dengan menggunakan BIOS interrupt call `10h` dan AH `02h` untuk mengset cursor position.

Kemudian untuk implementasi fungsi `printString` akan diberi sebuah parameter berupa pointer ke sebuah string yang diakhiri oleh karakter NULL untuk menandai batas string. Fungsi `printString` akan meng-loop satu-persatu karakter yang di-pass di argumen fungsi ketika dipanggil hingga menemukan karakter `\0` atau NULL. Apabila karakter pada posisi ke-i adalah karakter alfanumerik biasa maka dipanggil fungsi `putChar` untuk menampilkan karakter tersebut ke layar. Apabila bertemu dengan karakter return `\n` maka variabel global Y akan di increment sampai batas SCREEN_HEIGHT. Apabila melampaui batas, maka layar akan di clear atau di reset menggunakan fungsi `clearScreen`.

Untuk fungsi `clearScreen`, untuk setiap karakter yang sudah ditampilkan dari pojok kiri atas sampai pojok kanan bawah akan di-overwrite dengan karakter space (spasi) dengan menggunakan fungsi `putInMemory` yang didefinisikan di file `kernel.asm`. Kemudian fungsi ini akan meng-interrupt sistem untuk memposisikan kursor ke pojok kiri atas dengan memanggil 2 BIOS interrupt call, `06h` dan `02h` masing-masing untuk scroll up dan posisi kursor menggunakan fungsi `interrupt`.

// insert gambar di sini

Kemudian di subsoal b diberikan perintah untuk melengkapi dan mengimplementasi fungsi-fungsi yang didefinisikan di dalam `std_lib.h` dan `std_lib.c`. Beberapa fungsi tersebut adalah `mod`, `div`, `strcmp`, `memcpy`, dll. Disini kami menambahkan satu fungsi baru yaitu `strncmp` yang membandingkan string hingga urutan ke-n. Implementasi nya mirip seperti `strcmp` yang sudah diimplementasikan sebelumnya, fungsi ini nantinya akan membantu dalam mengerjakan subsoal c dan d.

// insert gambar disini

Kemudian di subsoal c diberikan perintah untuk mengimplementasi command `echo`. Pengimplementasian dilakukan dengan memanggil fungsi kustom `handleEcho`. Namun sebelum itu, di fungsi `main` terlebih dahulu perlu untuk menghitung berapa banyak karakter pipe `|` yang berada dalam buffer string input-an user. Jika terdapat 0 pipe, maka command yang akan dijalankan adalah echo, jika terdapat 1 pipe, maka antara kombinasi `echo + grep` atau `echo + wc`, dan jika terdapat 2 pipe, maka kombinasinya adalah `echo + grep + wc`. Fungsi `handleEcho` adalah fungsi yang menerima buffer dari pembacaan input-an user sampai user menekan tombol enter. Disini akan dicek 4 karakter awal buffer apakah mengandung kata `echo`. Jika tidak maka fungsi `handleEcho` tidak dijalankan. Kemudian untuk meng-ekstrak string yang di-pass sebagai argumen dari command echo, pengecekan di lakukan mulai dari indeks ke-5 dari buffer dan mengecek apakah terdapat karakter `"` di dalam buffer. Karakter ini akan mengindikasikan awalan dan akhiran dari string yang akan diproses oleh fungsi `handleEcho`. Apabila salah satu karakter `"` tidak lengkap, maka fungsi ini mengembalikan error `Error: Missing operator\\n`. Jika string didapat, maka di-loop satu-persatu dan dimasukkan satu-persatu kedalam variabel baru `outBuf` yang akan menampung karakter-karakter dalam string yang nantinya akan di-print ke layar atau digunakan dalam piping fungsi selanjutnya.

// insert gambar disini

Kemudian di subsoal c diberikan perintah untuk mengimplementasi command `grep`. Pengimplementasian dilakukan dengan memanggil fungsi kustom `handleGrep` yang menerima parameter berupa buffer dari fungsi `handleEcho`. Kemudian fungsi grep juga menerima input-an tersendiri untuk keyword. Untuk menentukan darimana substring milik command grep maka ditentukan dari awal jumlah karakter `|` yang dimasukkan dalam buffer string awal dan mencari di indeks ke berapa setalah karakter tersebut ditemukan. Apabila ditemukan karakter ke i + 1 dan seterusnya adalah bagian dari command grep. Untuk keyword dipastikan selalu berada di posisi `i + 5` dari awal kata grep dan berakhir sebelum ditemukan karakter spasi. Kemudian untuk mendeteksi adanya kata atau substring yang mengandung keyword dilakukan dengan 2 nested loop, loop pertama akan mengiterasi dari i = 0 sampai panjang string buffer yang dihasilkan oleh fungsi `handleEcho` dikurangi dengan panjang keyword `(panjang line - panjang keyword)`. Apabila karakter awal tidak sama dengan karakter awal keyword maka akan diskip, dan apabila sama maka akan dilakukan looping ke-2 untuk memastikan bahwa buffer tersebut memiliki substring keyword. Kemudian buffer yang terdeteksi mengandung keyword didalamnya akan dimasukkan ke buffer lain bernama `outBuf` dengan menggunakan fungsi `memcpy` untuk meng-copy string kedalam variabel `outBuf`. Variabel ini akan digunakan untuk fungsi selanjutnya atau hanya untuk mencetak line yang terdapat keyword didalamnya.

// insert gambar disini

Kemudian di subsoal d diberikan perintah untuk mengimplementasi command `wc` atau wordcount. Pengimplementasian dilakukan dengan memanggil fungsi kustom `handleWc` yang menerima buffer dari fungsi sebelumnya antara `handleGrep` atau `handleEcho`. Untuk menghitung jumlah karakter maka buffer yang dipass ke argumen `handleWc` akan diloop satu-persatu dan setiap iterasi variabel `chars` akan di-increment sebanyak 1. Kemudian untuk mendeteksi kata maka digunakan sebuah variabel bantu `insideWord` yang diinisialisasi dengan nilai `false`, apabila dalam setiap iterasi karakter sebelum indeks ke-i adalah karakter spasi atau `\\n` atau i == 0, dan karakter ke-i bukan karakter spasi maka variabel `words` di-increment. Ini menandakan substring tersebut berada didalam suatu kata. Kemudian apabila terdeteksi karakter `\\n` didalam string maka variabel `lines` di-increment, dan apabila tidak ditemukan karakter `\\n` dimanapun kecuali di akhir string, maka variabel `lines` di-increment 1 kali. Kemudian untuk mencetak angka kedalam fungsi printString, dibutuhkan suatu fungsi baru untuk mengubah bentuk angka menjadi bentuk angka yang string dengan memanggil fungsi kustom `intToStr` yang mengubah bilangan dalam bentuk integer menjadi bentuk stringnya dengan menggunakan fungsi `div` dan `mod` serta buffer baru untuk menampung integer yang dikonversi.

// insert gambar disini

Kemudian di subsoal e diberikan perintah untuk mengisi file `makefile` untuk mempermudah proses development dari OS kita. Terdapat beberapa command makefile yang harus diisi diantaranya adalah `prepare`, yakni membuat direktori bin dan membuat file `floppy.img` ke dalam direktori bin menggunakan command `dd`. Kemudian terdapat command `bootloader` yang mengcompile bootloader.asm menjadi file binary. Kemudian terdapat command `stdlib` yang berfungsi untuk mengcompile file std_lib.c dengan `bcc` dan meng-include file-file header yang didefinisikan didalam file tersebut dengan tambahan flag `-Iinclude`. Kemudian terdapat command `kernel` yang berfungsi untuk mengcompile file kernel.c dengan bcc dan juga menggunakan flag yang sama pada command `stdlib` serta mengcompmile kernel.asm dengan menggunakan `nasm`. Kemudian terdapat command `link` dimana berfungsi untuk menghubungkan file-file yang sudah dicompile sebelumnya dan dipaste kedalam direktori bin kedalam satu file `floppy.img`. Linking dilakukan dengan memanfaatkan command `ld86` serta `dd`. Kemudian terdapat command `build` yang digunakan untuk memanggil command-command yang sudah didefinisikan sebelumnya dari `prepare` sampai `link`. Kemudian terdapat command terakhir yakni `run` dimana berfungsi untuk menjalankan bosch dengan konfigurasi di file `bochsrc.txt`.
