#include "socket_unix.h"

#ifndef _WIN32

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <sys/un.h>
#include <sys/socket.h>

#include <mdz_hlp_functions/mem.h>

using namespace Mantids::Network;
using namespace Mantids::Network::Sockets;

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

   SecBACopy(server_address.sun_path, path);

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
    strncpy(address.sun_path, path,sizeof(address.sun_path));
    len = sizeof(address);

    // Set the timeout here.
    setReadTimeout(timeout);

    if(connect(sockfd, (sockaddr*)&address, len) == -1)
    {
        int valopt=0;
        // Socket selected for write
        socklen_t lon;
        lon = sizeof(int);
        if (getSocketOption(SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
        {
            lastError = "Error in getsockopt(SOL_SOCKET)";
            return false;
        }

        // Check the value returned...
        if (valopt)
        {

            char cError[1024]="Unknown Error";

            lastError = std::string("Connection to AF_UNIX Socket failed with error #") + std::to_string(valopt) + ": " + strerror_r(valopt,cError,sizeof(cError));
            return false;
        }

        lastError = "Connect(AF_UNIX) failed";
        return false;
    }

    return true;
}

Sockets::Socket_StreamBase * Socket_UNIX::acceptConnection()
{
    int sdconn;
    Socket_StreamBase * cursocket = nullptr;

    if ((sdconn = accept(sockfd, nullptr, nullptr)) >= 0)
    {
        cursocket = new Socket_StreamBase;
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

#endif
