#include "globalarguments.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <set>

#include <cx2_mem_vars/a_bool.h>
#include <cx2_thr_mutex/lock_shared.h>

using namespace std;
using namespace CX2::Application::Arguments;
using namespace CX2::Memory::Abstract;

GlobalArguments::GlobalArguments()
{
#ifndef WIN32
    sDefaultDaemonOption = "daemon";
#endif
    sDefaultHelpOption = "help";
    inifiniteWaitAtEnd = false;
}

bool GlobalArguments::addCommandLineOption(const string &optGroup, char optChar, const string &optName, const string &description, const string &defaultValue, const Type & varType, bool mandatory)
{
    if (getProgramOption(optChar)) return false;
    if (getProgramOption(optName)) return false;

    sProgCMDOpts * opts = new sProgCMDOpts;
    opts->val = optChar;
    opts->name = optName;
    opts->defaultValue = defaultValue;
    opts->description = description;
    opts->varType = varType;
    opts->mandatory = mandatory;

    cmdOptions[optGroup].push_back(opts);

    return true;
}

bool GlobalArguments::getCommandLineOptionBooleanValue(const string &optionName)
{
    Var * var = getCommandLineOptionValue(optionName);
    if (!var) return false;
    if (var->getVarType() != TYPE_BOOL) return false;
    return ((BOOL *)var)->getValue();
}

Var *GlobalArguments::getCommandLineOptionValue(const string &optionName)
{
    sProgCMDOpts * opt = getProgramOption(optionName);

    if (!opt) return nullptr;
    if (opt->parsedOption.size()==0) return opt->defaultValueVar;
    return opt->parsedOption.front();
}

std::list<Var *> GlobalArguments::getCommandLineOptionValues(const string &optionName)
{
    sProgCMDOpts * opt = getProgramOption(optionName);

    std::list<Var *> v;
    if (!opt) return v;

    if (!opt->parsedOption.size())
    {
        v.push_back(opt->defaultValueVar);
        return v;
    }

    return opt->parsedOption;
}

bool GlobalArguments::parseCommandLineOptions(int argc, char *argv[])
{
    std::list<sProgCMDOpts *> cmdOptions  = getAllCMDOptions();

    static string optString;
    static struct option * longOpts = new option[cmdOptions.size()+1];
    size_t i=0;
    for (sProgCMDOpts * optIter : cmdOptions)
    {
        if (optIter->val)
        {
            // Add option by using char
            char val[2];
            val[0] = optIter->val;
            val[1] = 0;
            optString += string(val) + (optIter->varType==TYPE_BOOL?"::":":");
        }
        // Put the long option
        longOpts[i++] = { optIter->name.c_str(), optIter->varType==TYPE_BOOL?optional_argument:required_argument, nullptr, optIter->val };
        // Put the default values:
        optIter->defaultValueVar = Var::makeAbstract(optIter->varType, optIter->defaultValue);
    }
    longOpts[cmdOptions.size()] = { nullptr       , no_argument      , nullptr, 0 };

    std::set<sProgCMDOpts *> cmdOptionsFulfilled;

    int longIndex;
    int opt = getopt_long(argc, argv, optString.c_str(), longOpts, &longIndex);
    while (opt != -1)
    {
        if (opt == 0)
        {
            if (longOpts[longIndex].val)
                fprintf(stderr, "ERR: Argument -%c / --%s value not specified.\n", longOpts[longIndex].val, longOpts[longIndex].name );
            else
                fprintf(stderr, "ERR: Argument --%s value not specified.\n", longOpts[longIndex].name );

            return false;
        }
        else
        {
            sProgCMDOpts * optValue = getProgramOption(opt);
            if (optValue)
            {
                Var * absVar = Var::makeAbstract(optValue->varType, "");

                if (( (optarg!=nullptr && !optarg[0]) || !optarg) && optValue->varType == TYPE_BOOL)
                {
                    absVar->fromString("1");
                }
                else if (optarg && !absVar->fromString(optarg))
                {
                    if (longOpts[longIndex].val)
                        fprintf(stderr, "ERR: Argument -%c / --%s value not parsed correctly.\n", longOpts[longIndex].val, longOpts[longIndex].name );
                    else
                        fprintf(stderr, "ERR: Argument --%s value not parsed correctly.\n",  longOpts[longIndex].name );
                    return false;
                }

                optValue->parsedOption.push_back(absVar);
                cmdOptionsFulfilled.insert(optValue);
            }
            else
            {
                fprintf(stderr, "Unknown Error: parsed option not wired.\n");
            }
        }
        opt = getopt_long(argc, argv, optString.c_str(), longOpts, &longIndex);
    }


    // Check if all
    bool fulfilled = true;

    for ( sProgCMDOpts * optIter : cmdOptions )
    {
        if (optIter->mandatory && cmdOptionsFulfilled.find(optIter) == cmdOptionsFulfilled.end())
        {
            if (optIter->val)
                fprintf(stderr, "ERR: Argument -%c / --%s value was required but not parsed.\n", optIter->val, optIter->name.c_str());
            else
                fprintf(stderr, "ERR: Argument --%s value was required but not parsed.\n", optIter->name.c_str());

            fulfilled =false;
        }
    }


    return fulfilled;
}

void GlobalArguments::printHelp()
{
    cout << endl;
    cout << "Help:" << endl;
    cout << "-----" << endl;
    cout << endl;

    for ( auto & i : cmdOptions )
    {
        cout << i.first << ":" << endl;
        cout << getLine(i.first.size()+1) << endl;

        uint32_t msize = getMaxOptNameSize(i.second);
        for ( sProgCMDOpts * v : i.second )
        {
            if (v->val)
            {
                string printer = "-%c ";
                printer+=(v->name!="")?"--%-":"  %";
                printer+= to_string(msize) + "s";
                printf(printer.c_str(), v->val, v->name.c_str());
            }
            else
            {
                string printer = "   ";
                printer+=(v->name!="")?"--%-":"  %";
                printer+= to_string(msize) + "s";
                printf(printer.c_str(), v->name.c_str());
            }

            printf(v->varType != TYPE_BOOL ? " <value>" : "        ");

            if (!v->mandatory)
                printf(" : %s (default: %s)\n", v->description.c_str(), v->varType != TYPE_BOOL ? v->defaultValue.c_str() : (v->defaultValue!="0"? "true" : "false") );
            else
                printf(" : %s (required argument)\n", v->description.c_str());
        }
        cout << endl;
    }
}

bool GlobalArguments::addStaticVariable(const string &name, Var *var)
{
    Threads::Sync::Lock_RW lock(mutex_vars);
    if (variables.find(name) == variables.end())
    {
        return false;
    }
    variables[name] = var;
    return true;
}

Var *GlobalArguments::getStaticVariable(const string &name)
{
    Threads::Sync::Lock_RD lock(mutex_vars);
    if (variables.find(name) == variables.end())
    {
        return nullptr;
    }
    return variables[name];
}


void GlobalArguments::printProgramHeader()
{
    cout << description << " (" <<  programName << ") v" << version << endl;
    cout << "Author:  " << author << " (" << email << ")" << endl;
    cout << "License: " << license << endl;
    cout << endl <<  flush;
}

bool GlobalArguments::isInifiniteWaitAtEnd() const
{
    return inifiniteWaitAtEnd;
}

void GlobalArguments::setInifiniteWaitAtEnd(bool value)
{
    inifiniteWaitAtEnd = value;
}

std::string GlobalArguments::getDefaultHelpOption() const
{
    return sDefaultHelpOption;
}

void GlobalArguments::setDefaultHelpOption(const std::string &value)
{
    sDefaultHelpOption = value;
}


#ifndef WIN32
std::string GlobalArguments::getDefaultDaemonOption() const
{
    return sDefaultDaemonOption;
}
void GlobalArguments::setDefaultDaemonOption(const std::string &value)
{
    sDefaultDaemonOption = value;
}
#endif
std::list<sProgCMDOpts *> GlobalArguments::getAllCMDOptions()
{
    std::list<sProgCMDOpts *> x;
    for ( auto & i : cmdOptions )
    {
        for ( sProgCMDOpts * v : i.second )
        {
            x.push_back(v);
        }
    }
    return x;
}

uint32_t GlobalArguments::getMaxOptNameSize(std::list<sProgCMDOpts *> options)
{
    unsigned int max = 1;
    for (sProgCMDOpts * x : options)
    {
        uint32_t cursize = x->name.size(); // + (x->varType!=TYPE_BOOL? strlen(" <value>") : 0 );
        if (cursize>max) max = cursize;
    }
    return max;
}

string GlobalArguments::getLine(const uint32_t & size)
{
    string line;
    for (uint32_t i=0;i<size;i++) line+="-";
    return line;
}

// TODO: unicode?
sProgCMDOpts * GlobalArguments::getProgramOption(char optChar)
{
    for ( auto & i : cmdOptions )
    {
        for ( sProgCMDOpts * v : i.second )
        {
            char x[2] = { 0, 0 };
            x[0] = optChar;

            if (optChar == v->val) return v;
            if (x == v->name) return v;
        }
    }
    return nullptr;
}

sProgCMDOpts * GlobalArguments::getProgramOption(const std::string &optName)
{
    for ( auto & i : cmdOptions )
    {
        for ( sProgCMDOpts * v : i.second )
        {
            if (optName == v->name) return v;
            if (optName.size() == 1 && optName.at(0) == v->val) return v;
        }
    }
    return nullptr;
}
