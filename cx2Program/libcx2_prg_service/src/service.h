#ifndef SERVICE_H
#define SERVICE_H

#include "globalarguments.h"
#include <unistd.h>

static CX2::Application::Arguments::GlobalArguments globalArgs;

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/*
* NOTE: when used as daemon, you have to create this writeable locations:
*
*               /var/log/<program_name>/
*               /var/lock/<program_name>/
*               /var/run/<program_name>/
*/


/**
 * @brief _shutdown function called for program shutdown (close here your descriptors, connections, etc)
 */
void _shutdown();
/**
 * @brief _initvars Function called for variables initialization
 * @param argc
 * @param argv
 * @param globalArguments
 */
void _initvars(int argc, char *argv[], CX2::Application::Arguments::GlobalArguments * globalArguments);
/**
 * @brief _config Function called for config parsing / program initialization
 * @param argc
 * @param argv
 * @param globalArguments
 * @return
 */
bool _config(int argc, char *argv[], CX2::Application::Arguments::GlobalArguments * globalArguments);
/**
 * @brief _start function called for program start
 * @param argc
 * @param argv
 * @param globalArguments
 * @return
 */
int _start(int argc, char *argv[], CX2::Application::Arguments::GlobalArguments * globalArguments);

#endif // SERVICE_H
