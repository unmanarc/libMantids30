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

    struct sAppExecCmd
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
    static ExecutionResult blexec(const sAppExecCmd &cmd);
};

#ifndef _WIN32
class AppSpawn
{
public:
    AppSpawn();

    /**
     * @brief setExec Set executable path
     * @param path path of executable
     * @return true if exist and it's an executable file, otherwise false.
     */
    bool setExec(const std::string &path);

    /**
     * @brief addOpenFD Add Open File Descriptor on the application
     * @param outFile file used...
     */
    bool addOpenFDToFile(const std::string &outFile, int fd = 1);

    /**
     * @brief redirectStdErrToStdOut Add dup STDERR to STDOUT
     */
    bool redirectStdErrToStdOut();

    /**
     * @brief addArgument Add program argument.
     * @param arg argument text
     */
    void addArgument(const std::string &arg);

    /**
     * @brief addEnvironment Add program environment var.
     * @param env environment var (in form of VAR=VALUE)
     */
    void addEnvironment(const std::string &env);

    /**
     * @brief spawnProcess Initiate the process
     * @param pipeStdout set to pipe here the stdout from the child
     * @param pipeStderr set to pipe here the stderr from the child
     * @return
     */
    bool spawnProcess(bool pipeStdout = false, bool pipeStderr = false);

    /**
     * @brief waitUntilDies Wait until the process die.
     */
    void waitUntilProcessEnds() const;

    /**
     * @brief pollResponse Poll response (detect from where there is data to read, stdout/stdin), remember to wait for the process after the pollResponse loop.
     * @return list of file descriptors that have some bytes to read
     */
    std::set<int> pollResponse();
    /**
     * @brief read Read from the child application file descriptor
     * @param fd file descriptor to read (1-STDOUT,2-STDERR)
     * @param buf buffer to put the data
     * @param count buffer size
     * @return On  success,  the number of bytes read is returned (zero indicates end of file), otherwise -1.
     */
    ssize_t read(int fd, void *buf, size_t count);

    // TODO: write to the process..

private:
    std::string m_execPath;
    std::vector<std::string> m_arguments;
    std::vector<std::string> m_environment;
    pid_t m_childPid{0};
    posix_spawnattr_t m_attr{};
    posix_spawnattr_t *m_attrp = nullptr;
    posix_spawn_file_actions_t m_fileActions{};
    posix_spawn_file_actions_t *m_fileActionsp = nullptr;
    int m_piStdOut[2]{0, 0};
    int m_piStdErr[2]{0, 0};
    std::vector<pollfd> m_plist;
};
#endif

} // namespace Mantids30::Helpers
