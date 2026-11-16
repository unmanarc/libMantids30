#include "mime_message.h"
#include <Mantids30/Core/Memory/parser.h>

#include <Mantids30/Core/Helpers/crypto.h>
#include <Mantids30/Core/Helpers/random.h>
#include <Mantids30/Core/Memory/b_chunks.h>
#include <Mantids30/Core/Memory/streamable_object.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <memory>

using namespace boost;
using namespace boost::algorithm;
using namespace Mantids30::Protocol::MIME;
using namespace Mantids30;

MIME_Message::MIME_Message(const std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> &value)
    : Mantids30::Core::Memory::Streams::Parser(value, false)
{
    m_currentSubParser = &m_subFirstBoundary;

    renewCurrentPart();
    setMultiPartBoundary(Mantids30::Core::Helpers::Random::createRandomString(64));
}

std::shared_ptr<MIME_Message> MIME_Message::create(const std::shared_ptr<StreamableObject> &value)
{
    std::shared_ptr<MIME_Message> x = std::shared_ptr<MIME_Message>(new MIME_Message(value));
    x->initSubParser(&x->m_subFirstBoundary);
    x->initSubParser(&x->m_subEndPBoundary);
    x->m_initialized = x->initProtocol();
    return x;
}

bool MIME_Message::streamTo(Mantids30::Core::Memory::Streams::StreamableObject *out)
{
    for (const auto &part : m_allParts)
    {
        if (!out->writeString("--" + m_multiPartBoundary + "\r\n"))
        {
            return false;
        }
        part->getHeader()->initElemParser(out, true);
        const bool headersSent = part->getHeader()->streamToUpstream();
        part->getHeader()->initElemParser(nullptr, true);
        if (!headersSent || !part->getContent()->getContentContainer()->streamTo(out) || !out->writeString("\r\n"))
        {
            return false;
        }
    }
    return out->writeString("--" + m_multiPartBoundary + "--\r\n");
}

void MIME_Message::clear()
{
    m_currentPart = nullptr;
    m_allParts.clear();
    m_partsByName.clear();
}

uint32_t MIME_Message::varCount(const std::string &varName)
{
    uint32_t ix = 0;
    std::pair<std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator, std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator> range = m_partsByName.equal_range(
        boost::to_upper_copy(varName));
    for (std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator i = range.first; i != range.second; ++i)
    {
        ix++;
    }
    return ix;
}

std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> MIME_Message::getValue(const std::string &varName)
{
    std::pair<std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator, std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator> range = m_partsByName.equal_range(
        boost::to_upper_copy(varName));
    for (std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator i = range.first; i != range.second; ++i)
    {
        return ((std::shared_ptr<MIME_PartMessage>(i->second))->getContent()->getContentContainer());
    }
    return nullptr;
}

std::list<std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject>> MIME_Message::getValues(const std::string &varName)
{
    std::list<std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject>> values;
    std::pair<std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator, std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator> range = m_partsByName.equal_range(
        boost::to_upper_copy(varName));
    for (std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator i = range.first; i != range.second; ++i)
    {
        values.push_back((std::shared_ptr<MIME_PartMessage>(i->second))->getContent()->getContentContainer());
    }
    return values;
}

std::set<std::string> MIME_Message::getKeysList()
{
    std::set<std::string> r;
    for (const auto &i : m_partsByName)
    {
        r.insert(i.first);
    }
    return r;
}

bool MIME_Message::isEmpty()
{
    return m_partsByName.empty();
}

void MIME_Message::iSetMaxVarContentSize()
{
    m_currentPart->getContent()->setMaxContentSize(m_maxVarContentSize);
}

void MIME_Message::renewCurrentPart()
{
    m_currentPart = std::make_shared<MIME_PartMessage>();

    initSubParser(m_currentPart->getContent());
    initSubParser(m_currentPart->getHeader());

    m_currentPart->getContent()->setBoundary(m_multiPartBoundary);
    m_currentPart->getContent()->setMaxContentSize(m_maxVarContentSize);

    // Header:
    m_currentPart->getHeader()->setMaxOptions(m_maxHeaderOptionsCount);
    m_currentPart->getHeader()->setMaxOptionSize(m_maxHeaderOptionSize);

    // Sub Header:
    m_currentPart->getHeader()->setMaxSubOptionCount(m_maxHeaderSubOptionsCount);
    m_currentPart->getHeader()->setMaxSubOptionSize(m_maxHeaderSubOptionsSize);
}

void MIME_Message::setCallbackOnHeaderReady(const MIMECallback &newCallbackOnHeaderReady)
{
    m_onHeaderReady = newCallbackOnHeaderReady;
}

void MIME_Message::setCallbackOnContentReady(const MIMECallback &newCallbackOnContentReady)
{
    m_onContentReady = newCallbackOnContentReady;
}

size_t MIME_Message::getMaxHeaderOptionSize() const
{
    return m_maxHeaderOptionSize;
}

void MIME_Message::setMaxHeaderOptionSize(const size_t &value)
{
    m_maxHeaderOptionSize = value;
    m_currentPart->getHeader()->setMaxOptionSize(m_maxHeaderOptionSize);
}

size_t MIME_Message::getMaxHeaderOptionsCount() const
{
    return m_maxHeaderOptionsCount;
}

void MIME_Message::setMaxHeaderOptionsCount(const size_t &value)
{
    m_maxHeaderOptionsCount = value;
    m_currentPart->getHeader()->setMaxOptions(m_maxHeaderOptionsCount);
}

size_t MIME_Message::getMaxHeaderSubOptionsSize() const
{
    return m_maxHeaderSubOptionsSize;
}

void MIME_Message::setMaxHeaderSubOptionsSize(const size_t &value)
{
    m_maxHeaderSubOptionsSize = value;
    m_currentPart->getHeader()->setMaxSubOptionSize(m_maxHeaderSubOptionsSize);
}

size_t MIME_Message::getMaxHeaderSubOptionsCount() const
{
    return m_maxHeaderSubOptionsCount;
}

void MIME_Message::setMaxHeaderSubOptionsCount(const size_t &value)
{
    m_maxHeaderSubOptionsCount = value;
    m_currentPart->getHeader()->setMaxSubOptionCount(m_maxHeaderSubOptionsCount);
}

bool MIME_Message::changeToNextParser()
{
    switch (m_currentParsingStatus)
    {
    case MIMEParsingStatus::FIRST_BOUNDARY:
    {
        // FIRST_BOUNDARY IS READY
        m_currentParsingStatus = MIMEParsingStatus::HEADERS;
        m_currentSubParser = m_currentPart->getHeader();
        return true;
    }
    break;
    case MIMEParsingStatus::ENDPOINT:
    {
        // ENDPOINT IS READY
        if (m_subEndPBoundary.getStatus() == ENDP_STAT_CONTINUE)
        {
            m_currentParsingStatus = MIMEParsingStatus::HEADERS;
            m_currentSubParser = m_currentPart->getHeader();
        }
        else
        {
            m_currentSubParser = nullptr;
        }
        return true;
    }
    break;
    case MIMEParsingStatus::HEADERS:
    {
        // HEADERS ARE READY
        // Callback:
        m_onHeaderReady.call(getMultiPartMessageName(m_currentPart), m_currentPart);
        // GOTO CONTENT:
        m_currentParsingStatus = MIMEParsingStatus::CONTENT;
        m_currentSubParser = m_currentPart->getContent();
        return true;
    }
    break;
    case MIMEParsingStatus::CONTENT:
    {
        // CONTENT IS READY
        // Callback:
        m_onContentReady.call(getMultiPartMessageName(m_currentPart), m_currentPart);

        // Put current Part.
        bool r = addMultiPartMessage(m_currentPart);
        // new current part definition.
        renewCurrentPart();

        // Check if the max parts target is reached.
        if (!r)
        {
            m_currentSubParser = nullptr; // End here.
        }
        else
        {
            // Goto boundary.
            m_currentParsingStatus = MIMEParsingStatus::ENDPOINT;
            m_subEndPBoundary.reset();
            m_currentSubParser = &m_subEndPBoundary;
        }
        return r;
    }
    break;
    default:
        return true;
    }
}

bool MIME_Message::addMultiPartMessage(const std::shared_ptr<MIME_PartMessage> &part)
{
    if (m_allParts.size() >= m_maxVarsCount)
    {
        return false;
    }

    m_allParts.push_back(part);

    // Insert by name:
    std::string varName = getMultiPartMessageName(part);
    if (!varName.empty())
    {
        m_partsByName.insert(std::pair<std::string, std::shared_ptr<MIME_PartMessage>>(boost::to_upper_copy(varName), part));
        return true;
    }

    return false;
}

std::string MIME_Message::getMultiPartMessageName(const std::shared_ptr<MIME_PartMessage> &part)
{
    std::shared_ptr<MIME_HeaderOption> opt = part->getHeader()->getOptionByName("content-disposition");
    if (opt)
    {
        return opt->getSubVar("name");
    }
    return "";
}

std::list<std::shared_ptr<MIME_PartMessage>> MIME_Message::getMultiPartMessagesByName(std::string_view varName)
{
    std::list<std::shared_ptr<MIME_PartMessage>> values;
    const std::string upperName = boost::to_upper_copy(std::string(varName));
    std::pair<std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator, std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator> range = m_partsByName.equal_range(upperName);
    for (std::multimap<std::string, std::shared_ptr<MIME_PartMessage>>::iterator i = range.first; i != range.second; ++i)
    {
        values.push_back(i->second);
    }
    return values;
}

std::shared_ptr<MIME_PartMessage> MIME_Message::getFirstMessageByName(std::string_view varName)
{
    const std::string upperName = boost::to_upper_copy(std::string(varName));
    if (m_partsByName.find(upperName) == m_partsByName.end())
    {
        return nullptr;
    }
    return m_partsByName.find(upperName)->second;
}

bool MIME_Message::addStringVar(const std::string &key, const std::string &value)
{
    return addStreamableObjectContainer(key, std::make_shared<Mantids30::Core::Memory::Containers::B_Chunks>(value));
}

bool MIME_Message::addReferecedFileVar(const std::string &varName, const std::string &filePath)
{
    const size_t separator = filePath.find_last_of("/\\");
    const std::string fileName = filePath.substr(separator == std::string::npos ? 0 : separator + 1);
    if (fileName.empty() || fileName.find_first_of("\"\\\r\n") != std::string::npos)
    {
        return false;
    }
    std::shared_ptr<Mantids30::Core::Memory::Containers::B_MMAP> fContainer = std::make_shared<Mantids30::Core::Memory::Containers::B_MMAP>();
    if (!fContainer->referenceFile(filePath, true, false))
    {
        return false;
    }
    if (!addStreamableObjectContainer(varName, fContainer))
    {
        return false;
    }
    auto header = m_allParts.back()->getHeader();
    header->replace("Content-Disposition", "form-data; name=\"" + varName + "\"; filename=\"" + fileName + "\"");
    header->replace("Content-Type", "application/octet-stream");
    return true;
}

bool MIME_Message::addVar(const std::string &varName, const std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> &data)
{
    return addStreamableObjectContainer(varName, data);
}

bool MIME_Message::addStreamableObjectContainer(const std::string &varName, const std::shared_ptr<StreamableObject> &obj)
{
    if (!obj || varName.empty() || varName.find_first_of("\"\\\r\n") != std::string::npos)
    {
        return false;
    }
    if (m_currentPart->getHeader()->add("Content-Disposition", "form-data; name=\"" + varName + "\""))
    {
        m_currentPart->getContent()->replaceContentContainer(obj);
        bool r = addMultiPartMessage(m_currentPart);
        renewCurrentPart();
        return r;
    }
    else
    {
        return false;
    }
}

std::string MIME_Message::getMultiPartType() const
{
    return m_multiPartType;
}

void MIME_Message::setMultiPartType(const std::string &value)
{
    m_multiPartType = value;
}

std::string MIME_Message::getMultiPartBoundary() const
{
    return m_multiPartBoundary;
}

void MIME_Message::setMultiPartBoundary(const std::string &value)
{
    m_multiPartBoundary = value;

    m_subFirstBoundary.setBoundary(m_multiPartBoundary);

    if (m_currentPart)
    {
        m_currentPart->getContent()->setBoundary(m_multiPartBoundary);
    }
}

bool MIME_Message::initProtocol()
{
    return true;
}

void MIME_Message::endProtocol() {}
