#ifndef GPASMAN_H
#define GPASMAN_H

/* used to define return values of file reading writing operations while
 * saving and reading passwd data. */
typedef enum {
	GPASMAN_FILE_INIT_EOF = -4,			/* -4 = premature EOF */
	GPASMAN_FILE_INIT_BAD_STATUS,		/* -3 = can't get file status */
	GPASMAN_FILE_INIT_SYMLINK,			/* -2 = is a symlink */
	GPASMAN_FILE_INIT_BAD_PERMISSION,	/* -1 = permissions are bad */ 
	GPASMAN_FILE_INIT_ERROR,			/* 0 = can't open filedescriptor / can't create file */
	GPASMAN_FILE_INIT_SUCCESS,			/* 1 = success */
	GPASMAN_FILE_INIT_LAST_LOADED		/* 2 = success - last entry loaded */
} GpasmanFileInitType;

/* used to define the entries while saving and reading passwd data.
 * used with the variable `entry' - which is not a global variable. */
typedef enum {
	COLUMN_HOST,
	COLUMN_USER,
	COLUMN_PASSWD,
	COLUMN_COMMENT,
	NUM_COLUMNS
} GpasmanColumnType;

#define GPASMAN_MIN_PWD_LENGTH 4
#define GPASMAN_MAX_PWD_LENGTH 99

#define GPASMAN_RC2_MAX_KEY_SIZE 128

/* the debug output format is as follow (i hope it works on all systems):
 * <program_name:PID>: DEBUG: +<line> <filename>, <function_name>: <user_output>
 */

//G_LOG_DOMAIN = NULL;
#define gpasman_debug(str, ...)		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, \
									"+%d %s, %s: " str, __LINE__, __FILE__, \
									__FUNCTION__, __VA_ARGS__)


#endif /* GPASMAN_H */
