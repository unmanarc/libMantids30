#include "appexec.h"

#ifdef _WIN32
#include <windows.h>

struct sAppPIPE {
    sAppPIPE()
    {
        hRead=0;
        hWrite=0;
    }
    ~sAppPIPE()
    {
        if (hRead) CloseHandle(hRead);
        if (hWrite) CloseHandle(hWrite);
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
            return CX2EXEC_ERR_CREATEPIPE;

        if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0))
            return CX2EXEC_ERR_SETHANDLEINFORMATION;

        return 0;
    }

    HANDLE hRead,hWrite;
};
#endif

CX2::Helpers::sAppExecResult CX2::Helpers::AppExec::blexec(const sAppExecCmd &cmd)
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
    rt.error =CX2EXEC_UNKNOWN;

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
        return {"",-1,CX2EXEC_ERR_CREATEPROCESS};
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
        return {"",-1,CX2EXEC_ERR_WAITFORSINGLEOBJECT};
    }

    DWORD lpExitCode;
    GetExitCodeProcess(runningProcInfo.hProcess, &lpExitCode);

    rt.result = lpExitCode;
    rt.error =CX2EXEC_NOERR;

    CloseHandle(runningProcInfo.hThread);
    CloseHandle(runningProcInfo.hProcess);



#endif
    ///////////////////////////////////////////////////////////////////////////////////
    // Bye:
    return rt;
}


