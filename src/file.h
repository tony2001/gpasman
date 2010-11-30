#ifndef FILE_H
#define FILE_H

/* 
 * one entry is a gchar *entry[4]
 * this means we have 4 pointers to a gchar -->
 * 1 = server
 * 2 = username
 * 3 = password
 * 4 = comment
 */

int save_init(char *filename);
int save_entry(char *entry[4]);
int save_finalize(void);
int load_init(char* filename);
int load_entry(char *entry[4]);
int load_finalize(void);
int check_file (char *filename);
int file_exists (char *tfile);
void expand_key(char *password);

#endif	/* FILE_H */
