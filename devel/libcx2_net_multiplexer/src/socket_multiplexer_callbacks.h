#ifndef SOCKET_MULTIPLEXER_CALLBACKS_H
#define SOCKET_MULTIPLEXER_CALLBACKS_H

#include <stdint.h>
#include <memory>

#include "socket_multiplexed_line.h"

namespace CX2 { namespace Network { namespace Multiplexor { namespace DataStructs {

enum eConnectFailedReason {
    E_CONN_FAILED=0x00,
    E_CONN_FAILED_TIMEOUT=0x01,
    E_CONN_FAILED_ANSTHREAD=0x02,
    E_CONN_FAILED_BADPARAMS=0x03,
    E_CONN_FAILED_NOCALLBACK = 0x5,
    E_CONN_FAILED_BADSERVERSOCK = 0x6,
    E_CONN_FAILED_BADLOCALLINE = 0x7,
    E_CONN_FAILED_NOTAUTHORIZED = 0x8,
    E_CONN_OK=0xFF
};


struct sServerConnectAcceptCallback
{
    sServerConnectAcceptCallback()
    {
        callbackFunction=nullptr;
        obj = nullptr;
    }
    Streams::StreamSocket * (*callbackFunction)(void *, const LineID &, const Json::Value &);
    void *obj;
};

struct sClientConnectAcceptedCallback
{
    sClientConnectAcceptedCallback()
    {
        callbackFunction=nullptr;
        obj = nullptr;
    }
    Streams::StreamSocket * (*callbackFunction)(void *, std::shared_ptr<Socket_Multiplexed_Line>);
    void *obj;
};

struct sClientConnectFailedCallback
{
    sClientConnectFailedCallback()
    {
        callbackFunction=nullptr;
        obj = nullptr;
    }
    // obj, socket, failed reason
    bool (*callbackFunction)(void *, std::shared_ptr<Socket_Multiplexed_Line>, eConnectFailedReason);
    void *obj;
};

struct sServerConnectionFinishedCallback
{
    sServerConnectionFinishedCallback()
    {
        callbackFunction=nullptr;
        obj = nullptr;
    }
    // obj, socket, failed reason
    void (*callbackFunction)(void *, const LineID &, Streams::StreamSocket *);
    void *obj;
};

}}}}

namespace CX2 { namespace Network { namespace Multiplexor {

class Socket_Multiplexer_Callbacks
{
public:
    Socket_Multiplexer_Callbacks();
    //////////////////////////////////
    // callback systems:
    void setCallback_ServerConnectAcceptor(Streams::StreamSocket * (*callbackFunction)(void *, const LineID &, const Json::Value &), void *obj=nullptr);
    void setCallback_ServerConnectionFinished(void (*callbackFunction)(void *, const LineID &, Streams::StreamSocket *), void *obj=nullptr);
    void setCallback_ClientConnectAccepted(Streams::StreamSocket * (*callbackFunction)(void *, std::shared_ptr<Socket_Multiplexed_Line>), void * obj=nullptr);
    void setCallback_ClientConnectFailed(bool (*callbackFunction)(void *, std::shared_ptr<Socket_Multiplexed_Line>, DataStructs::eConnectFailedReason), void * obj=nullptr);

protected:

    // callbacks:
    DataStructs::sServerConnectAcceptCallback    cbServerConnectAcceptor;
    DataStructs::sClientConnectAcceptedCallback  cbClientConnectAccepted;
    DataStructs::sClientConnectFailedCallback    cbClientConnectFailed;
    DataStructs::sServerConnectionFinishedCallback cbServerConnectionFinished;
};

}}}

#endif // SOCKET_MULTIPLEXER_CALLBACKS_H
