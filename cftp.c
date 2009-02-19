/*
 * Coleman's FTP.
 *
 * A simple socket client and server for sending files and messages in a
 * single port.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include "cftp.h"
#include "server.h"
#include "client.h"

/*
 * This function will process all server requests untill killed. Takes a string
 * containing the address to bind (if any) and the integer port.
 *
 */
int validate_command(const char *msg)
{
  char errmsg[ERRMSGLEN];
  
  /* GET file command */
  if(!strncmp(msg, "get", 3))
    return 1;
  
  /* PUT file command */
  if(!strncmp(msg, "put", 3))
    return 1;
  
  /* PING send back the time stamp of right now */
  if(!strncmp(msg, "ping", 4))
    return 1;
  
  /* nothing else was found, so clearly this is not true */
  sprintf(errmsg, "Invalid command: %s", msg);
  error(errmsg);
  return 0;
}

/*
 * Splits a buffer into a command and argument.
 *
 */
void split_command(const char *buffer, char *command, char *argument)
{
  int c = 0, d = 0;
  memset(command, '\0', CMDLEN);
  memset(argument, '\0', MSGLEN);
  
  /* find everything up to the colon delimiter */
  while(buffer[c] != '\0' && buffer[c] != ':' && c < CMDLEN && d < CMDLEN)
  {
    command[d++] = buffer[c++];
  }
  /* skip the colon */
  if(buffer[c] == ':')
  {
    c++;
    d = 0;
  }
  /* copy argument, if found */
  while(buffer[c] != '\0' && c < MSGLEN && d < MSGLEN)
  {
    argument[d++] = buffer[c++];
  }
}

/*
 * This function will display error messages on either client or server side.
 * Takes a string for the error message and prints it to stderr.
 *
 */ 
void error(const char *msg)
{
  char msg2[ERRMSGLEN + 1];
  sprintf(msg2, "%s\n", msg);
  fprintf(stderr, msg2);
}

/*
 * Prints out a debugging message to stdout.
 *
 */
void debug(const char *prefix, const char *msg)
{
  printf("%s: %s\n", prefix, msg);
}

/*
 * Wrapper to send a packet to a socket given a string. Handles errors and
 * returns a bool based on success.
 *
 */
int send_packet(int skt, const char *msg)
{
  char errmsg[ERRMSGLEN];
  memset(errmsg, '\0', ERRMSGLEN);
  if(send(skt, msg, strlen(msg), 0) < 0)
  {
    sprintf(errmsg, "Send Packet Error: %s", strerror(errno));
    error(errmsg);
    return 0;
  }
  return 1;
}

/*
 * Wrapper to receive a packet from a socket and stores the result in a given
 * buffer. Handles errors and returns a bool based on success.
 *
 */
int receive_packet(int skt, char *msg)
{
  char errmsg[ERRMSGLEN];
  memset(errmsg, '\0', ERRMSGLEN);
  memset(msg, '\0', MSGLEN);
  if(recv(skt, msg, MSGLEN, 0) < 0)
  {
    sprintf(errmsg, "Receive Packet Error: %s", strerror(errno));
    error(errmsg);
    return 0;
  }
  return 1;
}

/*
 * Sends a message to the socket. Pass in the socket and message. Expects msg
 * to be already validated by validate_command first! Formats response into
 * string buffer given. Works for client or server, so long as you expect a
 * response message from the other side.
 *
 */
void send_message(int skt, char *msg)
{
  if(send_packet(skt, msg))
  {
    receive_packet(skt, msg);
  }
}

/*
 * Prints out the usage of the program to stderr.
 *
 */
void usage(const char *progname)
{
  char msg[ERRMSGLEN];
  error("Coleman's FTP application");
  sprintf(msg, "Usage: %s [-h] [-s] [-a address] [-p port]", progname);
  error(msg);
  error("  -h for this message");
  error("  -s for server, default is client");
  error("  -a for address");
  error("  -p for port, default is 9000, no privileged ports allowed");
  exit(1);
}

int main(int argc, char **argv)
{
  int opt = 0, portnum = 9000, server = 0;
  char address[MSGLEN];
  memset(address, '\0', MSGLEN);
  
  /* process options using getopt */
  while((opt = getopt(argc, argv, "sa:p:h")) != -1)
  {
    switch(opt)
    {
      case 's':
        server = 1;
        break;
      
      case 'p':
        portnum = atoi(optarg);
        break;
      
      case 'a':
        strcpy(address, optarg);
        break;
      
      case 'h':
        usage(argv[0]);
        break;
    }
  }
  
  /* sanitization and cop outs */
  if(!server && strlen(address) == 0)
    usage(argv[0]);
  else if(portnum <= 1024 || portnum >= 65535)
    usage(argv[0]);
  
  if(server)
  {
    run_server(address, portnum);
  }
  else
  {
    run_client(address, portnum);
  }
  
  return 0;
}

