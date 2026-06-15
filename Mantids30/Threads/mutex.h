#pragma once

#include <mutex>

namespace Mantids30::Threads::Sync {

typedef std::mutex Mutex;
#define Lock_Mutex(x) std::lock_guard(x)

}

