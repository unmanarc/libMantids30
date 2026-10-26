#include "appexec.h"

#ifdef _WIN32
#include <boost/algorithm/string/predicate.hpp>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/types.h>

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
    if (!CreateProcessA(nullptr, (char *) sCmd.c_str(), nullptr, nullptr, TRUE, 0, nullptr, nullptr,
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

#ifndef _WIN32

Mantids30::Helpers::AppSpawn::AppSpawn()
{
    m_fileActionsp = &m_fileActions;

    if ((posix_spawn_file_actions_init(m_fileActionsp)) != 0)
    {
        throw std::runtime_error("Unable to create file actions in execution.");
    }
}

bool Mantids30::Helpers::AppSpawn::setExec(const std::string &path)
{
    if (
#ifndef _WIN32
        !access(path.c_str(), X_OK)
#else
        boost::iends_with(path, ".exe") || boost::iends_with(path, ".bat") || boost::iends_with(path, ".com")
#endif
    )
    {
        m_execPath = path;
        return true;
    }
    return false;
}

bool Mantids30::Helpers::AppSpawn::addOpenFDToFile(const std::string &outFile, int fd)
{
    return posix_spawn_file_actions_addopen(m_fileActionsp, fd, outFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644) == 0;
}

bool Mantids30::Helpers::AppSpawn::redirectStdErrToStdOut()
{
    m_dupStderrToStdout = true;
    return true;
}

void Mantids30::Helpers::AppSpawn::addArgument(const std::string &arg)
{
    m_arguments.push_back(arg);
}

void Mantids30::Helpers::AppSpawn::addEnvironment(const std::string &env)
{
    m_environment.push_back(env);
}

bool Mantids30::Helpers::AppSpawn::spawnProcess(bool pipeStdout, bool pipeStderr)
{
    // Build the argv... (pointers into existing string storage; posix_spawn
    // copies what it needs synchronously, so no strdup is required).
    std::vector<char *> argv;
    argv.reserve(m_arguments.size() + 2);
    argv.push_back(const_cast<char *>(m_execPath.c_str()));
    for (const auto &arg : m_arguments)
    {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    // Build the env... (current environment with our additions first).
    std::vector<std::string> envStrings;
    envStrings.reserve(m_environment.size() + 16);
    envStrings.insert(envStrings.end(), m_environment.begin(), m_environment.end());
    for (int i = 0; environ[i]; i++)
    {
        envStrings.emplace_back(environ[i]);
    }
    std::vector<char *> env;
    env.reserve(envStrings.size() + 1);
    for (auto &e : envStrings)
    {
        env.push_back(e.data());
    }
    env.push_back(nullptr);

    if (pipeStdout)
    {
        if (pipe(m_piStdOut) != 0)
        {
            closePipes();
            return false;
        }
        m_pipedStdout = true;

        // Ask the remote process to close the pipes side 0.
        posix_spawn_file_actions_addclose(m_fileActionsp, m_piStdOut[0]);

        // Ask the remote process to link pipes side 1 with stdout
        posix_spawn_file_actions_adddup2(m_fileActionsp, m_piStdOut[1], STDOUT_FILENO);

        // Ask the remote process to close the pipes side 1
        posix_spawn_file_actions_addclose(m_fileActionsp, m_piStdOut[1]);
    }

    if (pipeStderr)
    {
        if (pipe(m_piStdErr) != 0)
        {
            closePipes();
            return false;
        }
        m_pipedStderr = true;

        // Ask the remote process to close the pipes side 0.
        posix_spawn_file_actions_addclose(m_fileActionsp, m_piStdErr[0]);

        // Ask the remote process to link pipes side 1 with stderr
        posix_spawn_file_actions_adddup2(m_fileActionsp, m_piStdErr[1], STDERR_FILENO);

        // Ask the remote process to close the pipes side 1
        posix_spawn_file_actions_addclose(m_fileActionsp, m_piStdErr[1]);
    }

    // If redirectStdErrToStdOut was requested, dup stderr onto stdout *after*
    // the pipe dup2 above so that stderr ends up in the stdout pipe.
    if (m_dupStderrToStdout && m_pipedStdout)
    {
        posix_spawn_file_actions_adddup2(m_fileActionsp, STDOUT_FILENO, STDERR_FILENO);
    }

    // Execute:
    const int s = posix_spawn(&m_childPid, m_execPath.c_str(), m_fileActionsp, nullptr, &argv[0], &env[0]);

    if (s != 0)
    {
        // Spawn failed: release every pipe end we created.
        closePipes();
        m_arguments.clear();
        m_environment.clear();
        return false;
    }

    m_spawned = true;

    if (m_pipedStdout)
    {
        // close the pipe side 1 in the parent process...
        ::close(m_piStdOut[1]);
        m_piStdOut[1] = -1;
        m_plist = {{m_piStdOut[0], POLLIN}};
    }

    if (m_pipedStderr)
    {
        // close the pipe side 1 in the parent process...
        ::close(m_piStdErr[1]);
        m_piStdErr[1] = -1;
        m_plist.push_back({m_piStdErr[0], POLLIN});
    }

    // posix_spawn has copied argv/env synchronously; clear them so the
    // object is ready for the next spawn without stale data.
    m_arguments.clear();
    m_environment.clear();

    return true;
}

void Mantids30::Helpers::AppSpawn::closePipes()
{
    for (auto &fd : m_piStdOut)
    {
        if (fd != -1)
        {
            ::close(fd);
            fd = -1;
        }
    }
    for (auto &fd : m_piStdErr)
    {
        if (fd != -1)
        {
            ::close(fd);
            fd = -1;
        }
    }
    m_pipedStdout = false;
    m_pipedStderr = false;
}

void Mantids30::Helpers::AppSpawn::reapChild()
{
    if (!m_spawned || m_childExited)
    {
        return;
    }

    int status;
    while (true)
    {
        const pid_t s = waitpid(m_childPid, &status, WUNTRACED | WCONTINUED);
        if (s == m_childPid)
        {
            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                m_childExited = true;
                return;
            }
            continue;
        }
        if (s == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            // ECHILD or any other error: the child is gone (or was never
            // our child); do not block.
            m_childExited = true;
            return;
        }
    }
}

Mantids30::Helpers::AppSpawn::~AppSpawn()
{
    closePipes();
    reapChild();
    if (m_fileActionsp != nullptr)
    {
        posix_spawn_file_actions_destroy(m_fileActionsp);
    }
}

void Mantids30::Helpers::AppSpawn::waitUntilProcessEnds()
{
    reapChild();
}

bool Mantids30::Helpers::AppSpawn::hasExited() const
{
    return m_childExited;
}

void Mantids30::Helpers::AppSpawn::reset()
{
    closePipes();
    reapChild();
    m_arguments.clear();
    m_environment.clear();
    m_childPid = 0;
    m_spawned = false;
    m_childExited = false;
    m_pipedStdout = false;
    m_pipedStderr = false;
    m_dupStderrToStdout = false;
    m_plist.clear();
    m_fileActions = {};
    posix_spawn_file_actions_init(m_fileActionsp);
}

std::set<int> Mantids30::Helpers::AppSpawn::pollResponse()
{
    std::set<int> r;
    if (m_plist.empty())
    {
        return r;
    }
    if (poll(&m_plist[0], m_plist.size(), -1) > 0)
    {
        for (auto &i : m_plist)
        {
            if ((i.revents & POLLIN) != 0)
            {
                if (i.fd == m_piStdOut[0])
                {
                    r.insert(STDOUT_FILENO);
                }
                else if (i.fd == m_piStdErr[0])
                {
                    r.insert(STDERR_FILENO);
                }
            }
        }
    }
    return r;
}

ssize_t Mantids30::Helpers::AppSpawn::read(int fd, void *buf, size_t count)
{
    if (fd == STDOUT_FILENO)
    {
        if (!m_pipedStdout || m_piStdOut[0] == -1)
        {
            return -1;
        }
        return ::read(m_piStdOut[0], buf, count);
    }
    else if (fd == STDERR_FILENO)
    {
        if (!m_pipedStderr || m_piStdErr[0] == -1)
        {
            return -1;
        }
        return ::read(m_piStdErr[0], buf, count);
    }

    throw std::runtime_error("AppSpawn: You should use stderr or stdout magic numbers in read function.");
}
#endif
