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
 * Sends a message to the socket. Pass in the socket and message. Expects msg
 * to be already validated by validate_command first! Formats response into
 * string buffer given.
 *
 */
void send_message(int skt, char *msg)
{
  char command[CMDLEN], argument[MSGLEN], errmsg[ERRMSGLEN];
  memset(command, '\0', CMDLEN);
  memset(argument, '\0', MSGLEN);
  memset(errmsg, '\0', ERRMSGLEN);
  
  if(send(skt, msg, strlen(msg), 0) < 0)
  {
    sprintf(errmsg, "Send Error: %s", strerror(errno));
    error(errmsg);
  }
  else if(recv(skt, msg, MSGLEN, 0) < 0)
  {
    sprintf(errmsg, "Receive Error: %s", strerror(errno));
    error(errmsg);
  }
  else
  {
    split_command(msg, command, argument);
    if(strlen(argument) > 0)
    {
      sprintf(errmsg, "Response: %s: %s", command, argument);
    }
    else
    {
      sprintf(errmsg, "Response: %s", command);
    }
    cdebug(errmsg);
  }
}

/*
 * Runs the command processor to the server. Handles commands and responses.
 *
 */
void run_client(const char *address, int portnum)
{
  int skt;
  char msg[MSGLEN];
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
      send_message(skt, msg);
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

