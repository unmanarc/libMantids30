#include "socket_unix.h"

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <sys/un.h>
#include <sys/socket.h>

using namespace CX2::Network;
using namespace CX2::Network::Sockets;

Socket_UNIX::Socket_UNIX()
{
}

bool Socket_UNIX::listenOn(const uint16_t &, const char *path, const int32_t &recvbuffer, const int32_t &backlog)
{
   if (isActive()) closeSocket(); // close first

   // use the addr as path.
   sockaddr_un server_address;
   int         server_len;

   unlink(path);

   sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
   if (!isActive())
   {
      lastError = "socket() failed";
      return false;
   }

   if (recvbuffer) setRecvBuffer(recvbuffer);

   server_address.sun_family = AF_UNIX;
   strncpy(server_address.sun_path, path, sizeof(server_address.sun_path)-1);
   server_len = sizeof(server_address);

   if (bind(sockfd,(struct sockaddr *)&server_address,server_len) < 0)
   {
       lastError = "bind() failed";
       closeSocket();
       return false;
   }
   if (listen(sockfd, backlog) < 0)
   {
       lastError = "bind() failed";
       closeSocket();
       return false;
   }

   listenMode = true;
   return true;
}

bool Socket_UNIX::listenOn(const char *path, const int32_t &recvbuffer, const int32_t &backlog)
{
    return listenOn(0,path,recvbuffer,backlog);
}

bool Socket_UNIX::connectFrom(const char *, const char * path, const uint16_t &, const uint32_t & timeout)
{
    if (isActive()) closeSocket(); // close first

    int         len;
    sockaddr_un address;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (!isActive())
    {
        lastError = "socket() failed";
        return false;
    }

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, path);
    len = sizeof(address);

    // Set the timeout here.
    setReadTimeout(timeout);

    if(connect(sockfd, (sockaddr*)&address, len) == -1)
    {
        lastError = "socket() failed";
        return false;
    }

    return true;
}

Streams::StreamSocket * Socket_UNIX::acceptConnection()
{
    int sdconn;
    StreamSocket * cursocket = nullptr;

    if ((sdconn = accept(sockfd, nullptr, nullptr)) >= 0)
    {
        cursocket = new StreamSocket;
        // Set the proper socket-
        cursocket->setSocketFD(sdconn);
    }
    else
    {
        // Establish the error.
        lastError = "accept() failed";
    }

    // return the socket class.
    return cursocket;
}
