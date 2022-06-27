#include "application.h"

// STD:
#include <stdio.h>
#include <signal.h>

#ifndef _WIN32
#include <syslog.h>
#include <linux/limits.h>
#else
#include <windows.h>
#define strerror_r(errno,buf,len) strerror_s(buf,len,errno)
#endif

#include <fcntl.h>
// STL:
#include <iostream>
#include <fstream>
// SYS:
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <string.h>
#include <unistd.h>

#include <mdz_mem_vars/a_var.h>
#include <mdz_hlp_functions/mem.h>

using namespace std;

void catch_sigterm();

#ifndef _WIN32
// LOCK:
static int lockfd = -1;
static string pidFile;

static void daemonize();
void pidCheck();
void exitRoutine(int, siginfo_t *, void *);

#else

#define	LOG_PID		0x01
#define	LOG_LOCAL5	(21<<3)
#define	LOG_EMERG	0
#define	LOG_ALERT	1
#define	LOG_CRIT	2
#define	LOG_ERR		3
#define	LOG_WARNING	4
#define	LOG_NOTICE	5
#define	LOG_INFO	6
#define	LOG_DEBUG	7

// TODO: IMPLEMENT THIS FUNCTIONS IN WIN32...
void openlog(const char *ident, int option, int facility) {}
int syslog(int type, char *bufp) { return -1; }
int syslog(int type, char *bufp, int len) { return -1; }
void closelog() {}

#endif

using namespace Mantids::Application;

static Application *appPTR = nullptr;

int StartApplication(int argc, char *argv[], Application *_app)
{
#ifndef _WIN32
    pthread_setname_np(pthread_self(), "APP:Start");
#endif

    appPTR = _app;
    // Get program name from program path.
    globalArgs.initProgramName(argv[0]);
#ifndef _WIN32
    globalArgs.setGid(getgid());
    globalArgs.setUid(getuid());
#endif
    // Local default cmd options...
#ifndef _WIN32
    globalArgs.addCommandLineOption("Service Options",   0, "daemon" , "Run as daemon."         , "0", Mantids::Memory::Abstract::TYPE_BOOL );
#endif

    // be careful to sanitize install parameters, because it can affect the security.
    globalArgs.addCommandLineOption("Service Options",   0, "install" , "Install this program with systemd file/service name", "", Mantids::Memory::Abstract::TYPE_STRING );
    globalArgs.addCommandLineOption("Service Options",   0, "reinstall" , "Reinstall this program if it's already installed", "0", Mantids::Memory::Abstract::TYPE_BOOL );
    globalArgs.addCommandLineOption("Service Options",   0, "uninstall" , "Uninstall this program with systemd file/service name", "", Mantids::Memory::Abstract::TYPE_STRING );

    globalArgs.addCommandLineOption("Other Options",     0, "debugparams" , "Debug parameters and exit."         , "0", Mantids::Memory::Abstract::TYPE_BOOL );
    globalArgs.addCommandLineOption("Other Options"  ,   0, "verbose", "Set verbosity level."   , "0", Mantids::Memory::Abstract::TYPE_UINT8 );
    globalArgs.addCommandLineOption("Other Options"  ,  'h', "help"   , "Show information usage.", "0", Mantids::Memory::Abstract::TYPE_BOOL  );

    /////////////////////////
    struct timeval time;
    gettimeofday(&time,nullptr);
    srand(((time.tv_sec * 1000) + (time.tv_usec / 1000))*getpid());

    // Init vars...
    appPTR->_initvars(argc,argv, &globalArgs);
    // Print program description:
    globalArgs.printProgramHeader();
    // Parse program options.
    if (!globalArgs.parseCommandLineOptions(argc,argv))
    {
        cout << "# ERR: Failed to Load CMD Line Parameters." << endl << flush;
        return -2;
    }

    // Check if it's a help program:
    if (globalArgs.getCommandLineOptionBooleanValue(globalArgs.getDefaultHelpOption()))
    {
        globalArgs.printHelp();
        return 0;
    }

    if (globalArgs.getCommandLineOptionBooleanValue("debugparams"))
    {
        globalArgs.printCurrentProgramOptionsValues();
        return 0;
    }

    if (    globalArgs.getCommandLineOptionValue("install")->toString() != "" ||
            globalArgs.getCommandLineOptionValue("uninstall")->toString() != ""
            )
    {
        // :)
#ifdef WIN32

        // TODO: windows service or something like?
        fprintf(stderr,"Not supported yet.\n");

#else
        bool uninstall = false;
        std::string serviceName = globalArgs.getCommandLineOptionValue("install")->toString();
        if (serviceName.empty())
        {
            serviceName = globalArgs.getCommandLineOptionValue("uninstall")->toString();
            uninstall = true;
        }

        // Linux? -> systemd
        std::ofstream outfile;
        std::string serviceFilePath = "/etc/systemd/system/"  +  serviceName  +  ".service";
        if (getuid()!=0)
        {
            char *homedir = getenv("HOME");
            if (homedir == nullptr)
            {
                fprintf(stderr,"# ERR: Undefined HOME directory...\n");
                return -7;
            }
            serviceFilePath = std::string(homedir) + "/.config/systemd/user/"  +  serviceName  +  ".service";
        }

        if (uninstall)
        {
            if (access(serviceFilePath.c_str(), F_OK))
            {
                fprintf(stderr,"# ERR: Can't uninstall service '%s' not installed here\n", serviceName.c_str());
                return -10;
            }

            system( getuid()!=0 ? ("/usr/bin/systemctl --user disable --now " + serviceName).c_str() : ("/usr/bin/systemctl disable --now " + serviceName).c_str());
            unlink(serviceFilePath.c_str());
            system( getuid()!=0 ? "/usr/bin/systemctl --user daemon-reload" : "/usr/bin/systemctl daemon-reload");

            fprintf(stderr,"# Uninstalled systemd service '%s'...\n", serviceName.c_str());
        }
        else
        {
            if (!access(serviceFilePath.c_str(), F_OK) && !globalArgs.getCommandLineOptionBooleanValue("reinstall") )
            {
                fprintf(stderr,"# ERR: Service already exists, try using --reinstall=1...\n");
                return -9;
            }

            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) == NULL)
            {
                fprintf(stderr,"# ERR: Error getting CWD...\n");
                return -11;
            }
            outfile.open(serviceFilePath, std::ios_base::out);

            if (outfile.is_open())
            {

                std::string envs;

                char *ldlibrarypath = getenv("LD_LIBRARY_PATH");
                if (ldlibrarypath != nullptr)
                {
                    envs=std::string("LD_LIBRARY_PATH=") + ldlibrarypath;
                }

                outfile  <<   "[Unit]\n"
                              "Description=" << globalArgs.getDescription() << "\n"
                              "After=network.target\n"
                              "\n"
                              "[Service]\n"
                              "Type=simple\n"
                              "Restart=always\n"
                              "RestartSec=5\n"
                              "WorkingDirectory=" <<  cwd <<  "\n"
                              "ExecStart=" << realpath(argv[0],nullptr) << " "  << globalArgs.getCurrentProgramOptionsValuesAsBashLine(true) << "\n"
                              "Environment=" << envs << "\n"
                              "\n"
                              "[Install]\n"
                              "WantedBy=multi-user.target\n";
                outfile.close();
                system( getuid()!=0 ? "/usr/bin/systemctl --user daemon-reload" : "/usr/bin/systemctl daemon-reload");
                fprintf(stderr,"# Installed as systemd service '%s'...\n", serviceName.c_str());
                fprintf(stderr,"# you can activate it now using: systemctl %senable --now %s\n", getuid()==0?"":"--user ", serviceName.c_str());
            }
            else
            {
                fprintf(stderr,"# ERR: Failed to write into '%s'...\n", serviceFilePath.c_str());
                return -8;
            }
        }
#endif
        return 0;
    }

    // Load/Prepare the configuration based in command line arguments.
    if (!appPTR->_config(argc,argv,&globalArgs))
    {
        cout << "# ERR: Failed to Load Configuration." << endl << flush;
        return -1;
    }

#ifndef _WIN32
    // UID/GID change...
    if (getgid() != globalArgs.getGid() || getuid() != globalArgs.getUid())
    {
        // Drop privileges and act like user process:
        if (getgid() != globalArgs.getGid())
        {
            if (setgid(globalArgs.getGid()))
            {
                cout << "# ERR: Failed to drop privileged to group" << globalArgs.getGid() << endl << flush;
                return -3;
            }
        }
        if (getuid() != globalArgs.getUid())
        {
            if (setuid(globalArgs.getUid()))
            {
                cout << "# ERR: Failed to drop privileged to user" << globalArgs.getUid() << endl << flush;
                return -4;
            }
        }
        // Now change EUID/EGID...
        if (setegid(globalArgs.getGid()) != 0)
        {
            cout << "# ERR: Failed to drop extended privileged to group" << globalArgs.getGid() << endl << flush;
            return -5;
        }
        if (seteuid(globalArgs.getUid()) != 0)
        {
            cout << "# ERR: Failed to drop extended privileged to user" << globalArgs.getUid() << endl << flush;
            return -6;
        }
    }
#endif

    int r = 0;

#ifndef _WIN32
    if (!globalArgs.getCommandLineOptionBooleanValue(globalArgs.getDefaultDaemonOption()))
    {
#endif
        // Allow this application to be killed and setup an exit routine.
        catch_sigterm();

        r = appPTR->_start(argc,argv,&globalArgs);
        if (!globalArgs.isInifiniteWaitAtEnd())
            return r;
        else
        {
            cout<< "# "  << "> This program is running with background threads, press CTRL-C to exit..." << endl << flush;
#ifndef WIN32
            pthread_setname_np(pthread_self(), "Main:LoopWait");
#endif
            for (;;) { sleep(3600); }
        }
#ifndef _WIN32
    }
    else
    {
        // Initialize the logging interface
        openlog( globalArgs.getDaemonName().c_str(), LOG_PID, LOG_LOCAL5);
        syslog( LOG_INFO, "Initiating as service...");

        // Daemonize:
        daemonize();

        // Locked daemon -- check pid
        pidCheck();

        // Allow this application to be killed and setup an exit routine.
        catch_sigterm();

        r = appPTR->_start(argc,argv,&globalArgs);

        if (!globalArgs.isInifiniteWaitAtEnd())
        {
            // Finish up.
            syslog( LOG_NOTICE, "terminated (%d) by program execution", r);
            closelog();
            return r;
        }
        else
        {
            syslog( LOG_NOTICE, "This program (%d) is running with background threads, send kill signal to terminate it.", getpid());
            for (;;) { sleep(3600); }
        }
    }
#endif
}


#ifndef _WIN32
static void child_handler(int signum)
{
    switch (signum)
    {
    case SIGALRM:
        cerr << globalArgs.getDaemonName() << " child handler: SIGALRM" << endl << flush;
        _exit(EXIT_FAILURE);
    case SIGUSR1:
        _exit(EXIT_SUCCESS);
    case SIGCHLD:
        cerr << globalArgs.getDaemonName() << " child handler: SIGCHLD" << endl << flush;
        _exit(EXIT_FAILURE);
    }
}

static int get_lock()
{
    struct flock lplock;

    string lockFile = "/var/lock/" + globalArgs.getDaemonName() + "/state.lock";

    if ((lockfd = open(lockFile.c_str(), O_CREAT | O_RDWR, 0700)) < 0) return 0;

    ZeroBStruct(lplock);

    lplock.l_type = F_WRLCK;
    lplock.l_pid = getpid();

    // Lock this file to my PID.
    if (fcntl(lockfd, F_SETLK, &lplock) < 0) return 0;

    return 1;
}

static void free_lock(void)
{
    if (lockfd >= 0)
        (void) close(lockfd);
}

static void daemonize()
{
    pid_t pid, sid, parent;

    // already a daemon
    if (getppid() == 1)
        return;

    // Trap signals that we expect to recieve
    signal(SIGCHLD, child_handler);
    signal(SIGUSR1, child_handler);
    signal(SIGALRM, child_handler);

    // Fork off the parent process
    pid = fork();
    if (pid < 0)
    {
        char cError[1024]="Unknown Error";

        syslog( LOG_ERR, "unable to fork daemon, code=%d [%s]", errno, strerror_r(errno,cError,sizeof(cError)));
        _exit(EXIT_FAILURE);
    }

    // If we got a good PID, then we can exit the parent process.
    if (pid > 0)
    {
        // Wait for confirmation from the child via SIGTERM or SIGCHLD, or for two seconds to elapse (SIGALRM).  pause() should not return.
        alarm(2);
        pause();
        _exit(EXIT_FAILURE);
    }

    // Executing as child process
    parent = getppid();

    // disable some signals:
    signal(SIGCHLD, SIG_DFL); // Child process dies.
    signal(SIGTSTP, SIG_IGN); // TTY signals
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP, SIG_IGN);  // Ignore soft termination
    signal(SIGTERM, SIG_DFL); // die on hard terminate signal

    umask(0); // Change the mask

    // Create a new session ID for the child process
    sid = setsid();
    if (sid < 0)
    {
        char cError[1024]="Unknown Error";

        syslog( LOG_ERR, "unable to create a new session, code %d (%s)", errno, strerror_r(errno,cError,sizeof(cError)));
        _exit(EXIT_FAILURE);
    }

    // Tell the parent process that we are A-okay
    kill(parent, SIGUSR1);

    // Create the lock file as the current proccess, and if it does not work get out of here.
    if (get_lock() == 0)
    {
        cerr << "ERR: " << globalArgs.getDaemonName() << " unable to create lock file..." << endl << flush;
        fflush(stdout);
        syslog( LOG_ERR, "unable to create lock file.");
        _exit(EXIT_FAILURE);
    }

    // Redirect standard files to /dev/null and log files.
    string logFile_out = "/var/log/" + globalArgs.getDaemonName() + "/stdout.log";
    string logFile_err = "/var/log/" + globalArgs.getDaemonName() + "/stderr.log";
    freopen("/dev/null", "r", stdin);
    freopen( logFile_out.c_str(), "w", stdout);
    freopen( logFile_err.c_str(), "w", stderr);
}

void pidCheck()
{
    pidFile = "/var/run/" + globalArgs.getDaemonName() + "/pid";

    if (!access(pidFile.c_str(), F_OK)) remove(pidFile.c_str());

    ofstream runFile;
    runFile.open(pidFile.c_str());
    runFile << to_string(getpid());
    runFile.close();
}

void exitRoutine(int , siginfo_t *, void *)
{
    fprintf(stderr, "Receiving termination signal for (%s) - pid %d.\n", globalArgs.getDaemonName().c_str(), getpid());
    if (appPTR) appPTR->_shutdown();
    fprintf(stderr, "Finalizing (%s) - pid %d.\n", globalArgs.getDaemonName().c_str(), getpid());

    fflush(stdout);
    if (!pidFile.empty()) remove(pidFile.c_str());
    free_lock();

    _exit(0);
}
#endif


#ifdef _WIN32

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
        fprintf(stderr, "Receiving termination signal for (%s) - pid %d.\n", globalArgs.getDaemonName().c_str(), getpid());
        if (appPTR) appPTR->_shutdown();
        fprintf(stderr, "Finalizing (%s) - pid %d.\n", globalArgs.getDaemonName().c_str(), getpid());
        return TRUE;
        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
        return FALSE;
    case CTRL_LOGOFF_EVENT:
        return FALSE;
    case CTRL_SHUTDOWN_EVENT:
        return FALSE;
    default:
        return FALSE;
    }
}

//void exitHandler(int s)
//{
//    switch (s) {
//    case SIGTERM:
//    case SIGABRT:
//    case SIGINT:
//        fprintf(stderr, "Receiving termination signal for (%s) - pid %d.\n", globalArgs.getDaemonName().c_str(), getpid());
//        _shutdown();
//        fprintf(stderr, "Finalizing (%s) - pid %d.\n", globalArgs.getDaemonName().c_str(), getpid());
//        break;
//    }

//}
#endif



void catch_sigterm()
{
#ifdef _WIN32
    SetConsoleCtrlHandler( CtrlHandler, TRUE );
    //    signal(SIGINT, exitHandler);
    //    signal(SIGTERM, exitHandler);
    //    signal(SIGABRT, exitHandler);
#else
    static struct sigaction _sigact;

    ZeroBStruct(_sigact);

    _sigact.sa_sigaction = exitRoutine;
    _sigact.sa_flags = SA_SIGINFO;

    sigaction(SIGTERM, &_sigact, nullptr);
    sigaction(SIGKILL, &_sigact, nullptr);
    sigaction(SIGINT, &_sigact, nullptr);
    sigaction(SIGHUP, &_sigact, nullptr);
#endif
}


