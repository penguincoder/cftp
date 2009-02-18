#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "cftp.h"
#include "client.h"
#include "filesystem.h"

/*
 * Special client debug wrapper.
 *
 */
void cdebug(const char *msg)
{
  debug("Client", msg);
}

/*
 * This function runs a client to the specified address/hostname and port.
 *
 */
int connect_to_server(const char *address, int portnum)
{
  int skt;
  struct sockaddr_in server_address;
  struct hostent *host;
  char msg[ERRMSGLEN];
  
  memset(&server_address, 0, sizeof(server_address));
  memset(msg, '\0', ERRMSGLEN);
  
  /* look up hostname */
  if((host = gethostbyname(address)) == NULL)
  {
    sprintf(msg, "Could not look up hostname: %s", address);
    error(msg);
    exit(errno);
  }
  
  /* create socket */
  if((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    sprintf(msg, "Failed to make the socket: %s", strerror(errno));
    error(msg);
    exit(errno);
  }
  
  server_address.sin_family = AF_INET;
  server_address.sin_addr = *((struct in_addr *) host->h_addr);
  server_address.sin_port = htons(portnum);
  
  /* connect to the server */
  if(connect(skt, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
  {
    sprintf(msg, "Could not connect to the server: %s", strerror(errno));
    error(msg);
    exit(errno);
  }
  
  return skt;
}

/*
 * Runs the command processor to the server. Handles commands and responses.
 *
 */
void run_client(const char *address, int portnum)
{
  int skt;
  char msg[MSGLEN], cmd[CMDLEN], argument[MSGLEN];
  memset(msg, '\0', MSGLEN);
  
  /* simple informational header */
  sprintf(msg, "Using Server: %s:%d", address, portnum);
  cdebug(msg);
  memset(msg, '\0', MSGLEN);
  
  /* loop until finished */
  do {
    if(strlen(msg) > 0 && validate_command(msg))
    {
      skt = connect_to_server(address, portnum);
      split_command(msg, cmd, argument);
      if(!strcmp(cmd, "put"))
      {
        send_file(skt, argument);
      }
      else
      {
        send_message(skt, msg);
        cdebug(msg);
      }
      close(skt);
    }
    else if(strlen(msg) > 0 && !strcmp(msg, "help"))
    {
      sprintf(msg, "Using Server: %s:%d", address, portnum);
      cdebug(msg);
      cdebug("Available commands:");
      cdebug(" * ping - returns an unix timestamp from the server");
      cdebug(" * get:filename - gets a file, if possible");
      cdebug(" * put:filename - sends a file, if possible");
    }
    memset(msg, '\0', MSGLEN);
    printf("> ");
    scanf("%s", msg);
  } while(strcmp(msg, "quit") != 0);
  
  /* cleanup and quit */
  cdebug("Bye!");
}

