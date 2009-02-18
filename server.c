#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cftp.h"
#include "server.h"

/*
 * Special server debug wrapper.
 *
 */
void sdebug(const char *msg)
{
  debug("Server", msg);
}

/*
 * Handles each command received from the server. Expects that the message has
 * been validated by validate_command first! Takes one parameter, the message
 * from the client. The response is stored in the same buffer for return.
 *
 */
void server_command(char *msg)
{
  char command[CMDLEN], argument[MSGLEN];
  time_t pong;
  memset(command, '\0', CMDLEN);
  memset(argument, '\0', MSGLEN);
  
  /* get individual command and argument for processing */
  split_command(msg, command, argument);
  
  /* now check each command and perform the expected results */
  if(!strcmp(command, "ping"))
  {
    pong = time(NULL);
    sprintf(msg, "Pong:%llud", (uintmax_t)pong);
  }
}

/*
 * Proccesses a client connection and performs the necessary command. Called
 * on each connection from a client.
 *
 */
void handle_client(int skt, const char *clientip)
{
  char buffer[MSGLEN], msg[ERRMSGLEN];
  memset(buffer, '\0', MSGLEN);
  memset(msg, '\0', ERRMSGLEN);
  
  /* figure out what to do */
  if(recv(skt, buffer, MSGLEN, 0) >= 0 && validate_command(buffer))
  {
    server_command(buffer);
  }
  else if(errno)
  {
    sprintf(buffer, "ERR:%s", strerror(errno));
  }
  else
  {
    sprintf(buffer, "ERR:Invalid Command");
  }
  
  /* send message to client */
  if(strlen(buffer) > 0)
  {
    sprintf(msg, "%s: %s", clientip, buffer);
    sdebug(msg);
    send(skt, buffer, strlen(buffer), 0);
  }
  
  /* cleanup */
  close(skt);
}

/*
 * This function will process all server requests untill killed.
 * Takes a string containing the address to bind (if any) and the integer port.
 *
 */
void run_server(const char *address, const int portnum)
{
  int skt, client_socket;
  unsigned int length;
  char msg[ERRMSGLEN];
  struct sockaddr_in server_address, client_address;
  
  memset(&server_address, 0, sizeof(server_address));
  memset(&client_address, 0, sizeof(client_address));
  
  /* create server socket */
  if((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    sprintf(msg, "Socket error: %s", strerror(errno));
    error(msg);
    exit(errno);
  }
  else
  {
    sdebug("Created socket");
  }
  
  /* build server address struct */
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(portnum);
  /* any network address, or 0.0.0.0 if not supplied */
  if(strlen(address) == 0)
  {
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  /* assume ip address is given */
  else
  {
    if(inet_aton(address, &server_address.sin_addr) == 0)
    {
      sprintf(msg, "Malformed address: %s", address);
      error(msg);
      exit(1);
    }
  }
  
  /* bind to socket */
  length = sizeof(server_address);
  if(bind(skt, (struct sockaddr *)&server_address, length) < 0)
  {
    sprintf(msg, "Bind error: %s", strerror(errno));
    error(msg);
    exit(errno);
  }
  else
  {
    sprintf(msg, "Bound to address %s", inet_ntoa(server_address.sin_addr));
    sdebug(msg);
  }
  
  /* listen on socket */
  if(listen(skt, 10) < 0)
  {
    sprintf(msg, "Listen error: %s", strerror(errno));
    error(msg);
    exit(errno);
  }
  else
  {
    sprintf(msg, "Listening on port %d", portnum);
    sdebug(msg);
  }
  
  /* actual server loop */
  while(1)
  {
    length = sizeof(client_address);
    if((client_socket = accept(skt, (struct sockaddr *)&client_address, &length)) < 0)
    {
      sprintf(msg, "Accept error: %s", strerror(errno));
      error(msg);
      exit(errno);
    }
    else
    {
      handle_client(client_socket, inet_ntoa(client_address.sin_addr));
    }
  }
}
