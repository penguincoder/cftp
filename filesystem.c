#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>

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
 * This function will receive a packet and send a response back. Backwards of
 * send_message. Takes an integer for the socket and a buffer to write the
 * message into. This is only designed to take a reponse from send_file and
 * send back either "cts" or "err".
 *
 */
void receive_message(int skt, char *buf)
{
  char response[CMDLEN];
  memset(response, '\0', CMDLEN);
  if(!receive_packet(skt, buf))
  {
    strcpy(response, "err");
  }
  else
  {
    strcpy(response, "cts");
  }
  send_packet(skt, response);
}

/*
 * This function will send a file to a remote host. Expects a socket integer
 * and a string containing the full path to the local file name. File cannot
 * exceed MAXFILE definition. Takes a server boolean for some additional
 * checks.
 *
 */
void send_file(int skt, const char *filename, int server)
{
  char errmsg[ERRMSGLEN], msg[MSGLEN];
  int fsize = 0, segments = 0, cur_segment;
  FILE *infile;
  
  memset(errmsg, '\0', ERRMSGLEN);
  memset(msg, '\0', MSGLEN);
  
  /* file must exist */
  if(!exist(filename))
  {
    if(server)
    {
      send_packet(skt, "err:That file does not exist");
    }
    error("The file does not exist!");
    return;
  }
  
  /* file must be in valid size range, do not send endpoint sizes */
  fsize = filesize(filename);
  if(fsize <= 0 || fsize >= MAXFILE)
  {
    if(server)
    {
      send_packet(skt, "err:File is too big");
    }
    error("File is too big");
    return;
  }
  segments = (int) ceil(fsize / MSGLEN);
  
  /* open file or die */
  infile = fopen(filename, "r");
  if(infile == NULL)
  {
    if(server)
    {
      send_packet(skt, "err:File could not be read");
    }
    sprintf(errmsg, "File could not be read: %s", strerror(errno));
    error(errmsg);
    return;
  }
  
  /* send cts to client */
  if(server)
  {
    send_packet(skt, "cts");
    sleep(1);
  }
  
  /* send number of segments and file size to the other size cmd:arg style */
  sprintf(msg, "%d:%d", segments, fsize);
  /* do not expect a response, only data after this */
  send_packet(skt, msg);
  
  /* now read in the file, MSGLEN at a time and send the chunk to the host */
  for(cur_segment = 0; cur_segment <= segments; cur_segment++)
  {
    memset(msg, '\0', MSGLEN);
    fread(msg, MSGLEN, 1, infile);
    printf("\rFilesystem: Sending file %d / %d", cur_segment, segments);
    send_packet(skt, msg);
  }
  printf("\n");
  fclose(infile);
}

/*
 * This will receive a file from a remote host. Expects a socket integer and
 * a filename. The file will be basename'd and saved into /tmp.
 *
 */
void receive_file(int skt, const char *filename, int server)
{
  char msg[MSGLEN], cmd[CMDLEN], arg[MSGLEN], lname[MSGLEN], errmsg[ERRMSGLEN];
  char *basefname;
  FILE *outfile;
  int fsize = 0, segments = 0, cur_segment = 0;
  
  /* get the local file name in /tmp */
  memset(lname, '\0', MSGLEN);
  memset(errmsg, '\0', ERRMSGLEN);
  strcpy(msg, filename);
  basefname = basename(msg);
  sprintf(lname, "/tmp/%s", basefname);
  
  /* if file exists, send err to remote, otherwise send cts and await size */
  if(exist(lname))
  {
    debug("Filesystem", "Not receiving file since it already exists");
    if(server)
    {
      send_packet(skt, "err:File already exists");
    }
    return;
  }
  
  /* open file for writing */
  outfile = fopen(lname, "w");
  if(outfile == NULL)
  {
    sprintf(errmsg, "Could not open file for writing: %s", strerror(errno));
    debug("Filesystem", errmsg);
    if(server)
    {
      send_packet(skt, "err:File could not be written");
    }
    return;
  }
  
  /* ready to receive file */
  if(server)
  {
    memset(msg, '\0', MSGLEN);
    strcpy(msg, "cts");
    send_message(skt, msg);
  }
  else
  {
    receive_packet(skt, msg);
  }
  split_command(msg, cmd, arg);
  segments = atoi(cmd);
  fsize = atoi(arg);
  sprintf(errmsg, "Going to receive %d bytes in %d segments", fsize, segments);
  debug("Filesystem", errmsg);
  
  /* loop through segments packet count and get the file */
  for(cur_segment = 0; cur_segment <= segments; cur_segment++)
  {
    printf("\rFilesystem: Receiving File: %d / %d", cur_segment, segments);
    memset(msg, '\0', MSGLEN);
    receive_packet(skt, msg);
    if(cur_segment == segments)
    {
      /* write remaining bytes */
      fwrite(msg, (fsize - (segments * MSGLEN)), 1, outfile);
    }
    else
    {
      /* write complete segment */
      fwrite(msg, MSGLEN, 1, outfile);
    }
  }
  printf("\n");
  fclose(outfile);
}

