#include "mime_sub_header.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cx2_hlp_functions/random.h>

using namespace boost;
using namespace boost::algorithm;
using namespace CX2::Network::HTTP;
using namespace CX2;
using namespace std;

MIME_Sub_Header::MIME_Sub_Header()
{
    setParseMode(Memory::Streams::Parsing::PARSE_MODE_DELIMITER);
    setParseDelimiter("\r\n");
    setMaxOptionSize(2*KB_MULT); // 2K per option
    maxOptions = 32; // 32 Max options
    lastOpt = nullptr;
}
MIME_Sub_Header::~MIME_Sub_Header()
{
    for ( auto & i : headers )
    {
        //printf("Deleting %p\n", i.second); fflush(stdout);
        delete i.second;
    }
}

bool MIME_Sub_Header::stream(Memory::Streams::Status & wrStat)
{
    Memory::Streams::Status cur;

    // Write out the header option values...
    for (auto & i : headers)
    {
        std::string x = i.second->getString() + std::string("\r\n");
        if (!(cur+=upStream->writeString( x, wrStat )).succeed) return false;
    }
    if (!(cur+=upStream->writeString("\r\n", wrStat)).succeed) return false;
    return true;
}

bool MIME_Sub_Header::exist(const std::string &optionName) const
{
    return getOptionByName(optionName)!=nullptr?true:false;
}

void MIME_Sub_Header::remove(const std::string &optionName)
{
    auto range = headers.equal_range(boost::to_upper_copy(optionName));

    for (auto it = range.first; it != range.second; )
    {
        delete it->second;
        it = headers.erase(it);
    }
}

void MIME_Sub_Header::replace(const std::string &optionName, const std::string &optionValue)
{
    remove(optionName);
    add(optionName,optionValue);
}

// TODO: sanitize inputs when client...
void MIME_Sub_Header::add(const std::string &optionName, const std::string &optionValue, int state)
{
    HeaderOption * optP;
    if (state == 0)
    {
        optP = new HeaderOption;
        if (headers.size()==maxOptions)
        {
            delete optP;
            return; // Can't exceed.
        }

        optP->setOrigName(optionName);
        parseSubValues(optP,optionValue,state);

        headers.insert(std::pair<std::string,HeaderOption *>(boost::to_upper_copy(optionName),optP));
        lastOpt = optP;
    }
    else if (state == 1 && lastOpt)
    {
        optP = lastOpt;
        parseSubValues(optP,optionValue,state);
    }
}

void MIME_Sub_Header::addHeaderOption(HeaderOption *opt)
{
    if (headers.size()==maxOptions) return; // Can't exceed.
    headers.insert(std::pair<std::string,HeaderOption *>(opt->getUpperName(),opt));
}

std::list<HeaderOption *> MIME_Sub_Header::getOptionsByName(const std::string &varName) const
{
    std::list<HeaderOption *> values;
    auto range = headers.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) values.push_back(i->second);
    return values;
}

HeaderOption *MIME_Sub_Header::getOptionByName(const std::string &varName) const
{
    if (headers.find(boost::to_upper_copy(varName)) == headers.end()) return nullptr;
    return headers.find(boost::to_upper_copy(varName))->second;
}

std::string MIME_Sub_Header::getOptionRawStringByName(const std::string &varName) const
{
    HeaderOption * opt  = getOptionByName(varName);
    return opt?opt->getOrigValue():"";
}

std::string MIME_Sub_Header::getOptionValueStringByName(const std::string &varName) const
{
    HeaderOption * opt  = getOptionByName(varName);
    return opt?opt->getValue():"";
}

uint64_t MIME_Sub_Header::getOptionAsUINT64(const std::string &varName, uint16_t base, bool *optExist) const
{
    uint64_t r=0;
    bool lOptExist;
    if (!optExist) optExist = &lOptExist;
    HeaderOption * opt  = getOptionByName(varName);
    *optExist = opt!=nullptr?true:false;
    if (*optExist) r = strtoull(opt->getValue().c_str(),nullptr,base);
    return r;
}

void MIME_Sub_Header::setMaxOptionSize(const size_t &value)
{
    setParseDataTargetSize(value);
}

void MIME_Sub_Header::setMaxOptions(const size_t &value)
{
    maxOptions = value;
}

Memory::Streams::Parsing::ParseStatus MIME_Sub_Header::parse()
{
    if (!getParsedData()->size()) return Memory::Streams::Parsing::PARSE_STAT_GOTO_NEXT_SUBPARSER;
    parseOptionValue(getParsedData()->toString());
    return Memory::Streams::Parsing::PARSE_STAT_GET_MORE_DATA;
}

void MIME_Sub_Header::parseSubValues(HeaderOption * opt, const std::string &strName, int state)
{
    // hello weo; doaie; fa = "hello world;" hehe; asd=399; aik=""
    std::vector<std::string> vStaticTexts;
    std::string sVarValues = strName;
    // TODO: find a way to do this easier...
    std::string secureReplace = CX2::Helpers::Random::createRandomString(16);
    opt->setOrigValue(sVarValues);

    boost::trim(sVarValues);

    boost::match_flag_type flags = boost::match_default;

    // PRECOMPILE _STATIC_TEXT
    boost::regex exStaticText("\"(?<STATIC_TEXT>[^\"]*)\"");
    boost::match_results<string::const_iterator> whatStaticText;
    for (string::const_iterator start = sVarValues.begin(), end =  sVarValues.end();
         boost::regex_search(start, end, whatStaticText, exStaticText, flags);
         start = sVarValues.begin(), end =  sVarValues.end())
    {
        uint64_t pos = vStaticTexts.size();
        char _staticmsg[128];
        _staticmsg[127]=0;
        snprintf(_staticmsg,127,"_STATIC_%s_%lu",secureReplace.c_str(),pos);
        vStaticTexts.push_back(string(whatStaticText[1].first, whatStaticText[1].second));
        boost::replace_all(sVarValues,"\"" + string(whatStaticText[1].first, whatStaticText[1].second) + "\"" ,_staticmsg);
    }

    vector<string> vValues;
    split(vValues,sVarValues,is_any_of(";"),token_compress_on);

    bool first = true;

    for (const std::string & value : vValues)
    {
        if (boost::contains(value,"="))
        {
            std::string sVarName = value.substr(0,value.find("="));
            std::string sVarValue = value.substr(value.find("=")+1);

            boost::trim(sVarName);
            boost::trim(sVarValue);

            // replace back...
            for (uint64_t i=0; i<vStaticTexts.size();i++)
            {
                char _staticmsg[128];
                _staticmsg[127]=0;
                snprintf(_staticmsg,127,"_STATIC_%s_%lu",secureReplace.c_str(),i);

                boost::replace_all(sVarName,_staticmsg, vStaticTexts[i]);
                boost::replace_all(sVarValue,_staticmsg, vStaticTexts[i]);
            }

            opt->addSubVar(sVarName,sVarValue);
        }
        else
        {
            std::string sv = boost::trim_copy(value);

            // replace back...
            for (uint64_t i=0; i<vStaticTexts.size();i++)
            {
                char _staticmsg[128];
                _staticmsg[127]=0;
                snprintf(_staticmsg,127,"_STATIC_%s_%lu",secureReplace.c_str(),i);

                boost::replace_all(sv,_staticmsg, vStaticTexts[i]);
            }

            //if (!(first && vValues.size()==1))
            opt->addSubVar(sv,"");
        }

        if (first)
        {
            std::string sv = boost::trim_copy(value);

            // replace back...
            for (uint64_t i=0; i<vStaticTexts.size();i++)
            {
                char _staticmsg[128];
                _staticmsg[127]=0;
                snprintf(_staticmsg,127,"_STATIC_%s_%lu",secureReplace.c_str(),i);

                boost::replace_all(sv,_staticmsg, vStaticTexts[i]);
            }

            opt->setValue( sv );
            first=false;
        }

    }

    /*    int usingQuotes = 0;
    size_t prevPos = 0;
    std::string curSubVarName, curSubVarValue;

    for (size_t pos = 0; pos<strName.size()+1;pos++)
    {
        switch(state)
        {
        case 0:
        {   // Looking for the first value.
            if (cStrName[pos]==';' || cStrName[pos]==0)
            {
                opt->setValue(std::string(cStrName,pos));

                state = 1;
                prevPos = pos+1;
            }
        }break;
        case 1:
        {
            usingQuotes=0;
            // Looking for sub var Name.
            if (cStrName[pos]=='=' || cStrName[pos]==0 || cStrName[pos]==';')
            {
                curSubVarName = std::string(cStrName+prevPos,pos-prevPos);
                boost::trim(curSubVarName);
                if ( cStrName[pos]==0 || cStrName[pos]==';' ) // empty var.
                {
                    if (curSubVarName!="")
                    {
                        opt->addSubVar(curSubVarName,"");
                        state = 1;
                    }
                }
                else if (cStrName[pos]=='=') // var with data
                {
                    // There is a variable... look for " or char
                    state = 2;
                }
                prevPos = pos+1;
            }
        }break;
        case 2:
        {
            if ( cStrName[pos]==0 || (!usingQuotes && cStrName[pos]==';') )
            {
                curSubVarValue = std::string(cStrName+prevPos,pos-prevPos);
                boost::trim(curSubVarValue);
                opt->addSubVar(curSubVarName,curSubVarValue);
                state = 1;
                prevPos = pos+1;
            }
            else if (!usingQuotes && cStrName[pos]=='"')
            {
                usingQuotes = 1;
                prevPos = pos+1;
            }
            else if (usingQuotes && cStrName[pos]=='"')
            {
                usingQuotes = 0;
                curSubVarValue = std::string(cStrName+prevPos,pos-prevPos);
                opt->addSubVar(curSubVarName,curSubVarValue);
                state = 3; // Now search for ;
            }
        }break;
        case 3:
        {
            if ( cStrName[pos]==';' )
            {
                // discard data here.
                state = 1;
                prevPos = pos+1;
            }
        }
            break;
        default:
            break;
        }
    }*/
}

void MIME_Sub_Header::parseOptionValue(std::string optionValue)
{
    if (*(optionValue.c_str())==' ' || *(optionValue.c_str())=='\t')
    {
        // Continue on the last option...
        add("",optionValue,1);
    }
    else
    {
        size_t found=optionValue.find(": ");

        if (found!=std::string::npos)
        {
            // We have parameters..
            std::string optionValue2 = optionValue.c_str()+found+2;
            optionValue.resize(found);
            add(optionValue, optionValue2,0);
        }
        else
        {
            // bad option!
        }
    }

}

size_t MIME_Sub_Header::getMaxSubOptionSize() const
{
    return maxSubOptionSize;
}

void MIME_Sub_Header::setMaxSubOptionSize(const size_t &value)
{
    maxSubOptionSize = value;
}

size_t MIME_Sub_Header::getMaxSubOptionCount() const
{
    return maxSubOptionCount;
}

void MIME_Sub_Header::setMaxSubOptionCount(const size_t &value)
{
    maxSubOptionCount = value;
}

std::string HeaderOption::getString()
{
    std::string r;

    r = origName + ": ";

    bool prev = false;
    for (const auto & i : subVar)
    {
        if (prev)
        {
            r+="; ";
        }
        prev=true;
        if (i.second.empty())
        {
            r+= i.first;
        }
        else
        {
            r+= i.first + "=\"" + i.second + "\"";
        }
    }
    return r;

}


bool HeaderOption::isPermited7bitCharset(const std::string &varX)
{
    // No strange chars on our vars...
    for (size_t i=0;i<varX.size();i++)
    {
        if (varX[i]<32 || varX[i]>126) return false;
    }
    return true;
}


void HeaderOption::addSubVar(const std::string &varName, const std::string &varValue)
{
    if (varName.empty() && varValue.empty()) return;
    if (subVar.size()>=maxSubOptionsCount) return;
    if (varName.size()+varValue.size()+curHeaderOptSize>maxHeaderOptSize) return;
    if (!isPermited7bitCharset(varName)) return;
    if (!isPermited7bitCharset(varValue)) return;
    curHeaderOptSize += varName.size()+varValue.size();
    subVar[varName] = varValue;
}

std::string HeaderOption::getUpperName() const
{
    return boost::to_upper_copy(origName);
}

std::string HeaderOption::getOrigName() const
{
    return origName;
}

void HeaderOption::setOrigName(const std::string &value)
{
    origName = value;
}

std::string HeaderOption::getValue() const
{
    return value;
}

void HeaderOption::setValue(const std::string &value)
{
    this->value = value;
    boost::trim(this->value);
}

std::string HeaderOption::getOrigValue() const
{
    return origValue;
}

void HeaderOption::setOrigValue(const std::string &value)
{
    origValue = value;
}

uint64_t HeaderOption::getMaxSubOptions() const
{
    return maxSubOptionsCount;
}

void HeaderOption::setMaxSubOptions(const uint64_t &value)
{
    maxSubOptionsCount = value;
}
