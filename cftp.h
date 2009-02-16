#ifndef __CFTP_H
#define __CFTP_H

/*
 * Public protocol API for CFTP. Any function defined in here will be available
 * in any part of the application that needs it.
 *
 */
#define MSGLEN 128
#define ERRMSGLEN 256
#define CMDLEN 5
int validate_command(const char *);
void split_command(char *, char *, char *);
void error(const char *);
void debug(const char *, const char *);

#endif

