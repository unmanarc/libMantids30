#ifndef APPEXEC_H
#define APPEXEC_H

#include <string>
#include <vector>

#define CX2EXEC_NOERR 0
#define CX2EXEC_ERR_CREATEPIPE 1
#define CX2EXEC_ERR_SETHANDLEINFORMATION 2
#define CX2EXEC_ERR_WAITFORSINGLEOBJECT 3
#define CX2EXEC_UNKNOWN 4
#define CX2EXEC_ERR_CREATEPROCESS 5


namespace CX2 { namespace Helpers {


struct sAppExecResult {
    // Command output
    std::string output;

    // Result (usually 0: success)
    int result;

    // Errors (DEFINED):
    int error;
};

struct sAppExecCmd
{
    std::string arg0;
    std::vector<std::string> args;
};

class AppExec
{
public:
    AppExec() {}
    /**
     * @brief blexec blocking command execution
     * @param cmd
     * @return command execution result and output
     */
    static sAppExecResult blexec(const sAppExecCmd& cmd);
};


}}
#endif
