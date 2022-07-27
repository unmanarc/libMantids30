#include "mime_message.h"
#include "mdz_hlp_functions/crypto.h"
#include "mdz_hlp_functions/random.h"
#include "mdz_mem_vars/b_chunks.h"
#include "mdz_mem_vars/streamableobject.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids::Protocols::MIME;
using namespace Mantids;

MIME_Message::MIME_Message(Memory::Streams::StreamableObject *value) : Memory::Streams::Parser(value, false)
{
    initSubParser(&subFirstBoundary);
    initSubParser(&subEndPBoundary);

    currentPart = nullptr;

    multiPartType = "multipart/mixed";

    maxNumberOfParts=128;

    maxHeaderSubOptionsCount=16;
    maxHeaderSubOptionsSize=8*KB_MULT;

    maxHeaderOptionsCount=64;
    maxHeaderOptionSize=8*KB_MULT;

    maxVarContentSize = 32*KB_MULT;
    renewCurrentPart();

    setMultiPartBoundary(Helpers::Random::createRandomString(64));

    currentParser = &subFirstBoundary;
    currentState = MP_STATE_FIRST_BOUNDARY;
}

MIME_Message::~MIME_Message()
{
    if (currentPart) delete currentPart;
    for (MIME_PartMessage * i : allParts) delete i;
}

bool MIME_Message::streamTo(Memory::Streams::StreamableObject *out, Memory::Streams::StreamableObject::Status &wrStat)
{
    Memory::Streams::StreamableObject::Status cur;
    // first boundary:
    if (!(cur+=out->writeString("--" + multiPartBoundary, wrStat)).succeed) return false;
    for (MIME_PartMessage * i : allParts)
    {
        if (!(cur+=out->writeString("\r\n", wrStat)).succeed) return false;
        i->getContent()->initElemParser(out,true);
        i->getHeader()->initElemParser(out,true);
        if (!i->stream(wrStat))
            return false;
        if (!(cur+=out->writeString("--" + multiPartBoundary, wrStat)).succeed) return false;
    }
    if (!(cur+=out->writeString("--\r\n", wrStat)).succeed)
        return false;
    return true;
}

uint32_t MIME_Message::varCount(const std::string &varName)
{
    uint32_t ix=0;
    auto range = partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) ix++;
    return ix;
}

Memory::Streams::StreamableObject *MIME_Message::getValue(const std::string &varName)
{
    auto range = partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i)
        return (((MIME_PartMessage *)i->second)->getContent()->getContentContainer());
    return nullptr;
}

std::list<Memory::Streams::StreamableObject *> MIME_Message::getValues(const std::string &varName)
{
    std::list<Memory::Streams::StreamableObject *> values;
    auto range = partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i)
        values.push_back(((MIME_PartMessage *)i->second)->getContent()->getContentContainer());
    return values;
}

std::set<std::string> MIME_Message::getKeysList()
{
    std::set<std::string> r;
    for ( const auto & i : partsByName ) r.insert(i.first);
    return r;
}

bool MIME_Message::isEmpty()
{
    return partsByName.empty();
}

void MIME_Message::iSetMaxVarContentSize()
{
    currentPart->getContent()->setMaxContentSize(maxVarContentSize);
}

void MIME_Message::renewCurrentPart()
{
    currentPart = new MIME_PartMessage;

    initSubParser(currentPart->getContent());
    initSubParser(currentPart->getHeader());

    currentPart->getContent()->setBoundary( multiPartBoundary );
    currentPart->getContent()->setMaxContentSize(maxVarContentSize);

    // Header:
    currentPart->getHeader()->setMaxOptions(maxHeaderOptionsCount);
    currentPart->getHeader()->setMaxOptionSize(maxHeaderOptionSize);

    // Sub Header:
    currentPart->getHeader()->setMaxSubOptionCount(maxHeaderSubOptionsCount);
    currentPart->getHeader()->setMaxSubOptionSize(maxHeaderSubOptionsSize);
}

void MIME_Message::setCallbackOnHeaderReady(const sMIMECallback &newCallbackOnHeaderReady)
{
    callbackOnHeaderReady = newCallbackOnHeaderReady;
}

void MIME_Message::setCallbackOnContentReady(const sMIMECallback &newCallbackOnContentReady)
{
    callbackOnContentReady = newCallbackOnContentReady;
}

size_t MIME_Message::getMaxHeaderOptionSize() const
{
    return maxHeaderOptionSize;
}

void MIME_Message::setMaxHeaderOptionSize(const size_t &value)
{
    maxHeaderOptionSize = value;
    currentPart->getHeader()->setMaxOptionSize(maxHeaderOptionSize);
}

size_t MIME_Message::getMaxHeaderOptionsCount() const
{
    return maxHeaderOptionsCount;
}

void MIME_Message::setMaxHeaderOptionsCount(const size_t &value)
{
    maxHeaderOptionsCount = value;
    currentPart->getHeader()->setMaxOptions(maxHeaderOptionsCount);
}

size_t MIME_Message::getMaxHeaderSubOptionsSize() const
{
    return maxHeaderSubOptionsSize;
}

void MIME_Message::setMaxHeaderSubOptionsSize(const size_t &value)
{
    maxHeaderSubOptionsSize = value;
    currentPart->getHeader()->setMaxSubOptionSize(maxHeaderSubOptionsSize);
}

size_t MIME_Message::getMaxHeaderSubOptionsCount() const
{
    return maxHeaderSubOptionsCount;
}

void MIME_Message::setMaxHeaderSubOptionsCount(const size_t &value)
{
    maxHeaderSubOptionsCount = value;
    currentPart->getHeader()->setMaxSubOptionCount(maxHeaderSubOptionsCount);
}

size_t MIME_Message::getMaxParts() const
{
    return maxNumberOfParts;
}

void MIME_Message::setMaxParts(const size_t &value)
{
    maxNumberOfParts = value;
}

bool MIME_Message::changeToNextParser()
{
    switch (currentState)
    {
    case MP_STATE_FIRST_BOUNDARY:
    {
        // FIRST_BOUNDARY IS READY
        currentState = MP_STATE_HEADERS;
        currentParser = currentPart->getHeader();
    }break;
    case MP_STATE_ENDPOINT:
    {
        // ENDPOINT IS READY
        if (subEndPBoundary.getStatus() == ENDP_STAT_CONTINUE)
        {
            currentState = MP_STATE_HEADERS;
            currentParser = currentPart->getHeader();
        }
        else
            currentParser = nullptr;
    }break;
    case MP_STATE_HEADERS:
    {
        // HEADERS ARE READY
        // Callback:
        callbackOnHeaderReady.call(getMultiPartMessageName(currentPart),currentPart);
        // GOTO CONTENT:
        currentState = MP_STATE_CONTENT;
        currentParser = currentPart->getContent();
    }break;
    case MP_STATE_CONTENT:
    {
        // CONTENT IS READY
        // Callback:
        callbackOnContentReady.call(getMultiPartMessageName(currentPart),currentPart);

        // Put current Part.
        addMultiPartMessage(currentPart);
        // new current part definition.
        renewCurrentPart();
        // Check if the max parts target is reached.
        if (allParts.size()==maxNumberOfParts)
            currentParser = nullptr; // End here.
        else
        {
            // Goto boundary.
            currentState = MP_STATE_ENDPOINT;
            subEndPBoundary.reset();
            currentParser = &subEndPBoundary;
        }
    }break;
    default:
    {
    }break;
    }
    return true;
}

void MIME_Message::addMultiPartMessage(MIME_PartMessage *part)
{
    allParts.push_back(part);
    // Insert by name:
    std::string varName = getMultiPartMessageName(part);
    if (varName!="") partsByName.insert(std::pair<std::string,MIME_PartMessage*>(boost::to_upper_copy(varName),part));
}

std::string MIME_Message::getMultiPartMessageName(MIME_PartMessage *part)
{
    MIME_HeaderOption * opt = part->getHeader()->getOptionByName("content-disposition");
    if (opt) return opt->getSubVar("name");
    return "";
}

std::list<MIME_PartMessage *> MIME_Message::getMultiPartMessagesByName(const std::string &varName)
{
    std::list<MIME_PartMessage *> values;
    auto range = partsByName.equal_range(boost::to_upper_copy(varName));
    for (auto i = range.first; i != range.second; ++i) values.push_back(i->second);
    return values;
}

MIME_PartMessage *MIME_Message::getFirstMessageByName(const std::string &varName)
{
    if (partsByName.find(boost::to_upper_copy(varName)) == partsByName.end()) return nullptr;
    return partsByName.find(boost::to_upper_copy(varName))->second;
}

bool MIME_Message::addStringVar(const std::string &varName, const std::string &varValue)
{
    // TODO: check max size?
    if (    currentPart->getHeader()->add("Content-Disposition", "form-data") &&
            currentPart->getHeader()->add("name", varName)
            )
    {
        auto * fContainer = new Memory::Containers::B_Chunks;
        fContainer->append(varValue.c_str(),varValue.size());
        currentPart->getContent()->replaceContentContainer(fContainer);
        addMultiPartMessage(currentPart);
        renewCurrentPart();
    }
    else
        return false;
    return true;
}

bool MIME_Message::addReferecedFileVar(const std::string &varName, const std::string &filePath)
{
    // TODO: check max size?
    auto * fContainer = new Memory::Containers::B_MMAP;
    if (!fContainer->referenceFile(filePath,true,false))
    {
        delete fContainer;
        return false;
    }

    if (    currentPart->getHeader()->add("Content-Disposition", "form-data") &&
            currentPart->getHeader()->add("name", varName)
            )
    {
        currentPart->getContent()->replaceContentContainer(fContainer);
        addMultiPartMessage(currentPart);
        renewCurrentPart();
    }
    else
    {
        delete fContainer;
        return false;
    }
    return true;
}

std::string MIME_Message::getMultiPartType() const
{
    return multiPartType;
}

void MIME_Message::setMultiPartType(const std::string &value)
{
    multiPartType = value;
}

std::string MIME_Message::getMultiPartBoundary() const
{
    return multiPartBoundary;
}

void MIME_Message::setMultiPartBoundary(const std::string &value)
{
    multiPartBoundary = value;

    subFirstBoundary.setBoundary(multiPartBoundary);

    if (currentPart)
        currentPart->getContent()->setBoundary(multiPartBoundary);
}

bool MIME_Message::initProtocol()
{
    return true;
}

void MIME_Message::endProtocol()
{

}
