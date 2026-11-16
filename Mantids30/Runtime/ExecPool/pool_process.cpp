#include "pool.h"

#ifndef _WIN32
#include <fcntl.h>
#include <cerrno>

using namespace Mantids30::Runtime;

std::unique_ptr<ExecPool::App> ExecPool::spawn(size_t index, bool restart)
{
    // Rebuild launch settings for each generation because AppSpawn consumes its
    // argument and environment configuration after a successful spawn.
    auto app = std::make_unique<App>();
    if (!app->setExec(m_config.command.arg0))
    {
        const int savedError = errno;
        logProblem(Error::SPAWN_FAILED, "Executable validation failed", "", index + 1, savedError);
        errno = savedError;
        return {};
    }
    for (const auto &arg : m_config.command.args)
    {
        app->addArgument(arg);
    }
    for (const auto &env : m_config.environment)
    {
        app->addEnvironment(env);
    }
    // Keep diagnostics off stdout, which is reserved for response framing.
    // Appending preserves logs when the same worker index is restarted.
    if (!m_config.stderrFilePrefix.empty() &&
        !app->addOpenFDToFile(m_config.stderrFilePrefix + "-" + std::to_string(index + 1) + m_config.stderrFileSuffix,
                              STDERR_FILENO, true))
    {
        const int savedError = errno;
        logProblem(Error::SPAWN_FAILED, "Stderr redirection setup failed", "", index + 1, savedError);
        errno = savedError;
        return {};
    }
    // Capture stdout and stdin only; stderr is inherited or redirected to its file.
    if (!app->spawnProcess(true, false, true))
    {
        const int savedError = errno;
        logProblem(Error::SPAWN_FAILED, "Child creation failed", "", index + 1, savedError);
        errno = savedError;
        return {};
    }
    // Nonblocking pipes let exchange() enforce deadlines even when the child stalls.
    for (const int stream : {STDIN_FILENO, STDOUT_FILENO})
    {
        const int fd = app->getStreamFD(stream);
        const int flags = ::fcntl(fd, F_GETFL);
        if (flags == -1 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            const int savedError = errno;
            logProblem(Error::IO_ERROR, "Nonblocking pipe setup failed", "", index + 1, savedError);
            // A live child must be reaped if pipe setup fails after spawning.
            app->terminate();
            errno = savedError;
            return {};
        }
    }
    // Count only fully configured children that can be used by exchange().
    std::scoped_lock lock(m_statisticsMutex);
    ++m_statistics.processesStarted;
    if (restart)
    {
        ++m_statistics.processesRestarted;
    }
    return app;
}
#endif
