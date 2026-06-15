#pragma once

#include <mutex>

namespace Mantids30::Threads::Sync {

using Mutex = std::mutex;
#define Lock_Mutex(x) std::lock_guard(x)

} // namespace Mantids30::Threads::Sync
