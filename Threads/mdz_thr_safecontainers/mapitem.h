#ifndef XMAPELEMENT_H
#define XMAPELEMENT_H

#include <atomic>

namespace Mantids { namespace Threads { namespace Safe {


class MapItem
{
public:
    MapItem();
    virtual ~MapItem();

    /**
     * @brief stopReaders: stop and wait for all readers to finish...
     */
    void stopReaders();

protected:
    // Multiple readers can be accessing this, so StopSignal should
    // defuse/close waiting sockets and all functions.
    virtual void stopSignal();

    /**
     * @brief xMapFinished this map is finished.
     */
    std::atomic<bool> xMapFinished;
};

}}}

#endif // XMAPELEMENT_H
