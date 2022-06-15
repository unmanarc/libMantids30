#ifndef GarbageCollector_H
#define GarbageCollector_H

#include "threaded.h"

#include <condition_variable>
#include <mutex>

namespace Mantids { namespace Threads {

class GarbageCollector
{
public:
    GarbageCollector(const uint32_t &periodMS = 3000);
    void startGC(void (*gcRunner)(void *obj), void *obj, const char * threadName = "GC:Generic");

    virtual ~GarbageCollector();

    void loopGC();
    void setGCPeriodMS(const uint32_t & msecs);

private:
    static void bgGCLoop(GarbageCollector * threadClass, const char * threadName);

    int gcThreadStatus;

    std::mutex mutex_endNotificationLoop;
    std::condition_variable cond_endNotification;

    void (*gcRunner)(void *obj);
    void * obj;

    bool gcFinished;
    std::thread xThreadGC;
    std::atomic<uint32_t> gcPeriodMS;


};
}}

#endif // GarbageCollector_H
