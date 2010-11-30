/* Gpasman, a password manager
   (C) 1998-1999 Olivier Sessink <olivier@lx.student.wau.nl>
   (C) 2003 - T. Bugra Uytun <t.bugra@uytun.com>
	http://gpasman.sourceforge.net

   file.c, handles file opening and closing

   Other code contributors:
   Dave Rudder, Chris Halverson, Matthew Palmer, Guide Berning, Jimmy Mason

   This program is free software; you can redistribute it and/or modify it 
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2 of the License, or (at your
   option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along 
   with this program; if not, write to the Free Software Foundation, Inc., 
   59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

/*
 * before editing this file read the
 * PKCS #5: Password-Based Cryptography Standard
 * which can be found at http://www.rsasecurity.com/rsalabs/pkcs/
 *
 * this code is based on version 1
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include "gpasman.h"
#include "file.h"
#include "rc2.h"

#define SAVE_BUFFER_LENGTH 1024
#define LOAD_BUFFER_LENGTH 20480

#ifndef S_IAMB
#define S_IAMB 00777
#endif



/********************************************************************
 * GLOBAL VARIABLES
 ********************************************************************/

/* these three are needed in all save functions and initialized
 * in save_init() */
FILE *fd;
unsigned short iv[4];
char *buffer;

/* these two are global because save_entry() and save_finalize() both need 
   them */
int bufferIndex;
unsigned short plaintext[4];



/********************************************************************
 * BEGIN OF FUNCTIONS
 ********************************************************************/

/* uses GpasmanFileInitType as return code */
int save_init(char *filename)
{
	int i;

	/* first we should check the permissions of the filename */
	if (g_file_test((const gchar *)filename, G_FILE_TEST_IS_REGULAR)) {
		i = check_file(filename);
		if (i != GPASMAN_FILE_INIT_SUCCESS) {
#ifdef DEBUG
			gpasman_debug("return %d", i);
#endif
			return i;
		}
	}
	else {
		i = creat(filename, (S_IRUSR | S_IWUSR));
		if (i == -1) {
#ifdef DEBUG
			g_warning("Could not create file '%s'", filename);
#endif
			return GPASMAN_FILE_INIT_ERROR;
		}
		else {
			close(i);
		}
	}

	fd = fopen(filename, "wb");
	g_return_val_if_fail(fd, GPASMAN_FILE_INIT_ERROR);

	/* First, we make the IV */
	for (i = 0; i < 4; i++) {
		iv[i] = rand();
		putc((unsigned char) (iv[i] >> 8), fd);
		putc((unsigned char) (iv[i] & 0xff), fd);
	}

	bufferIndex = 0;
	buffer = (char *)g_malloc(SAVE_BUFFER_LENGTH);

	return GPASMAN_FILE_INIT_SUCCESS;
}


int save_entry(char *entry[NUM_COLUMNS])
{
	int count2, count3;
	unsigned short ciphertext[4];

	/* clean up buffer */
	memset(buffer, '\0', SAVE_BUFFER_LENGTH);

	for (count2 = 0; count2 < NUM_COLUMNS; count2++) {
		if(strlen(entry[count2]) == 0)	{
			g_strlcat(buffer, " ", SAVE_BUFFER_LENGTH);
		}
		else	{
			g_strlcat(buffer, entry[count2], SAVE_BUFFER_LENGTH);
		}
		/* Use 255 as the marker.  \n is too tough to test for */
		buffer[strlen(buffer)] = 255;
	}
#ifdef DEBUG
	gpasman_debug("buffer contains '%s' %d", buffer, strlen(buffer));
#endif
	count2 = 0;
	/* I'm using CBC mode and encrypting the data straight from top down.
	   At the bottom, encrypted, I will append an MD5 hash of the file,
	   eventually.  PKCS 5 padding (explained at the code section */
	while (count2 < strlen(buffer)) {
#ifndef WORDS_BIGENDIAN
		plaintext[bufferIndex] = buffer[count2 + 1] << 8;
		plaintext[bufferIndex] += buffer[count2] & 0xff;
#endif
#ifdef WORDS_BIGENDIAN
		plaintext[bufferIndex] = buffer[count2] << 8;
		plaintext[bufferIndex] += buffer[count2 + 1] & 0xff;
#endif
		bufferIndex++;
		
		if (bufferIndex == 4) {
			rc2_encrypt(plaintext);

			for (count3 = 0; count3 < 4; count3++) {
				ciphertext[count3] = iv[count3] ^ plaintext[count3];

				/* Now store the ciphertext as the iv */
				iv[count3] = plaintext[count3];

				/* reset the buffer index */
				bufferIndex = 0;
				if (putc((unsigned char)(ciphertext[count3] >> 8), fd) == EOF)
					return GPASMAN_FILE_INIT_EOF;
				if (putc((unsigned char)(ciphertext[count3] & 0xff), fd) == EOF)
					return GPASMAN_FILE_INIT_EOF;
			}
		}
		/* increment a short, not a byte */
		count2 += 2;
	}
	return GPASMAN_FILE_INIT_SUCCESS;
}



int save_finalize(void)
{
	int count1, retval = 1;
	unsigned short ciphertext[4];

	/* Tack on the PKCS 5 padding How it works is we fill up the last n
	   bytes with the value n

	   So, if we have, say, 13 bytes, 8 of which are used, we have 5 left
	   over, leaving us 3 short, so we fill it in with 3's.

	   If we come out even, we fill it with 8 8s

	   um, except that in this instance we are using 4 shorts instead of 8
	   bytes. so, half everything */
	for (count1 = bufferIndex; count1 < 4; count1++) {
		plaintext[count1] = (4 - bufferIndex);
	}
#ifdef DEBUG
	gpasman_debug("4 - bufferIndex=%d", (4 - bufferIndex));
	gpasman_debug("plaintext[3]=%c", plaintext[3]);
#endif
	rc2_encrypt(plaintext);
	for (count1 = 0; count1 < 4; count1++) {
		ciphertext[count1] = iv[count1] ^ plaintext[count1];
		if (putc((unsigned char) (ciphertext[count1] >> 8), fd) == EOF)
			retval = GPASMAN_FILE_INIT_EOF;
		if (putc((unsigned char) (ciphertext[count1] & 0xff), fd) == EOF)
			retval = GPASMAN_FILE_INIT_EOF;
	}

	fclose(fd);
#ifdef DEBUG
	gpasman_debug("fd is closed and return value is %d", retval);
#endif
	free(buffer);
	return retval;

}


/****************************************************************************
 * Loading Functions
 ****************************************************************************/

int load_init(char *filename)
{
	int i, j;
	unsigned char charbuf[8];

	/* first we should check the file permissions */
	if (g_file_test((const gchar *)filename, G_FILE_TEST_IS_REGULAR)) {
		i = check_file(filename);
		if(i != GPASMAN_FILE_INIT_SUCCESS) {
			return i;
		}
	}
	else {
		return GPASMAN_FILE_INIT_ERROR;
	}

	/* the filemode 'b' is for compatibility with non-POSIX systems */
	fd = fopen(filename, "rb");
	g_return_val_if_fail(fd, GPASMAN_FILE_INIT_ERROR);
	
	/* if 0 then EOF == zero file
	 * if 0 < read < 8 then IV is not initialized correctly while saving */
	if(read(fileno(fd), (unsigned char *)charbuf, 8) < 8)	{
		g_warning("wrong or corrupted gpasman file");
		return GPASMAN_FILE_INIT_BAD_PERMISSION;
	}

	/* restore IV from the first 8 bytes of file */
	for (i = 0; i < 4; i++) {
		j = i << 1;
		iv[i] = charbuf[j] << 8;		/* == K (higher 8bits) */
		iv[i] += charbuf[j + 1];		/* == IV (lower 8bits) */
#ifdef DEBUGload
		gpasman_debug("iv[%d]=%d", i, iv[i]);
#endif
	}

	/* initializing buffer */
	bufferIndex = 0;
	buffer = (char *)g_malloc0(8);
	return GPASMAN_FILE_INIT_SUCCESS;
}


/* the memory for 'entry' will be allocated here and must be freed in the
 * original application if not used anymore */
int load_entry(char *entry[NUM_COLUMNS])
{
	unsigned char charbuf[8];		/* container for reading from file */
	int i, j;						/* tmp values */
	int e_count = 0;				/* entry counter */
	int e_index = 0;				/* entry indexer */
	unsigned short ciphertext[4];

	/* allocating memory for entries */
	for(i = 0; i < NUM_COLUMNS; i++)	{
		entry[i] = g_malloc0(LOAD_BUFFER_LENGTH);
	}

	/* if the buffer is not 0 then this is not the first call
	 * that means i have to begin splitting the buffer here
	 * before reading from the file */
	if(buffer[0] != '\0')	{
		while(bufferIndex < 8)	{
			if((unsigned char)(buffer[bufferIndex]) == 255)	{
				e_count++;
				e_index = 0;
			}
			else	{
				/* don't ask me why i have to check if it's != '\0' but
				 * if don't do this it wont work!!!! */
				if(buffer[bufferIndex] != '\0')	{
					memcpy(&((entry[e_count])[e_index]), &(buffer[bufferIndex]), 1);
#ifdef DEBUGload
					gpasman_debug("1: e_count=%d, e_index=%d, bufferIndex=%d,"
							"entry=%s, buffer=%s",
							e_count, e_index, bufferIndex,
							entry[e_count], buffer);
#endif
					e_index++;
				}
			}
			bufferIndex++;
		}
	}

	/* cleaning out the buffer */
	memset(buffer, '\0', 8);

	while((read(fileno(fd), (unsigned char *)charbuf, 8)) > 0) {
		/* decrypting */
		for (i = 0; i < 4; i++) {
			j = i << 1;
			ciphertext[i] = charbuf[j] << 8;
			ciphertext[i] += charbuf[j+1];

			plaintext[i] = ciphertext[i] ^ iv[i];
			iv[i] = plaintext[i];
		}
		rc2_decrypt(plaintext);
		memcpy(buffer, plaintext, 8);

		/* spilitting up into etries */
		for(bufferIndex = 0; bufferIndex < 8; bufferIndex++)	{
			if((unsigned char)(buffer[bufferIndex]) == 255)	{
				e_count++;
				e_index = 0;
#ifdef DEBUGload
				gpasman_debug("e_count=%d", e_count);
#endif
				if(e_count == NUM_COLUMNS)	{
					/* the index must be increased for the next call */
					bufferIndex++;
					
					/* i've completed */
					return GPASMAN_FILE_INIT_SUCCESS;
				}
			}
			else	{
				/* don't ask me why i have to check if it's != '\0' but
				 * if don't do this it wont work!!!! */
				if(buffer[bufferIndex] != '\0')	{
					memcpy(&((entry[e_count])[e_index]), &(buffer[bufferIndex]), 1);
#ifdef DEBUGload
					gpasman_debug("2: e_count=%d, e_index=%d, bufferIndex=%d,"
							"entry=%s, buffer=%s",
							e_count, e_index, bufferIndex, entry[e_count], buffer);
#endif
					e_index++;
				}
			}
		}
	}

#ifdef DEBUGload
	gpasman_debug("ended no entries anymore%s", "\0");
#endif
	return GPASMAN_FILE_INIT_LAST_LOADED;
}


int load_finalize(void)
{
	fclose(fd);
	free(buffer);
	return GPASMAN_FILE_INIT_SUCCESS;
}


/* uses GpasmanFileInitType as return code */
int check_file(char *filename)
{
	struct stat naamstat;

	if (stat(filename, &naamstat) == -1) {
		return GPASMAN_FILE_INIT_BAD_STATUS;
	}

	if (((naamstat.st_mode & S_IAMB) | (S_IRUSR | S_IWUSR))
			!= (S_IRUSR | S_IWUSR)) {
#ifdef DEBUGload
		gpasman_debug("%s perms are bad, they are: %d, should be -rw------",
				filename, (naamstat.st_mode & (S_IREAD | S_IWRITE)));
#endif
		return GPASMAN_FILE_INIT_BAD_PERMISSION;
	}

	if (!S_ISREG(naamstat.st_mode)) {
		lstat(filename, &naamstat);
		if (S_ISLNK(naamstat.st_mode)) {
#ifdef DEBUGload
			gpasman_debug("%s is a symlink", filename);
#endif
			return GPASMAN_FILE_INIT_SYMLINK;
		}
	}

	return GPASMAN_FILE_INIT_SUCCESS;
}

void expand_key(char *password)	{
	unsigned char key[GPASMAN_RC2_MAX_KEY_SIZE];
	int i;

#ifdef DEBUGload
	gpasman_debug("password='%s'", password);
#endif
	for(i = 0; password[i] != '\0'; i++)	{
		key[i] = password[i];
	}
	rc2_expandkey(key, i, GPASMAN_RC2_MAX_KEY_SIZE);
}

