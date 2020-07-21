#include "threaded.h"
#include <iostream>

using namespace CX2::Threads;

Threaded::Threaded()
{
    thrRunner = nullptr;
    objRunner = nullptr;
    thrStopper = nullptr;
    objStopper = nullptr;

    running = false;
    destroyOnExit = true;
}

Threaded::~Threaded()
{
    join();
}

void Threaded::start()
{
    xThread = std::thread(Threaded::bgRunner, this);
}

void Threaded::stop()
{
    thrStopper(objStopper);
}

void Threaded::join()
{
    xThread.join();
}

void Threaded::detach()
{
    xThread.detach();
}

void Threaded::setThreadRunner(void (*thrRunner)(void *), void *obj)
{
    this->thrRunner = thrRunner;
    this->objRunner = obj;
}

void Threaded::setThreadStopper(void (*thrStopper)(void *), void *obj)
{
    this->thrStopper = thrStopper;
    this->objStopper = obj;
}

void Threaded::bgRunner(Threaded *t)
{
    t->execRun();
}

void Threaded::execRun()
{
    running = true;
    thrRunner(objRunner);
    running = false;
}

bool Threaded::isRunning() const
{
    return running;
}
