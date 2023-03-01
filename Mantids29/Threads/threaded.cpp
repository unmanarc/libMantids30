#include "threaded.h"
#include <iostream>
#include <memory>

using namespace Mantids29::Threads;

Threaded::Threaded()
{
    threadRunner = nullptr;
    runnerArg = nullptr;
    threadStopper = nullptr;
    stopperArg = nullptr;

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
    threadStopper(stopperArg);
}

void Threaded::join()
{
    threadObj.join();
}

void Threaded::detach()
{
    threadObj.detach();
}

void Threaded::setThreadRunner(void (*threadRunner)(void *), void *obj)
{
    this->threadRunner = threadRunner;
    this->runnerArg = obj;
}

void Threaded::setThreadStopper(void (*threadStopper)(void *), void *obj)
{
    this->threadStopper = threadStopper;
    this->stopperArg = obj;
}

void Threaded::bgRunner(const std::shared_ptr<Threaded> & t)
{
    t->execRun();
}

void Threaded::execRun()
{
    running = true;
    threadRunner(runnerArg);
    running = false;
}

bool Threaded::isRunning() const
{
    return running;
}
