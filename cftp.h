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
void split_command(const char *, char *, char *);
void error(const char *);
void debug(const char *, const char *);
int send_packet(int, const char *);
int receive_packet(int, char *);
void send_message(int, char *);

#endif

