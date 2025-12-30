#pragma once

#include <Mantids30/Memory/subparser.h>
#include <Mantids30/Helpers/json.h>

#include <memory>
#include <string>
#include <map>
#include <list>

/*
 * TODO: Security: check if other servers can handle the MIME properly...
 */

namespace Mantids30 { namespace Network { namespace Protocols { namespace MIME {
// ??
/**
 * @brief The HeaderOption struct
 */
class MIME_HeaderOption
{
public:
    MIME_HeaderOption()
    {
        m_maxHeaderOptSize=8*KB_MULT;
        m_curHeaderOptSize=0;
        m_maxSubOptionsCount=16;
    }

    std::string getSubVar(const std::string & subVarName)
    {
        if (m_subVar.find(subVarName) == m_subVar.end()) 
            return "";
        return m_subVar.find(subVarName)->second;
    }

    std::list<std::string> getSubVars(const std::string & subVarName)
    {
        std::list<std::string> r;
        auto ret = m_subVar.equal_range(subVarName);
        for (std::multimap<std::string,std::string>::iterator it=ret.first; it!=ret.second; ++it)
        {
              r.push_back(it->second);
        }
        return r;
    }

    std::multimap<std::string, std::string> getAllSubVars()
    {
        return m_subVar;
    }

    std::string toString();

    void addSubVar(const std::string & varName, const std::string & varValue);

    std::string getUpperName() const;

    std::string getOrigName() const;
    void setOrigName(const std::string &value);

    std::string getValue() const;
    void setValue(const std::string &value);

    std::string getOrigValue() const;
    void setOrigValue(const std::string &value);

    uint64_t getMaxSubOptions() const;
    void setMaxSubOptions(const uint64_t &value);

private:
    bool isPermited7bitCharset(const std::string & varX);

    uint64_t m_maxSubOptionsCount;
    uint64_t m_maxHeaderOptSize;
    uint64_t m_curHeaderOptSize;

    std::string m_origName;
    std::string m_origValue;
    std::string m_value;
    std::multimap<std::string,std::string> m_subVar;
};

class MIME_Sub_Header : public Memory::Streams::SubParser
{
public:
    MIME_Sub_Header();

    bool streamToUpstream( ) override;

    void clear();

    /**
     * @brief exist exist option.
     * @param optionName
     * @return
     */
    bool exist(const std::string &optionName) const;
    /**
     * @brief remove Remove option from MIME Structure.
     * @param optionName option Name.
     */
    void remove(const std::string &optionName);
    /**
     * @brief replace Remove the option (if exist), and add you own.
     * @param optionName option Name.
     * @param optionValue option value.
     */
    void replace(const std::string &optionName, const std::string &optionValue);
    /**
     * @brief add Add Header Option.
     * @param optionName option Name.
     * @param optionValue option value.
     * @param state 1: continue with the last option. 0: new option.
     */
    bool add(const std::string &optionName, const std::string &optionValue, int state = 0);
    /**
     * @brief getOptionsSize Get Number of Options (headers)
     * @return number of options/headers
     */
    size_t getOptionsSize();
    /**
     * @brief getOptionsByName Get option list by name (multiple options with the same name (eg. set-cookie)
     * @param varName value name.
     * @return list of HeaderOption's
     */
    std::list<std::shared_ptr<MIME_HeaderOption>> getOptionsByName(const std::string &varName) const;
    /**
     * @brief getOptionByName Get the first value
     * @param varName
     * @return nullptr if not exist.
     */
    std::shared_ptr<MIME_HeaderOption> getOptionByName(const std::string & varName) const;
    /**
     * @brief getOptionRawStringByName Get Option STD String By Name (raw, as came from the input)
     * @param varName variable name.
     * @return "" if nothing found, or the original option value.
     */
    std::string getOptionRawStringByName(const std::string & varName) const;
    /**
     * @brief getOptionValueStringByName
     * @param varName
     * @return
     */
    std::string getOptionValueStringByName(const std::string & varName) const;

    /**
     * @brief getOptionAsUINT64
     * @param varName
     * @param optExist
     * @return
     */
    uint64_t getOptionAsUINT64(const std::string & varName, uint16_t base = 10, bool * optExist = nullptr) const;
    //////////////////////////////////////////////////

    bool addHeaderOption(std::shared_ptr<MIME_HeaderOption> opt);

    //////////////////////////////////////////////////
    // Security:
    void setMaxOptionSize(const size_t &value);
    void setMaxOptions(const size_t &value);

    size_t getMaxSubOptionCount() const;
    void setMaxSubOptionCount(const size_t &value);

    size_t getMaxSubOptionSize() const;
    void setMaxSubOptionSize(const size_t &value);

protected:
    Memory::Streams::SubParser::ParseStatus parse() override;

private:
    void parseSubValues(std::shared_ptr<MIME_HeaderOption> opt, const std::string &strName);

    std::shared_ptr<MIME_HeaderOption> m_lastOpt;
    void parseOptionValue(std::string optionValue);

    std::multimap<std::string,std::shared_ptr<MIME_HeaderOption>> m_headers;
    size_t m_maxOptions = 32; // 32 Max options
    size_t m_maxSubOptionCount=100, m_maxSubOptionSize=2*KB_MULT;
};

}}}}

