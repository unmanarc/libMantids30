#include "globalarguments.h"
#include <iostream>
#include <memory>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <set>
#include <limits>

#include <algorithm>
#include <boost/algorithm/string/replace.hpp>

#include <Mantids30/Memory/a_bool.h>
#include <Mantids30/Threads/lock_shared.h>

using namespace std;
using namespace Mantids30::Program::Arguments;
using namespace Mantids30::Memory::Abstract;


GlobalArguments::GlobalArguments()
{
#ifndef _WIN32
    m_sDefaultDaemonOption = "daemon";
#endif
    m_sDefaultHelpOption = "help";
    m_inifiniteWaitAtEnd = false;
    m_extraOptChars = 256;
}

bool GlobalArguments::addCommandLineOption(const string &optGroup, char shortOption, const string &optName, const string &description, const string &defaultValue, const Var::Type & optionType, bool isMandatory)
{
    if (getProgramOption(shortOption))
    {
        return false;
    }
    if (getProgramOption(optName))
    {
        return false;
    }

    std::shared_ptr<CommandLineOption> opts = std::make_shared<CommandLineOption>();
    opts->shortOption = shortOption?shortOption:(m_extraOptChars++);
    opts->name = optName;
    opts->defaultValue = defaultValue;
    opts->description = description;
    opts->optionType = optionType;
    opts->isMandatory = isMandatory;

    if(m_extraOptChars==std::numeric_limits<int>::max())
    {
        throw std::runtime_error("too many command line defined options.");
    }

    m_commandOptions[optGroup].push_back(opts);

    return true;
}

bool GlobalArguments::getCommandLineOptionBooleanValue(const string &optionName)
{
    std::shared_ptr<Var> var = getCommandLineOptionValue(optionName);
    if (!var)
    {
        return false;
    }
    if (var->getVarType() != Var::TYPE_BOOL)
    {
        return false;
    }
    return std::dynamic_pointer_cast<BOOL>(var)->getValue();
}

std::shared_ptr<Var> GlobalArguments::getCommandLineOptionValue(const string &optionName)
{
    std::shared_ptr<CommandLineOption> opt = getProgramOption(optionName);

    if (!opt)
    {
        return nullptr;
    }
    if (opt->parsedOption.size()==0)
    {
        return opt->defaultValVar;
    }
    return opt->parsedOption.front();
}

std::list<std::shared_ptr<Var>> GlobalArguments::getCommandLineOptionValues(const string &optionName)
{
    std::shared_ptr<CommandLineOption> opt = getProgramOption(optionName);

    std::list<std::shared_ptr<Var>> v;
    if (!opt)
    {
        return v;
    }

    if (!opt->parsedOption.size())
    {
        v.push_back(opt->defaultValVar);
        return v;
    }

    return opt->parsedOption;
}

bool GlobalArguments::parseCommandLineOptions(int argc, char *argv[])
{
    std::list<std::shared_ptr<CommandLineOption>> cmdOptions  = getAllCommandLineOptions();

    static string optString;
    static struct option * longOpts = new option[cmdOptions.size()+1];
    size_t iLongOptPos=0;
    for (std::shared_ptr<CommandLineOption> optIter : cmdOptions)
    {
        if (optIter->shortOption && optIter->shortOption < 256)
        {
            // Add option by using char
            char val[2];
            val[0] = optIter->shortOption;
            val[1] = 0;
            optString += string(val) + (optIter->optionType==Var::TYPE_BOOL?"::":":");
        }
        // Put the long option
        longOpts[iLongOptPos++] = {
            optIter->name.c_str(), // Option variable
            optIter->optionType==Var::TYPE_BOOL?optional_argument:required_argument,  // With argument?
            nullptr,
            optIter->shortOption
        };
        // Put the default values:
        optIter->defaultValVar = Var::makeAbstract(optIter->optionType, optIter->defaultValue);
    }
    longOpts[cmdOptions.size()] = { nullptr       , no_argument      , nullptr, 0 };

    std::set<std::shared_ptr<CommandLineOption>> cmdOptionsFulfilled;

    int longIndex;
    int curOptChar = getopt_long(argc, argv, optString.c_str(), longOpts, &longIndex);
    while (curOptChar != -1)
    {
        if (curOptChar == 0)
        {
            if (longOpts[longIndex].val)
                fprintf(stderr, "ERR: Argument -%c / --%s value not specified.\n", longOpts[longIndex].val, longOpts[longIndex].name );
            else
                fprintf(stderr, "ERR: Argument --%s value not specified.\n", longOpts[longIndex].name );

            return false;
        }
        else
        {
            std::shared_ptr<CommandLineOption> optValue = getProgramOption(curOptChar);
            if (optValue)
            {
                std::shared_ptr<Var> absVar = Var::makeAbstract(optValue->optionType, "");

                if (( (optarg!=nullptr && !optarg[0]) || !optarg) && optValue->optionType == Var::TYPE_BOOL)
                {
                    absVar->fromString("1");
                }
                else if (optarg && !absVar->fromString(optarg))
                {
                    if (longOpts[longIndex].val && longOpts[longIndex].val<256)
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
        curOptChar = getopt_long(argc, argv, optString.c_str(), longOpts, &longIndex);
    }


    // Check if all
    bool fulfilled = true;

    for ( std::shared_ptr<CommandLineOption> optIter : cmdOptions )
    {
        if (optIter->isMandatory && cmdOptionsFulfilled.find(optIter) == cmdOptionsFulfilled.end())
        {
            if (optIter->shortOption && optIter->shortOption<256)
                fprintf(stderr, "ERR: Argument -%c / --%s value was required but not parsed.\n", optIter->shortOption, optIter->name.c_str());
            else
                fprintf(stderr, "ERR: Argument --%s value was required but not parsed.\n", optIter->name.c_str());

            fulfilled =false;
        }
    }


    return fulfilled;
}



#ifndef _WIN32
void GlobalArguments::setUid(uint16_t newUid)
{
    m_uid = newUid;
}

void GlobalArguments::setGid(uint16_t newGid)
{
    m_gid = newGid;
}
uint16_t GlobalArguments::getUid() const
{
    return m_uid;
}
uint16_t GlobalArguments::getGid() const
{
    return m_gid;
}
#endif

void GlobalArguments::printHelp()
{
    cout << endl;
    cout << "Help:" << endl;
    cout << "-----" << endl;
    cout << endl;

    for ( auto & i : m_commandOptions )
    {
        cout << i.first << ":" << endl;
        cout << getLine(i.first.size()+1) << endl;

        uint32_t msize = getMaxOptNameSize(i.second);
        for ( std::shared_ptr<CommandLineOption> v : i.second )
        {
            if (v->shortOption && v->shortOption<256)
            {
                string printer = "-%c ";
                printer+=(v->name!="")?"--%-":"  %";
                printer+= to_string(msize) + "s";
                printf(printer.c_str(), v->shortOption, v->name.c_str());
            }
            else
            {
                string printer = "   ";
                printer+=(v->name!="")?"--%-":"  %";
                printer+= to_string(msize) + "s";
                printf(printer.c_str(), v->name.c_str());
            }

            printf(v->optionType != Var::TYPE_BOOL ? " <value>" : "        ");

            if (!v->isMandatory)
            {
                if ( v->optionType != Var::TYPE_BOOL )
                    printf(" : %s (default: %s)\n", v->description.c_str(),  v->defaultValue.c_str() );
                else
                {
                    BOOL defValue;
                    defValue.fromString(v->defaultValue);
                    printf(" : %s (default: %s)\n", v->description.c_str(), defValue.toString().c_str());
                }
            }
            else
                printf(" : %s (required argument)\n", v->description.c_str());
        }
        cout << endl;
    }
}

bool GlobalArguments::addStaticVariable(const string &name, std::shared_ptr<Var> var)
{
    Threads::Sync::Lock_RW lock(m_variablesMutex);
    if (m_variables.find(name) == m_variables.end())
    {
        return false;
    }
    m_variables[name] = var;
    return true;
}

std::shared_ptr<Var> GlobalArguments::getStaticVariable(const string &name)
{
    Threads::Sync::Lock_RD lock(m_variablesMutex);
    if (m_variables.find(name) == m_variables.end())
    {
        return nullptr;
    }
    return m_variables[name];
}


void GlobalArguments::printProgramHeader()
{
    cout << "# " << softwareDescription << " (" <<  softwareName << ") v" << m_version << endl;

    for ( const Program::Values::Author & i : m_authors )
    {
        cout << "# " << "Author:  " << i.name << " (" << i.email << ")" << endl;
    }

    cout << "# " << "License: " << softwareLicense << endl;
    cout << "# " << endl <<  flush;
}

void GlobalArguments::printCurrentProgramOptionsValues()
{
    for ( auto & i : m_commandOptions )
    {
        for ( std::shared_ptr<CommandLineOption> v : i.second )
        {
            std::string varNameToPrint = "";
            if (v->shortOption>0 && v->shortOption<256)
            {
                if (v->name.empty())
                {
                    char x[2] = { 0, 0 };
                    x[0] = v->shortOption;
                    varNameToPrint = x;
                }
                else
                {
                    varNameToPrint = v->name;
                }
            }
            else
            {
                varNameToPrint = v->name;
            }

            for ( auto & var : v->parsedOption )
            {
                printf("--%s=%s\n",varNameToPrint.c_str(), var->toString().c_str());
            }
            if (v->parsedOption.empty())
            {
                printf("--%s=%s\n",varNameToPrint.c_str(), v->defaultValVar->toString().c_str());
            }
        }
    }
    fflush(stdout);
}

std::string GlobalArguments::getCurrentProgramOptionsValuesAsBashLine(bool removeInstall)
{
    std::string r;
    for ( auto & i : m_commandOptions )
    {
        for ( std::shared_ptr<CommandLineOption> v : i.second )
        {
            std::string varNameToPrint = "";
            if (v->shortOption>0 && v->shortOption<256)
            {
                if (v->name.empty())
                {
                    char x[2] = { 0, 0 };
                    x[0] = v->shortOption;
                    varNameToPrint = x;
                }
                else
                {
                    varNameToPrint = v->name;
                }
            }
            else
            {
                varNameToPrint = v->name;
            }

            if (removeInstall && (varNameToPrint == "install" || varNameToPrint == "reinstall" || varNameToPrint == "uninstall"))
                continue;

            for ( auto & var : v->parsedOption )
            {
                r += std::string(r.empty()?"":" ") + "--" + varNameToPrint + "='" + boost::replace_all_copy(var->toString(), "'" , "'\''") + "'";
            }
            if (v->parsedOption.empty())
            {
                r += std::string(r.empty()?"":" ") + "--" + varNameToPrint + "='" + boost::replace_all_copy(v->defaultValVar->toString(), "'" , "'\''") + "'";
            }
        }
    }
    return r;
}

bool GlobalArguments::isInifiniteWaitAtEnd() const
{
    return m_inifiniteWaitAtEnd;
}

void GlobalArguments::setInifiniteWaitAtEnd(bool value)
{
    m_inifiniteWaitAtEnd = value;
}

std::string GlobalArguments::getDefaultHelpOption() const
{
    return m_sDefaultHelpOption;
}

void GlobalArguments::setDefaultHelpOption(const std::string &value)
{
    m_sDefaultHelpOption = value;
}


#ifndef _WIN32
std::string GlobalArguments::getDefaultDaemonOption() const
{
    return m_sDefaultDaemonOption;
}
void GlobalArguments::setDefaultDaemonOption(const std::string &value)
{
    m_sDefaultDaemonOption = value;
}
#endif
std::list<std::shared_ptr<CommandLineOption>> GlobalArguments::getAllCommandLineOptions()
{
    std::list<std::shared_ptr<CommandLineOption>> x;
    for ( auto & i : m_commandOptions )
    {
        for ( std::shared_ptr<CommandLineOption> v : i.second )
        {
            x.push_back(v);
        }
    }
    return x;
}

uint32_t GlobalArguments::getMaxOptNameSize(std::list<std::shared_ptr<CommandLineOption>> options)
{
    unsigned int max = 1;
    for (std::shared_ptr<CommandLineOption> x : options)
    {
        uint32_t cursize = x->name.size(); // + (x->optionType!=TYPE_BOOL? strlen(" <value>") : 0 );
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
std::shared_ptr<CommandLineOption> GlobalArguments::getProgramOption(int shortOption)
{
    if (!shortOption) 
        return nullptr;
    for ( auto & i : m_commandOptions )
    {
        for ( std::shared_ptr<CommandLineOption> v : i.second )
        {
            if (shortOption == v->shortOption) 
        return v;

            if (shortOption>0 && shortOption<256)
            {
                char x[2] = { 0, 0 };
                x[0] = shortOption;
                if (x == v->name) 
        return v;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<CommandLineOption> GlobalArguments::getProgramOption(const std::string &optName)
{
    for ( auto & i : m_commandOptions )
    {
        for ( std::shared_ptr<CommandLineOption> v : i.second )
        {
            if (optName == v->name) 
        return v;
            if (optName.size() == 1 && v->shortOption>0 && v->shortOption<256 && optName.at(0) == v->shortOption) 
        return v;
        }
    }
    return nullptr;
}
