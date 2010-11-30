#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <rc2.h>

void rc2(void);
void derc2(void);

int main(int argc, char *argv[])
{
	unsigned int i, j = 0, keypiece, keylength;
	unsigned char temp[3], key[128];

	if (argc < 3) {
		exit(0);
	}
	if (!strncmp("0x",argv[1],2)) {
		for (i = 2; argv[1][i] != '\0'; j++) {
			temp[0] = argv[1][i++];
			temp[1] = argv[1][i++];
			temp[2] = '\0';
			keypiece = (int) strtol(temp, NULL, 16);
			key[j] = (char) keypiece;		
		}
	} else {
		for (j = 0; argv[1][j] != '\0'; j++) {
			key[j] = argv[1][j];
		}
	}
	keylength = j;
	rc2_expandkey(key, keylength, atoi(argv[2]));
	if (!strcmp(argv[0],"rc2")) {
		rc2();
	} else if (!strcmp(argv[0],"derc2")) {
		derc2();
	} else {
	}
	return 0;
}

void rc2()
{
	unsigned char charbuf[8];
	unsigned short text[4];
	unsigned int count, i;
	
	do {
		count = read(fileno(stdin),charbuf,8);
		if (count == 0) {
			exit(0);
		}
		if (count < 8) {
			for (i = count; i < 8; i++) {
				charbuf[i] = '\0';
			}
		}
		for (i = 0; i < 4; i++) {
			text[i] = charbuf[i * 2] + 256 * charbuf[i * 2 + 1];
		}
		rc2_encrypt(text);
		for (i = 0; i < 4; i++) {
			charbuf[i * 2] = text[i] & 0xff;
			charbuf[i * 2 + 1] = text[i] >> 8;
		}
		for (i = 0; i < 8; i++) {
			putchar((int) charbuf[i]);
		}
	} while (count == 8);
	exit(0);
}

void derc2()
{
	unsigned char charbuf[8];
	unsigned short text[4];
	unsigned int count, i;
	
	do {
		count = read(fileno(stdin),charbuf,8);
		if (count == 0) {
			exit(0);
		}
		if (count < 8) {
			for (i = count; i < 8; i++) {
				charbuf[i] = '\0';
			}
		}
		for (i = 0; i < 4; i++) {
			text[i] = charbuf[i * 2] + 256 * charbuf[i * 2 + 1];
		}
		rc2_decrypt(text);
		for (i = 0; i < 4; i++) {
			charbuf[i * 2] = text[i] & 0xff;
			charbuf[i * 2 + 1] = text[i] >> 8;
		}
		for (i = 0; i < 8; i++) {
			putchar((int) charbuf[i]);
		}
	} while (count == 8);
	exit(0);
}
