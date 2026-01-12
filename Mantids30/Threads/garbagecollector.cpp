#include "garbagecollector.h"

using namespace Mantids30::Threads;

using Ms = std::chrono::milliseconds;

GarbageCollector::GarbageCollector(const uint32_t &intervalMS)
{
    m_gcFinished = false;
    m_gcIntervalMs = intervalMS;
}

void GarbageCollector::startGarbageCollector(void (*garbageCollectorFunction)(void *), void *parameter, const char *threadName)
{
    this->m_gcFunction = garbageCollectorFunction;
    this->m_gcParameter = parameter;
    m_gcThreadObject = std::thread(backgroundGarbageCollectorLoop,this,threadName);
}

GarbageCollector::~GarbageCollector()
{
    stopGarbageCollector();
    m_gcThreadObject.join();
}

void GarbageCollector::loopGarbageCollector()
{
    std::unique_lock<std::mutex> lock(m_endNotificationMutex);


#ifdef __linux__
    pthread_setname_np(pthread_self(), "Thrd:GCLoop");
#endif



    while(!m_gcFinished)
    {
        if (m_endNotificationCondition.wait_for(lock,Ms(m_gcIntervalMs)) == std::cv_status::timeout)
        {
            m_gcFunction(m_gcParameter);
        }
        else
        {
        }
    }
}

void GarbageCollector::setGarbageCollectorInterval(const uint32_t &intervalMs)
{
    m_gcIntervalMs = intervalMs;
}

void GarbageCollector::stopGarbageCollector()
{
    std::unique_lock<std::mutex> lock(m_endNotificationMutex);
    m_gcFinished = true;
    m_endNotificationCondition.notify_one();
}

void GarbageCollector::backgroundGarbageCollectorLoop(GarbageCollector *threadClass, const char * threadName)
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), threadName);
#endif
    threadClass->loopGarbageCollector();
}

