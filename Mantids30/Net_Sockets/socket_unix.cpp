#include "socket_unix.h"

#ifndef _WIN32

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <sys/un.h>
#include <sys/socket.h>

#include <Mantids30/Helpers/mem.h>

using namespace Mantids30::Network;
using namespace Mantids30::Network::Sockets;

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
   
   m_sockFD = socket(AF_UNIX, SOCK_STREAM, 0);
   if (!isActive())
   {
       m_lastError = "socket() failed";
      return false;
   }

   if (recvbuffer) setRecvBuffer(recvbuffer);

   server_address.sun_family = AF_UNIX;

   SecBACopy(server_address.sun_path, path);

   server_len = sizeof(server_address);
   
   if (bind(m_sockFD,(struct sockaddr *)&server_address,server_len) < 0)
   {
      m_lastError = "bind() failed";
       closeSocket();
       return false;
   }
   if (listen(m_sockFD, backlog) < 0)
   {
       m_lastError = "bind() failed";
       closeSocket();
       return false;
   }

   m_isInListenMode = true;
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

    m_remoteServerHostname = path;
    
    m_sockFD = socket(AF_UNIX, SOCK_STREAM, 0);
    if (!isActive())
    {
       m_lastError = "socket() failed";
        return false;
    }

    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, path,sizeof(address.sun_path));
    len = sizeof(address);

    // Set the timeout here.
    setReadTimeout(timeout);
    
    if(connect(m_sockFD, (sockaddr*)&address, len) == -1)
    {
        int valopt=0;
        // Socket selected for write
        socklen_t lon;
        lon = sizeof(int);
        if (getSocketOption(SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
        {
            m_lastError = "Error in getsockopt(SOL_SOCKET)";
            return false;
        }

        // Check the value returned...
        if (valopt)
        {

            char cError[1024]="Unknown Error";
            
            m_lastError = std::string("Connection to AF_UNIX Socket failed with error #") + std::to_string(valopt) + ": " + strerror_r(valopt,cError,sizeof(cError));
            return false;
        }
        
        m_lastError = "Connect(AF_UNIX) failed";
        return false;
    }

    return true;
}

Sockets::Socket_Stream_Base * Socket_UNIX::acceptConnection()
{
    int sdconn;
    Socket_Stream_Base * cursocket = nullptr;
    
    if ((sdconn = accept(m_sockFD, nullptr, nullptr)) >= 0)
    {
        cursocket = new Socket_Stream_Base;
        // Set the proper socket-
        cursocket->setSocketFD(sdconn);
    }
    else
    {
        // Establish the error.
        m_lastError = "accept() failed";
    }

    // return the socket class.
    return cursocket;
}

#endif
