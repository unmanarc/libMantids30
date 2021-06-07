#ifndef SOCKET_MULTIPLEXED_LINE
#define SOCKET_MULTIPLEXED_LINE

#include <stdint.h>
#include <string.h>

#include <queue>
#include <mutex>
#include <condition_variable>
#include <cx2_hlp_functions/json.h>

#include <cx2_thr_mutex/mutex.h>
#include <cx2_thr_mutex/mutex_shared.h>

#include "socket_multiplexer_a_struct_databuffer.h"
#include "socket_multiplexer_a_struct_lineid.h"

#include <cx2_net_sockets/streamsocket.h>

namespace CX2 { namespace Network { namespace Multiplexor {

class Socket_Multiplexed_Line
{
public:
    Socket_Multiplexed_Line();
    ~Socket_Multiplexed_Line();
    /////////////////////////////////////////////////////////////////////////////////////////////
    // Buffer:
    /**
     * @brief addToBuffer Write Data into buffer which will be readed by line socket (used to write from multiplexed socket -> line)
     * @param data data
     * @param len 0: connection finalized, n: data lenght
     * @return true if written, false if not (eg. out of memory)
     */
    bool addBufferElement(void * data, uint16_t len);
    /////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Socket:
    /**
     * @brief _lshutdown shutdown line socket introduced in processLine
     */
    void _lshutdown();
    /////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Line Processor:
    /**
     * @brief processLine process line until it dies. (blocking function)
     * @param lineAttachedSocket line socket (you should manage the deletion of this after "waitForProcessLine")
     * @param multiPlexer interface pointer
     * @return true if processed, false for memory related problems (undefined behaviour).
     */
    bool processLine(Streams::StreamSocket * lineAttachedSocket, void * multiPlexer);
    /**
     * @brief finalizeProcessor
     */
    void finalizeProcessor();
    /**
     * @brief waitForProcessLine Wait until unlockLineProcessorWait is called or processLine finished.
     */
    void waitForProcessLine();
    /**
     * @brief processBuffer process the internal buffer once / don't call this (this is for internal thread)
     * @return true if you should continue calling this function, or false if not (exit).
     */
    bool processBuffer();
    /////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Line Identification:
    bool isValidLine();
    void setLineLocalID(const LineID &value);
    void setLineRemoteID(const LineID &value);
    DataStructs::sLineID getLineID();
    /////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Window management
    void setRemoteWindowSize(const uint32_t &value);
    bool addProcessedBytes(const uint16_t &value);
    uint32_t getLocalWindowSize() const;
    void resetRemoteAvailableBytes();
    /////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////////////////////
    // Local object (to handled aync during connect)
    void *getLocalObject();
    void setLocalObject(void *value);
    /////////////////////////////////////////////////////////////////////////////////////////////

    json getConnectionParams() const;
    void setConnectionParams(const json &value);

private:
    static void processLineThread(Socket_Multiplexed_Line * multiplexedLine);


    uint32_t getRemoteAvailableBytes();
    void addBufferElement( DataStructs::sDataBuffer * dbuf );
    DataStructs::sDataBuffer * getBufferElement( bool emptyBlocking = true );

    Streams::StreamSocket * lineAttachedSocket; // xmutexVars mutex.
    void * multiPlexer; // xmutexVars mutex.
    void * localObject; // rwLock_LocalObject mutex.

    uint32_t localWindowSize; // unmodified constant.
    uint32_t localWindowUsedBuffer; // mutexBufferHeap mutex
    uint32_t remoteWindowSize; // mutexRemoteProccesedBytes mutex.
    uint32_t remoteUnprocessedBytes; // mutexRemoteProccesedBytes mutex.

    DataStructs::sLineID lineID; // xMutexLineID mutex.

    bool processLineFinished; // mtProcessLineFinished mutex.
    bool remoteUnprocessedFinished; // mutexRemoteProccesedBytes mutex.

   json connectionParams;

    std::queue<DataStructs::sDataBuffer *> dataBuffer;

    Threads::Sync::Mutex_Shared rwLock_LocalObject, rwLock_Vars, rwLock_LineID;
    Threads::Sync::Mutex mtLock_BufferHeap, mtLock_RemoteProccesedBytes, mtLock_ProcessLineFinished;
    std::condition_variable psigRemoteProccesedBytesNotFull,psigBufferNotFull,psigBufferNotEmpty,psigProcessLineFinished;
};

}}}

#endif // SOCKET_MULTIPLEXED_LINE
