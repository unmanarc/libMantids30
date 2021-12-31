#include "socket_multiplexer_callbacks.h"

using namespace CX2::Network::Multiplexor;

Socket_Multiplexer_Callbacks::Socket_Multiplexer_Callbacks()
{

}

void Socket_Multiplexer_Callbacks::setCallback_ClientConnectAccepted(Streams::StreamSocket * (*callbackFunction)(void *, std::shared_ptr<Socket_Multiplexed_Line>), void *obj)
{
    cbClientConnectAccepted.callbackFunction = callbackFunction;
    cbClientConnectAccepted.obj = obj;
}

void Socket_Multiplexer_Callbacks::setCallback_ClientConnectFailed(bool (*callbackFunction)(void *, std::shared_ptr<Socket_Multiplexed_Line>, DataStructs::eConnectFailedReason), void *obj)
{
    cbClientConnectFailed.callbackFunction = callbackFunction;
    cbClientConnectFailed.obj = obj;
}

void Socket_Multiplexer_Callbacks::setCallback_ServerConnectAcceptor(Streams::StreamSocket * (*callbackFunction)(void *, const LineID &, const json &), void *obj)
{
    cbServerConnectAcceptor.obj = obj;
    cbServerConnectAcceptor.callbackFunction = callbackFunction;
}

void Socket_Multiplexer_Callbacks::setCallback_ServerConnectionFinished(void (*callbackFunction)(void *, const LineID &, Streams::StreamSocket *), void *obj)
{
    cbServerConnectionFinished.obj = obj;
    cbServerConnectionFinished.callbackFunction = callbackFunction;
}
