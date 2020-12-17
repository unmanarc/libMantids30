#ifndef SOCKET_MULTIPLEXER_H
#define SOCKET_MULTIPLEXER_H

#include "vars.h"

#include "socket_multiplexer_lines.h"
#include "socket_multiplexer_a_struct_threadparams.h"
#include "socket_multiplexer_a_enum_mplx_msgs.h"
#include "socket_multiplexer_a_enum_lineaccept_msgs.h"
#include "socket_multiplexer_plugin.h"

#include <cx2_net_sockets/streamsocket.h>

namespace CX2 { namespace Network { namespace Multiplexor {

class Socket_Multiplexer : public Socket_Multiplexer_Callbacks, public Socket_Multiplexer_Lines
{
public:
    Socket_Multiplexer();
    ~Socket_Multiplexer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Initialization (call before start):
    bool plugin_Add(Socket_Mutiplexer_Plugin * plugin);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Start:
    /**
     * @brief run Start processing multiplexor (locked until finished)
     * @param multiplexedSocket socket for sending/recving multiplexed messages
     * @param localName local multiplexor name
     */
    void run(Streams::StreamSocket *multiplexedSocket, const std::string &localName = "localhost");
    /**
     * @brief close sends the message to close the multiplexed socket (ordered close)
     * @return true if remotely written, false if not.
     */
    bool close();
    /**
     * @brief forceClose force close the multiplexed socket shutting down the socket.
     * @return
     */
    void forceClose();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Connections:
    /**
     * @brief connect Creates new line
     * @param connectionParams JSON connnection params to be delivered.
     * @param lineSocketLocalObject  object to be setted into.
     * @return line id (only local).
     */
    LineID connect(const Json::Value & connectionParams, void *multiplexedSocketLocalObject = nullptr, unsigned int milliseconds = 5000);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // status:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Internal functions (don't use them):
    // Plugins:
    bool plugin_SendData(const std::string & pluginId, void * data, const uint32_t &datalen, bool lock = true); // use lock false when
    bool plugin_SendJson(const std::string & pluginId, const Json::Value & jData, bool lock = true);
    // Line:
    bool multiplexedSocket_sendLineData(const DataStructs::sLineID &lineId, void * data, const uint16_t &datalen);
    bool multiplexedSocket_sendTermination(const DataStructs::sLineID &lineId);
    bool multiplexedSocket_sendReadenBytes(const DataStructs::sLineID &lineId, const uint16_t &freedSize);

    // callbacks:
    void server_AcceptConnection_Callback(DataStructs::sLineID remoteLineId, const uint32_t &remoteWindowSize, const Json::Value & connectionParams);
    void client_HandlerConnection_Callback(std::shared_ptr<Socket_Multiplexed_Line> sock);
    void client_FailedConnection_Callback(std::shared_ptr<Socket_Multiplexed_Line> sock, DataStructs::eConnectFailedReason reason);

    std::string getRemoteName() const;
    std::string getLocalName() const;

    bool getDestroySocketOnClient() const;
    void setDestroySocketOnClient(bool value);

    bool getDestroySocketOnServer() const;
    void setDestroySocketOnServer(bool value);

private:
    static void serverAcceptConnectionThread(DataStructs::sServerLineInitThreadParams * thrParams);
    static void clientHandleConnectionFailedThread(DataStructs::sConnectionThreadParams * thrParams);
    static void clientHandleConnectionThread(DataStructs::sConnectionThreadParams * thrParams);

    bool processMultiplexedSocket();

    bool sendOnMultiplexedSocket_LineID(const LineID & chId);
    LineID recvFromMultiplexedSocket_LineID(bool *readen);

    bool multiplexedSocket_sendCloseACK1();
    bool multiplexedSocket_sendCloseACK2();

    bool multiplexedSocket_sendLineConnectionAnswer(const DataStructs::sLineID & lineId, const DataStructs::eLineAcceptAnswerMSG & msg, const uint32_t &localLineWindow = 0);
    bool multiplexedSocket_sendLineConnectionAnswer(const DataStructs::sLineID & lineId, const DataStructs::eLineAcceptAnswerMSG & msg, const Json::Value & answerValue, const uint32_t &localLineWindow = 0);

    bool processMultiplexedSocketCommand_Plugin_JSON16();
    bool processMultiplexedSocketCommand_Plugin_Data();
    bool processMultiplexedSocketCommand_Line_ConnectionAnswer();
    bool processMultiplexedSocketCommand_Line_Connect(bool authorized);
    bool processMultiplexedSocketCommand_Line_Data();
    bool processMultiplexedSocketCommand_Line_UpdateReadenBytes();

    // multiplexed socket:
    Streams::StreamSocket * multiplexedSocket;
    std::timed_mutex mtLock_multiplexedSocket;

    std::string localName, remoteName;

    uint16_t peerMultiplexorVersion;

    std::atomic<bool> noSendData;
    bool destroySocketOnClient,destroySocketOnServer;
    unsigned char readData[65536];

    std::map<std::string,Socket_Mutiplexer_Plugin *> plugins;
};

}}}

#endif // SOCKET_MULTIPLEXER_H
