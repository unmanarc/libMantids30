#ifndef VARSFILE_H
#define VARSFILE_H

#include <string>
#include <map>
#include <list>

namespace CX2 { namespace Files { namespace Vars {

enum eFileError
{
    INVALID_FILE_FORMAT,
    INVALID_VARNAME_FORMAT,
    CANT_OPEN_FILE,
    FILE_NO_ERROR
};

/**
 * @brief The Vars class
 */
class File
{
public:
    File( const std::string & filePath );

    /**
     * @brief load Load file data into multimap
     * @return true if loaded, false if error.
     */
    bool load();
    /**
     * @brief save Save multimap data to file.
     * @return true if saved, false if error.
     */
    bool save();
    /**
     * @brief setVar Set Variable (Replacing values if exist)
     * @param varName Variable Name
     * @param varValue Variable Value
     */
    void setVar(const std::string & varName, const std::string & varValue);
    /**
     * @brief addVar Add Variable (multimap value, multiple vars with the same name could coexist)
     * @param varName Variable Name
     * @param varValue Variable Value
     */
    void addVar(const std::string & varName, const std::string & varValue);
    /**
     * @brief getVarValues Get Variable Values
     * @param varName Variable Name
     * @return list of values
     */
    std::list<std::string> getVarValues(const std::string & varName);
    /**
     * @brief getVarValue Get Variable Value
     * @param varName Variable Name
     * @param found returns true if varname found, if else false
     * @return First Variable Value
     */
    std::string getVarValue(const std::string & varName, bool * found = nullptr);
    /**
     * @brief getVars Get Variable Vars
     * @return Full Variable Vars map.
     */
    std::multimap<std::string, std::string> getVars() const;
    /**
     * @brief getVars Get Variable Vars in map format (one element per key)
     * @return Full Variable Vars map.
     */
    std::map<std::string, std::string> getVarsMap() const;

    /**
     * @brief getLineVars Get VarName and VarValue from VARNAME:VARVALUE string format
     * @param line string in format VARNAME:VARVALUE
     * @param ok returns true if succeed, false if error happenned (eg. invalid string format)
     * @return pair with varName and VarValue
     */
    std::pair<std::string,std::string> getLineVars( const std::string & line, bool * ok = nullptr );
    /**
     * @brief getLineFromVars Compose File Line Format (VarName:VarValue)
     * @param vars pair with name and value
     * @param ok returns true if succeed, false if error happenned (eg. invalid varname format)
     * @return string composed/sanitized with varName:varValue
     */
    std::string getLineFromVars( const std::pair<std::string,std::string> & vars, bool * ok = nullptr );

    /**
     * @brief getLastError Get Last Error
     * @return last error
     */
    eFileError getLastError() const;

private:
    eFileError lastError;
    std::string filePath;
    std::multimap<std::string,std::string> vars;
};


}}}


#endif // VARSFILE_H
