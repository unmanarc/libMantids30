#include "pool.h"

#ifndef _WIN32
#include <utility>

using namespace Mantids30::Runtime;

ExecPool::Request::Request(boost::json::value input) : m_state(std::make_shared<State>(std::move(input))) {}

ExecPool::Error ExecPool::Request::exec(ExecPool &pool)
{
    // Execution, queueing and completion arbitration operate on shared state,
    // allowing the worker to retain it after this caller reaches its deadline.
    return pool.execute(m_state);
}

boost::json::value ExecPool::Request::output() const
{
    // Serialize inspection with completion publishing the result.
    std::scoped_lock lock(m_state->mutex);
    return m_state->output;
}

ExecPool::Error ExecPool::Request::error() const
{
    // Serialize inspection with completion publishing the result.
    std::scoped_lock lock(m_state->mutex);
    return m_state->error;
}
#endif
