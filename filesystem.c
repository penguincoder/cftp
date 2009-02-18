#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>

#include "cftp.h"
#include "filesystem.h"

/*
 * Checks if a file exists. Required for send, causes receive to fail if this
 * returns true. Returns int.
 */
int exist(const char *filename)
{
  struct stat sbuf;
  if(stat(filename, &sbuf) == -1 && (errno == ENOENT || errno == EACCES))
  {
    return 0;
  }
  return 1;
}

/*
 * Returns the size of the requested file. Takes a string of the filename,
 * expects the file to exist, too.
 *
 */
int filesize(const char *filename)
{
  struct stat fstat;
  if(stat(filename, &fstat) != -1)
  {
    return fstat.st_size;
  }
  return 0;
}

/*
 * This function will send a file to a remote host. Expects a socket integer
 * and a string containing the full path to the local file name. File cannot
 * exceed MAXFILE definition.
 *
 */
void send_file(int skt, const char *filename)
{
  char errmsg[ERRMSGLEN], cmd[CMDLEN], arg[MSGLEN], msg[MSGLEN];
  char *basefname;
  int fsize = 0, segments = 0, cur_segment;
  FILE *infile;
  
  memset(errmsg, '\0', ERRMSGLEN);
  memset(cmd, '\0', CMDLEN);
  memset(arg, '\0', MSGLEN);
  memset(msg, '\0', MSGLEN);
  
  /* file must exist */
  if(!exist(filename))
  {
    error("The file does not exist!");
    return;
  }
  
  /* file must be in valid size range, do not send endpoint sizes */
  fsize = filesize(filename);
  if(fsize <= 0 || fsize >= MAXFILE)
  {
    error("That file is inappropriately sized!");
    return;
  }
  segments = MAXFILE / MSGLEN;
  
  /* make a copy then figure out just the filename */
  strcpy(msg, filename);
  basefname = basename(msg);
  sprintf(errmsg, "Sending file (%s) in %d segments.", basefname, segments);
  debug("Filesystem", errmsg);
  
  /* send the command, we do not send the user command because we just want
   * to send the base filename. also, we expect a response from the server,
   * either cts or err.
   */
  sprintf(msg, "put:%s", basefname);
  send_message(skt, msg);
  split_command(msg, cmd, arg);
  if(!strcmp(cmd, "err"))
  {
    sprintf(errmsg, "ERROR Not sending the file: %s", arg);
    error(errmsg);
    return;
  }
  else if(strcmp(cmd, "cts"))
  {
    sprintf(errmsg, "ERROR invalid reponse from remote: %s", msg);
    error(errmsg);
    return;
  }
  
  /* send number of segments and file size to the other size cmd:arg style */
  sprintf(msg, "%d:%d", segments, fsize);
  /* do not expect a response, only data after this */
  if(send(skt, msg, strlen(msg), 0) < 0)
  {
    sprintf(errmsg, "Failed to send the number of segments: %s", strerror(errno));
    error(errmsg);
    return;
  }
  
  /* open file or die */
  infile = fopen(filename, "r");
  if(infile == NULL)
  {
    sprintf(errmsg, "File could not be read: %s", strerror(errno));
    error(errmsg);
    return;
  }
  /* now read in the file, MSGLEN at a time and send the chunk to the host */
  for(cur_segment = 0; cur_segment < segments; cur_segment++)
  {
    fread(msg, MSGLEN, 1, infile);
    sprintf(errmsg, "\rFilesystem: Sending file %.2f%%", (cur_segment / segments * 100.0));
    printf(errmsg);
    send_message(skt, msg);
  }
  fclose(infile);
  debug("Filesystem", "File sent successfully");
}

/*
 * This will receive a file from a remote host. Expects a socket integer and
 * a filename. The file will be basename'd and saved into /tmp.
 *
 */
void receive_file(int skt, const char *filename)
{
  unsigned char fbuf[MAXFILE];
  int fsize = 0, segments = 0, cur_segment = 0;
}

