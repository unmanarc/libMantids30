#include "appexec.h"

#ifdef _WIN32
#include <boost/algorithm/string/predicate.hpp>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#include <string.h>
#include <stdexcept>
#include <sys/types.h>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>

struct sAppPIPE {
    sAppPIPE()
    {
        hRead=INVALID_HANDLE_VALUE;
        hWrite=INVALID_HANDLE_VALUE;
    }
    ~sAppPIPE()
    {
        if (hRead!=INVALID_HANDLE_VALUE) CloseHandle(hRead);
        if (hWrite!=INVALID_HANDLE_VALUE) CloseHandle(hWrite);
    }

    SECURITY_ATTRIBUTES createInheritedSecAttribs()
    {
        SECURITY_ATTRIBUTES r;

        r.bInheritHandle = TRUE; // inherited pipe handles...
        r.lpSecurityDescriptor = nullptr;
        r.nLength = sizeof(SECURITY_ATTRIBUTES);

        return r;
    }
    int init()
    {
        SECURITY_ATTRIBUTES secAttribs = createInheritedSecAttribs();

        if (!CreatePipe(&hRead, &hWrite, &secAttribs, 0))
            return APPEXEC_ERR_CREATEPIPE;

        if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0))
            return APPEXEC_ERR_SETHANDLEINFORMATION;

        return 0;
    }

    HANDLE hRead,hWrite;
};
#endif

Mantids::Helpers::AppExec::sAppExecResult Mantids::Helpers::AppExec::blexec(const sAppExecCmd &cmd)
{
    sAppExecResult rt;

#ifdef _WIN32
    ///////////////////////////////////////////////////////////////////////////////////
    // Variables definition
    STARTUPINFOA procInteractParams;
    PROCESS_INFORMATION runningProcInfo;
    sAppPIPE pipes;
    std::string sCmd;

    rt.output="";
    rt.result=-1;
    rt.error =APPEXEC_UNKNOWN;

    // Structs sanitization:
    memset(&runningProcInfo,0, sizeof(PROCESS_INFORMATION));
    memset(&procInteractParams,0, sizeof(STARTUPINFO));

    ///////////////////////////////////////////////////////////////////////////////////
    // create the execution path:
    sCmd+=cmd.arg0;
    for (size_t i=0;i<cmd.args.size();i++)
    {
        sCmd+=" " + cmd.args.at(i);
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // Initialize the pipes for getting the process output:
    int err;
    if ((err=pipes.init())!=0)
        return {"",-1,err};

    procInteractParams.cb = sizeof(STARTUPINFO);
    procInteractParams.hStdInput = nullptr;
    procInteractParams.dwFlags |= STARTF_USESTDHANDLES;
    procInteractParams.hStdError = pipes.hWrite;
    procInteractParams.hStdOutput = pipes.hWrite;

    ///////////////////////////////////////////////////////////////////////////////////
    // Execute the process:
    if (!CreateProcessA(nullptr,(char *)sCmd.c_str(),nullptr,nullptr,TRUE,0,nullptr,nullptr,
                        &procInteractParams,  // STARTUPINFO process interaction
                        &runningProcInfo))
    {
        return {"",-1,APPEXEC_ERR_CREATEPROCESS};
    }

    CloseHandle(pipes.hWrite);

    ///////////////////////////////////////////////////////////////////////////////////
    // Read the output:
    char buf[4096];
    for (;;)
    {
        DWORD bytesRead;
        if (    !ReadFile( pipes.hRead, buf, 4096, &bytesRead, nullptr)
                || bytesRead == 0 )
            break;
        rt.output += std::string(buf,0,bytesRead);
    }
    memset(buf,0,sizeof(buf));

    ///////////////////////////////////////////////////////////////////////////////////
    // Wait until program finishes:
    if (WaitForSingleObject(runningProcInfo.hProcess, INFINITE) == WAIT_FAILED)
    {
        CloseHandle(runningProcInfo.hThread);
        CloseHandle(runningProcInfo.hProcess);
        return {"",-1,APPEXEC_ERR_WAITFORSINGLEOBJECT};
    }

    DWORD lpExitCode;
    GetExitCodeProcess(runningProcInfo.hProcess, &lpExitCode);

    rt.result = lpExitCode;
    rt.error =APPEXEC_NOERR;

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

Mantids::Helpers::AppSpawn::AppSpawn()
{
    attrp = nullptr;
    file_actionsp = &file_actions;

    if ((posix_spawn_file_actions_init (file_actionsp)) != 0)
        throw std::runtime_error("Unable to create file actions in execution.");

}

bool Mantids::Helpers::AppSpawn::setExec(const std::string &path)
{
    if (
        #ifndef _WIN32
            !access(path.c_str(),X_OK)
        #else
            boost::iends_with(path,".exe") || boost::iends_with(path,".bat") || boost::iends_with(path,".com")
        #endif
            )
    {
        execPath = path;
        return true;
    }
    return false;
}

bool Mantids::Helpers::AppSpawn::addOpenFDToFile(const std::string &outFile, int fd)
{
    return posix_spawn_file_actions_addopen (file_actionsp, fd, outFile.c_str(),
                                             O_WRONLY | O_CREAT | O_TRUNC, 0644)==0;
}

bool Mantids::Helpers::AppSpawn::redirectStdErrToStdOut()
{
    return posix_spawn_file_actions_adddup2 (file_actionsp, STDOUT_FILENO, STDERR_FILENO)==0;
}

void Mantids::Helpers::AppSpawn::addArgument(const std::string &arg)
{
    arguments.push_back(arg);
}

void Mantids::Helpers::AppSpawn::addEnvironment(const std::string &env)
{
    environment.push_back(env);
}

bool Mantids::Helpers::AppSpawn::spawnProcess(bool pipeStdout, bool pipeStderr)
{
    // Create the argv...
    char ** argv = (char **)malloc( (arguments.size()+2)*sizeof(char *) );
    argv[0] = (char *)strdup(execPath.c_str());
    for (size_t i=1; i<(arguments.size()+1);i++)
        argv[i] = (char *)strdup(arguments[i-1].c_str());
    argv[arguments.size()+1] = 0;

    // Create the env...
    std::vector<std::string> _environment = environment;
    // Pass the current environ...
    for (int i=0;environ[i];i++ )
        _environment.push_back(environ[i]);

    char ** env = (char **)malloc( (_environment.size()+1)*sizeof(char *) );
    for (size_t i=0; i<_environment.size();i++)
        env[i] = (char *)strdup(_environment[i].c_str());
    env[_environment.size()] = 0;


    if (pipeStdout)
    {
        if(pipe(piStdOut)!=0)
            throw std::runtime_error("Unable to create pipes.");

        // Ask the remote process to close the pipes side 0.
        posix_spawn_file_actions_addclose(file_actionsp, piStdOut[0]);

        // Ask the remote process to link pipes side 1 with stdout
        posix_spawn_file_actions_adddup2(file_actionsp, piStdOut[1], STDOUT_FILENO);

        // Ask the remote process to close the pipes side 1
        posix_spawn_file_actions_addclose(file_actionsp, piStdOut[1]);
    }

    if (pipeStderr)
    {
        if(pipe(piStdErr)!=0)
            throw std::runtime_error("Unable to create pipes.");

        // Ask the remote process to close the pipes side 0.
        posix_spawn_file_actions_addclose(file_actionsp, piStdErr[0]);

        // Ask the remote process to link pipes side 1 with stderr
        posix_spawn_file_actions_adddup2(file_actionsp, piStdErr[1], STDERR_FILENO);

        // Ask the remote process to close the pipes side 1
        posix_spawn_file_actions_addclose(file_actionsp, piStdErr[1]);
    }


    // Execute:
    int s = posix_spawn(&child_pid, execPath.c_str(), file_actionsp, attrp,
                        &argv[0], env);

    if (pipeStdout)
    {
        // close the pipe side 1 in the parent process..
        close(piStdOut[1]);
        plist = { {piStdOut[0],POLLIN} };
    }

    if (pipeStderr)
    {
        // close the pipe side 1 in the parent process..
        close(piStdErr[1]);
        plist.push_back({piStdErr[0],POLLIN});
    }

    // Destroy objects:
    for (int i=0; argv[i]; i++) free(argv[i]);
    free(argv);

    // Destroy objects:
    for (int i=0; env[i]; i++) free(env[i]);
    free(env);


    if (attrp != nullptr && ((s = posix_spawnattr_destroy(attrp))!=0))
        throw std::runtime_error("Unable to destroy execution attributes.");
    if (file_actionsp != nullptr && ((s = posix_spawn_file_actions_destroy(file_actionsp)))!=0 )
        throw std::runtime_error("Unable to destroy file actions execution.");

    return s==0;
}

void Mantids::Helpers::AppSpawn::waitUntilProcessEnds()
{
    int s,status;
    do
    {
        s = waitpid(child_pid, &status, WUNTRACED | WCONTINUED);
        if (s == -1)
        {
            return;
        }

    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
}

std::set<int> Mantids::Helpers::AppSpawn::pollResponse()
{
    std::set<int> r;
    if (  poll(&plist[0],plist.size(),-1)>0 )
    {
        for (size_t i=0; i<plist.size(); i++)
        {
            if ( (plist[i].revents&POLLIN)!=0 )
            {
                if ( plist[i].fd == piStdOut[0] )
                    r.insert(STDOUT_FILENO);
                else if ( plist[i].fd == piStdErr[0] )
                    r.insert(STDERR_FILENO);
            }
        }
    }
    return r;
}

ssize_t Mantids::Helpers::AppSpawn::read(int fd, void *buf, size_t count)
{
    if (fd == STDOUT_FILENO)
        return ::read(piStdOut[0], buf, count);
    else if (fd == STDERR_FILENO)
        return ::read(piStdErr[0], buf, count);

    throw std::runtime_error("AppSpawn: You should use stderr or stdout magic numbers in read function.");
}
#endif
