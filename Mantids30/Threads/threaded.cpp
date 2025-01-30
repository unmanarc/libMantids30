#include "threaded.h"
#include <iostream>
#include <memory>

using namespace Mantids30::Threads;

Threaded::Threaded()
{
    threadRunner = nullptr;
    contextRunner = nullptr;
    threadStopper = nullptr;
    contextStopper = nullptr;

    running = false;
}

Threaded::~Threaded()
{
    join();
}

void Threaded::start(const std::shared_ptr<Threaded> & tc)
{
    threadObj = std::thread(Threaded::bgRunner, tc);
}

void Threaded::stop()
{
    threadStopper(contextStopper);
}

void Threaded::join()
{
    threadObj.join();
}

void Threaded::detach()
{
    threadObj.detach();
}

void Threaded::setThreadRunner(void (*threadRunner)(void *), void *context)
{
    this->threadRunner = threadRunner;
    this->contextRunner = context;
}

void Threaded::setThreadStopper(void (*threadStopper)(void *), void *context)
{
    this->threadStopper = threadStopper;
    this->contextStopper = context;
}

void Threaded::bgRunner(const std::shared_ptr<Threaded> & t)
{
    t->execRun();
}

void Threaded::execRun()
{
    running = true;
    threadRunner(contextRunner);
    running = false;
}

bool Threaded::isRunning() const
{
    return running;
}
