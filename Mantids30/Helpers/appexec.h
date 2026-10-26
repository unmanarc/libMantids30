#pragma once

#ifndef _WIN32
#include <cstdio>
#include <poll.h>
#include <set>
#include <spawn.h>
#endif

#include <string>
#include <vector>

// Define STDOUT_FILENO/STDERR_FILENO:
#include <cstdint>
#include <unistd.h>

namespace Mantids30::Helpers {

class AppExec
{
public:
    enum class Status : uint8_t
    {
        SUCCESS = 0,
        PIPE_CREATION_FAILED = 1,
        HANDLE_INFORMATION_SETTING_FAILED = 2,
        WAIT_FOR_PROCESS_FAILED = 3,
        UNKNOWN_ERROR = 4,
        PROCESS_CREATION_FAILED = 5
    };

    struct ExecutionResult
    {
        // Command output
        std::string output;

        // Result (usually 0: success)
        int exitCode;

        // Execution Status (DEFINED):
        AppExec::Status status;
    };

    struct ExecutionArgs
    {
        std::string arg0;
        std::vector<std::string> args;
    };

    AppExec() = default;
    /**
     * @brief blexec blocking command execution
     * @param cmd
     * @return command execution result and output
     */
    static ExecutionResult blexec(const ExecutionArgs &cmd);
};
// h

#ifndef _WIN32
class AppSpawn
{
public:
    AppSpawn();
    ~AppSpawn();

    AppSpawn(const AppSpawn &) = delete;
    AppSpawn &operator=(const AppSpawn &) = delete;
    AppSpawn(AppSpawn &&) = delete;
    AppSpawn &operator=(AppSpawn &&) = delete;

    bool setExec(const std::string &path);
    bool addOpenFDToFile(const std::string &outFile, int fd = 1);
    bool redirectStdErrToStdOut();

    void addArgument(const std::string &arg);
    void addEnvironment(const std::string &env);

    // One successful spawn per reset().
    // stderr-to-stdout redirection takes precedence over pipeStderr.
    bool spawnProcess(bool pipeStdout = false, bool pipeStderr = false);

    // Drain captured output before calling this method.
    void waitUntilProcessEnds();

    // Returns descriptors ready for data or EOF.
    // Empty means all captured streams have reached EOF.
    std::set<int> pollResponse();

    // A positive-size read returning zero closes that captured stream.
    ssize_t read(int fd, void *buf, size_t count);

    // True once the child has been reaped, or waitpid reports ECHILD.
    [[nodiscard]] bool hasExited() const;

    // Closes pipes and waits for the previous child before clearing state.
    void reset();

private:
    struct OpenFD
    {
        std::string path;
        int fd;
    };

    std::string m_execPath;
    std::vector<std::string> m_arguments;
    std::vector<std::string> m_environment;
    std::vector<OpenFD> m_openFiles;

    pid_t m_childPid{0};
    int m_piStdOut[2]{-1, -1};
    int m_piStdErr[2]{-1, -1};
    std::vector<pollfd> m_plist;

    bool m_pipedStdout{false};
    bool m_pipedStderr{false};
    bool m_dupStderrToStdout{false};
    bool m_spawned{false};
    bool m_childExited{false};

    void closePipes() noexcept;
    void closeStream(int fd) noexcept;
    bool reapChild() noexcept;
};
#endif

} // namespace Mantids30::Helpers
