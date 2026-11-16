#include "threaded.h"
#include <memory>

using namespace Mantids30::Core::Threads;

Threaded::Threaded() : m_isRunning(false)
{
}

Threaded::~Threaded()
{
    join();
}

void Threaded::startInBackground()
{
    auto self = shared_from_this();
    m_isRunning = true;
    try
    {
        m_threadObj = std::thread(Threaded::bgRunner, self);
    }
    catch (...)
    {
        m_isRunning = false;
        throw;
    }
}

void Threaded::stop()
{
    m_threadStopper(m_contextStopper);
}

void Threaded::join()
{
    if (m_threadObj.joinable())
    {
        if (m_threadObj.get_id() == std::this_thread::get_id())
        {
            m_threadObj.detach();
        }
        else
        {
            m_threadObj.join();
        }
    }
}

void Threaded::detach()
{
    m_threadObj.detach();
}

void Threaded::setThreadRunner(void (*threadRunner)(void *), void *context)
{
    this->m_threadRunner = threadRunner;
    this->m_contextRunner = context;
}

void Threaded::setThreadStopper(void (*threadStopper)(void *), void *context)
{
    this->m_threadStopper = threadStopper;
    this->m_contextStopper = context;
}

void Threaded::bgRunner(const std::shared_ptr<Threaded> &t)
{
    t->execRun();
}

void Threaded::execRun()
{
#ifdef __linux__
    pthread_setname_np(pthread_self(), "Thrd:Runner");
#endif

    m_isRunning = true;
    m_threadRunner(m_contextRunner);
    m_isRunning = false;
}

bool Threaded::isRunning() const
{
    return m_isRunning;
}
