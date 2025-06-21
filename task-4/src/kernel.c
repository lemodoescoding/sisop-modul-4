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

  updateCursorPos();
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
