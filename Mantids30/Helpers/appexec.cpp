#include "appexec.h"
#include <climits>
#include <map>
#include <system_error>

#ifdef _WIN32
#include <boost/algorithm/string/predicate.hpp>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>

struct WinExecPIPE
{
    WinExecPIPE()
    {
        hRead = INVALID_HANDLE_VALUE;
        hWrite = INVALID_HANDLE_VALUE;
    }
    ~WinExecPIPE()
    {
        if (hRead != INVALID_HANDLE_VALUE)
            CloseHandle(hRead);
        if (hWrite != INVALID_HANDLE_VALUE)
            CloseHandle(hWrite);
    }

    SECURITY_ATTRIBUTES createInheritedSecAttribs()
    {
        SECURITY_ATTRIBUTES r;

        r.bInheritHandle = TRUE; // inherited pipe handles...
        r.lpSecurityDescriptor = nullptr;
        r.nLength = sizeof(SECURITY_ATTRIBUTES);

        return r;
    }
    Mantids30::Helpers::AppExec::Status init()
    {
        SECURITY_ATTRIBUTES secAttribs = createInheritedSecAttribs();

        if (!CreatePipe(&hRead, &hWrite, &secAttribs, 0))
            return Mantids30::Helpers::AppExec::Status::PIPE_CREATION_FAILED;

        if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0))
            return Mantids30::Helpers::AppExec::Status::HANDLE_INFORMATION_SETTING_FAILED;

        return Mantids30::Helpers::AppExec::Status::SUCCESS;
    }

    HANDLE hRead, hWrite;
};
#endif

Mantids30::Helpers::AppExec::ExecutionResult Mantids30::Helpers::AppExec::blexec(const ExecutionArgs &cmd)
{
    ExecutionResult rt;

#ifdef _WIN32
    ///////////////////////////////////////////////////////////////////////////////////
    // Variables definition
    STARTUPINFOA procInteractParams;
    PROCESS_INFORMATION runningProcInfo;
    WinExecPIPE pipes;
    std::string sCmd;

    rt.output.clear();
    rt.exitCode = -1;
    rt.status = AppExec::Status::UNKNOWN_ERROR;

    // Structs sanitization:
    memset(&runningProcInfo, 0, sizeof(PROCESS_INFORMATION));
    memset(&procInteractParams, 0, sizeof(STARTUPINFO));

    ///////////////////////////////////////////////////////////////////////////////////
    // create the execution path:
    sCmd += cmd.arg0;
    for (size_t i = 0; i < cmd.args.size(); i++)
    {
        sCmd += " " + cmd.args.at(i);
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Initialize the pipes for getting the process output:
    AppExec::Status err;
    if ((err = pipes.init()) != AppExec::Status::SUCCESS)
        return {"", -1, err};

    procInteractParams.cb = sizeof(STARTUPINFO);
    procInteractParams.hStdInput = nullptr;
    procInteractParams.dwFlags |= STARTF_USESTDHANDLES;
    procInteractParams.hStdError = pipes.hWrite;
    procInteractParams.hStdOutput = pipes.hWrite;

    ///////////////////////////////////////////////////////////////////////////////////
    // Execute the process:
    if (!CreateProcessA(nullptr,
                        (char *) sCmd.c_str(),
                        nullptr,
                        nullptr,
                        TRUE,
                        0,
                        nullptr,
                        nullptr,
                        &procInteractParams, // STARTUPINFO process interaction
                        &runningProcInfo))
    {
        return {"", -1, AppExec::Status::PROCESS_CREATION_FAILED};
    }

    CloseHandle(pipes.hWrite);

    ///////////////////////////////////////////////////////////////////////////////////
    // Read the output:
    char buf[4096];
    for (;;)
    {
        DWORD bytesRead;
        if (!ReadFile(pipes.hRead, buf, 4096, &bytesRead, nullptr) || bytesRead == 0)
            break;
        rt.output += std::string(buf, 0, bytesRead);
    }
    memset(buf, 0, sizeof(buf));

    ///////////////////////////////////////////////////////////////////////////////////
    // Wait until program finishes:
    if (WaitForSingleObject(runningProcInfo.hProcess, INFINITE) == WAIT_FAILED)
    {
        CloseHandle(runningProcInfo.hThread);
        CloseHandle(runningProcInfo.hProcess);
        return {"", -1, AppExec::Status::WAIT_FOR_PROCESS_FAILED};
    }

    DWORD lpExitCode;
    GetExitCodeProcess(runningProcInfo.hProcess, &lpExitCode);

    rt.exitCode = lpExitCode;
    rt.status = AppExec::Status::SUCCESS;

    CloseHandle(runningProcInfo.hThread);
    CloseHandle(runningProcInfo.hProcess);

#else
    // TODO:

#endif
    ///////////////////////////////////////////////////////////////////////////////////
    // Bye:
    return rt;
}
// cpp

#ifndef _WIN32

Mantids30::Helpers::AppSpawn::AppSpawn() = default;

Mantids30::Helpers::AppSpawn::~AppSpawn()
{
    // Closing captured streams can cause SIGPIPE/EPIPE in the child.
    // Destruction waits for the child; it does not terminate it.
    closePipes();
    reapChild();
}

bool Mantids30::Helpers::AppSpawn::setExec(const std::string &path)
{
    if (m_spawned)
    {
        errno = EBUSY;
        return false;
    }

    m_execPath.clear();

    if (path.empty() || path.find('\0') != std::string::npos)
    {
        errno = EINVAL;
        return false;
    }

    struct stat st{};
    if (::stat(path.c_str(), &st) == -1)
    {
        return false;
    }

    if (!S_ISREG(st.st_mode))
    {
        errno = EACCES;
        return false;
    }

    if (::access(path.c_str(), X_OK) == -1)
    {
        return false;
    }

    m_execPath = path;
    return true;
}

bool Mantids30::Helpers::AppSpawn::addOpenFDToFile(const std::string &outFile, int fd)
{
    if (m_spawned)
    {
        errno = EBUSY;
        return false;
    }

    if (fd < 0 || fd == INT_MAX || outFile.empty() || outFile.find('\0') != std::string::npos)
    {
        errno = EINVAL;
        return false;
    }

    m_openFiles.push_back({outFile, fd});
    return true;
}

bool Mantids30::Helpers::AppSpawn::redirectStdErrToStdOut()
{
    if (m_spawned)
    {
        errno = EBUSY;
        return false;
    }

    m_dupStderrToStdout = true;
    return true;
}

void Mantids30::Helpers::AppSpawn::addArgument(const std::string &arg)
{
    if (m_spawned)
    {
        throw std::logic_error("AppSpawn: reset before changing arguments.");
    }

    if (arg.find('\0') != std::string::npos)
    {
        throw std::invalid_argument("AppSpawn: argument contains NUL.");
    }

    m_arguments.push_back(arg);
}

void Mantids30::Helpers::AppSpawn::addEnvironment(const std::string &env)
{
    if (m_spawned)
    {
        throw std::logic_error("AppSpawn: reset before changing environment.");
    }

    const auto separator = env.find('=');
    if (separator == std::string::npos || separator == 0 || env.find('\0') != std::string::npos)
    {
        throw std::invalid_argument("AppSpawn: expected NAME=VALUE.");
    }

    m_environment.push_back(env);
}

bool Mantids30::Helpers::AppSpawn::spawnProcess(bool pipeStdout, bool pipeStderr)
{
    if (m_spawned)
    {
        errno = EBUSY;
        return false;
    }

    if (m_execPath.empty())
    {
        errno = EINVAL;
        return false;
    }

    closePipes();

    if (m_dupStderrToStdout)
    {
        pipeStderr = false;
    }

    std::vector<char *> argv;
    argv.reserve(m_arguments.size() + 2);
    argv.push_back(const_cast<char *>(m_execPath.c_str()));

    for (auto &arg : m_arguments)
    {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }

    argv.push_back(nullptr);

    // Later assignments replace earlier values with the same name.
    std::map<std::string, std::string> environment;

    if (environ != nullptr)
    {
        for (char **entry = environ; *entry != nullptr; ++entry)
        {
            const std::string value(*entry);
            const auto separator = value.find('=');
            if (separator != std::string::npos && separator != 0)
            {
                environment[value.substr(0, separator)] = value;
            }
        }
    }

    for (const auto &value : m_environment)
    {
        environment[value.substr(0, value.find('='))] = value;
    }

    std::vector<char *> env;
    env.reserve(environment.size() + 1);
    for (auto &entry : environment)
    {
        env.push_back(const_cast<char *>(entry.second.c_str()));
    }
    env.push_back(nullptr);

    // Allocate before spawning so successful spawn cannot be followed
    // by an allocation failure while recording the captured streams.
    m_plist.reserve(2);

    struct FileActions
    {
        posix_spawn_file_actions_t value{};
        bool initialized{false};

        ~FileActions()
        {
            if (initialized)
            {
                posix_spawn_file_actions_destroy(&value);
            }
        }
    } actions;

    int error = posix_spawn_file_actions_init(&actions.value);
    if (error != 0)
    {
        errno = error;
        return false;
    }
    actions.initialized = true;

    const auto fail = [&](int code)
    {
        closePipes();
        errno = code;
        return false;
    };

    // Keep pipe descriptors above all redirection targets so earlier
    // addopen/dup2 actions cannot overwrite a pipe descriptor.
    int minimumFD = 3;
    for (const auto &file : m_openFiles)
    {
        if (file.fd >= minimumFD)
        {
            minimumFD = file.fd + 1;
        }
    }

    const auto createPipe = [&](int (&ends)[2]) -> bool
    {
#ifdef __linux__
        if (::pipe2(ends, O_CLOEXEC) == -1)
        {
            return false;
        }
#else
        // On platforms without pipe2(), creation and CLOEXEC setup are
        // not atomic relative to concurrent fork/exec.
        if (::pipe(ends) == -1)
        {
            return false;
        }
#endif

        for (int &fd : ends)
        {
            if (fd < minimumFD)
            {
                const int replacement = ::fcntl(fd, F_DUPFD_CLOEXEC, minimumFD);

                if (replacement == -1)
                {
                    return false;
                }

                ::close(fd);
                fd = replacement;
            }
            else
            {
                if (::fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
                {
                    return false;
                }
            }
        }

        return true;
    };

    for (const auto &file : m_openFiles)
    {
        error = posix_spawn_file_actions_addopen(&actions.value, file.fd, file.path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (error != 0)
        {
            return fail(error);
        }
    }

    const auto addPipeActions = [&](int (&ends)[2], int target) -> int
    {
        int result = posix_spawn_file_actions_addclose(&actions.value, ends[0]);
        if (result != 0)
        {
            return result;
        }

        result = posix_spawn_file_actions_adddup2(&actions.value, ends[1], target);
        if (result != 0)
        {
            return result;
        }

        return posix_spawn_file_actions_addclose(&actions.value, ends[1]);
    };

    if (pipeStdout)
    {
        if (!createPipe(m_piStdOut))
        {
            return fail(errno);
        }

        error = addPipeActions(m_piStdOut, STDOUT_FILENO);
        if (error != 0)
        {
            return fail(error);
        }
    }

    if (pipeStderr)
    {
        if (!createPipe(m_piStdErr))
        {
            return fail(errno);
        }

        error = addPipeActions(m_piStdErr, STDERR_FILENO);
        if (error != 0)
        {
            return fail(error);
        }
    }

    if (m_dupStderrToStdout)
    {
        error = posix_spawn_file_actions_adddup2(&actions.value, STDOUT_FILENO, STDERR_FILENO);
        if (error != 0)
        {
            return fail(error);
        }
    }

    pid_t childPid = 0;
    error = posix_spawn(&childPid, m_execPath.c_str(), &actions.value, nullptr, argv.data(), env.data());

    if (error != 0)
    {
        return fail(error);
    }

    m_childPid = childPid;
    m_spawned = true;
    m_childExited = false;
    m_pipedStdout = pipeStdout;
    m_pipedStderr = pipeStderr;

    if (pipeStdout)
    {
        ::close(m_piStdOut[1]);
        m_piStdOut[1] = -1;
        m_plist.push_back({m_piStdOut[0], POLLIN, 0});
    }

    if (pipeStderr)
    {
        ::close(m_piStdErr[1]);
        m_piStdErr[1] = -1;
        m_plist.push_back({m_piStdErr[0], POLLIN, 0});
    }

    m_arguments.clear();
    m_environment.clear();
    return true;
}

void Mantids30::Helpers::AppSpawn::closeStream(int fd) noexcept
{
    int *ends = nullptr;

    if (fd == STDOUT_FILENO)
    {
        ends = m_piStdOut;
        m_pipedStdout = false;
    }
    else if (fd == STDERR_FILENO)
    {
        ends = m_piStdErr;
        m_pipedStderr = false;
    }
    else
    {
        return;
    }

    const int readFD = ends[0];

    for (auto it = m_plist.begin(); it != m_plist.end();)
    {
        if (it->fd == readFD)
        {
            it = m_plist.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        if (ends[i] != -1)
        {
            ::close(ends[i]);
            ends[i] = -1;
        }
    }
}

void Mantids30::Helpers::AppSpawn::closePipes() noexcept
{
    const int savedErrno = errno;
    closeStream(STDOUT_FILENO);
    closeStream(STDERR_FILENO);
    m_plist.clear();
    errno = savedErrno;
}

bool Mantids30::Helpers::AppSpawn::reapChild() noexcept
{
    if (!m_spawned || m_childExited)
    {
        return true;
    }

    int status = 0;
    pid_t result;

    do
    {
        result = ::waitpid(m_childPid, &status, 0);
    }
    while (result == -1 && errno == EINTR);

    if (result == m_childPid || (result == -1 && errno == ECHILD))
    {
        m_childExited = true;
        m_childPid = 0;
        return true;
    }

    return false;
}

void Mantids30::Helpers::AppSpawn::waitUntilProcessEnds()
{
    if (!reapChild())
    {
        throw std::system_error(errno, std::generic_category(), "AppSpawn: waitpid");
    }
}

bool Mantids30::Helpers::AppSpawn::hasExited() const
{
    return m_childExited;
}

void Mantids30::Helpers::AppSpawn::reset()
{
    closePipes();
    waitUntilProcessEnds();

    m_execPath.clear();
    m_arguments.clear();
    m_environment.clear();
    m_openFiles.clear();

    m_childPid = 0;
    m_spawned = false;
    m_childExited = false;
    m_dupStderrToStdout = false;
}

std::set<int> Mantids30::Helpers::AppSpawn::pollResponse()
{
    std::set<int> ready;

    if (m_plist.empty())
    {
        return ready;
    }

    int result;
    do
    {
        result = ::poll(m_plist.data(), m_plist.size(), -1);
    }
    while (result == -1 && errno == EINTR);

    if (result == -1)
    {
        throw std::system_error(errno, std::generic_category(), "AppSpawn: poll");
    }

    for (const auto &entry : m_plist)
    {
        if ((entry.revents & POLLNVAL) != 0)
        {
            throw std::system_error(EBADF, std::generic_category(), "AppSpawn: invalid pipe");
        }

        if ((entry.revents & POLLERR) != 0)
        {
            throw std::system_error(EIO, std::generic_category(), "AppSpawn: pipe error");
        }

        // HUP can coexist with unread data. Let read() drain the pipe
        // and remove it only after a positive-size read returns EOF.
        if ((entry.revents & (POLLIN | POLLHUP)) != 0)
        {
            if (entry.fd == m_piStdOut[0])
            {
                ready.insert(STDOUT_FILENO);
            }
            else if (entry.fd == m_piStdErr[0])
            {
                ready.insert(STDERR_FILENO);
            }
        }
    }

    return ready;
}

ssize_t Mantids30::Helpers::AppSpawn::read(int fd, void *buf, size_t count)
{
    int pipeFD = -1;

    if (fd == STDOUT_FILENO)
    {
        pipeFD = m_piStdOut[0];
    }
    else if (fd == STDERR_FILENO)
    {
        pipeFD = m_piStdErr[0];
    }

    if (pipeFD == -1)
    {
        errno = EBADF;
        return -1;
    }

    ssize_t result;
    do
    {
        result = ::read(pipeFD, buf, count);
    }
    while (result == -1 && errno == EINTR);

    if (result == 0 && count != 0)
    {
        closeStream(fd);
    }

    return result;
}

#endif
