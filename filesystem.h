#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H

/*
 * This is the public API for CFTP's filesystem access. You just have to
 * ask it to send or receive a file, and this will handle the rest. Errors
 * raised if not accessible or if you are trying to overwrite a file. No files
 * may be overwritten. All files written to client or server are saved in /tmp.
 *
 */

/*
 * The MAXFILE size is calculated to be the largest integer in CMDLEN - 1
 * characters times MSGLEN. MAXFILE must be a multiple of MSGLEN.
 *
 * 9999 segments * 128 bytes = 1279872 bytes maximum for example
 *
 */
#define MAXFILE 1279872
void send_file(int, const char *, int);
void receive_file(int, const char *, int);

#endif

