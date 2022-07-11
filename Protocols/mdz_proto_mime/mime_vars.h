#ifndef MIME_MULTIPART_H
#define MIME_MULTIPART_H

#include <map>

#include <mdz_mem_vars/vars.h>
#include <mdz_mem_vars/parser.h>

#include "mime_partmessage.h"
#include "mime_sub_firstboundary.h"
#include "mime_sub_endpboundary.h"


namespace Mantids { namespace Protocols { namespace MIME {


class MIME_Message : public Memory::Abstract::Vars, public Memory::Streams::Parser
{
public:
    MIME_Message(Memory::Streams::StreamableObject *value= nullptr);
    ~MIME_Message() override;

    bool streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::StreamableObject::Status & wrStat) override;

    ////////////////////////////////////////////////////////////////////
    //        ------------- VARIABLE GET FUNCTIONS -------------
    ////////////////////////////////////////////////////////////////////
    /**
     * @brief varCount Get the number of variables with an specific name
     * @param varName variable name
     * @return number of elements
     */
    uint32_t varCount(const std::string & varName) override;
    /**
     * @brief getValue Get the memory container for an specific variable
     * @param varName Variable Name
     * @return Memory Container Base
     */
    Memory::Containers::B_Base * getValue(const std::string & varName) override;
    /**
     * @brief getValues Get memory containers for an specific variable name (if one variable name contains multiple definitions)
     * @param varName Variable Name
     * @return list of memory containers
     */
    std::list<Memory::Containers::B_Base *> getValues(const std::string & varName) override;
    /**
     * @brief getKeysList Get Variable Name List
     * @return Variable Name List
     */
    std::set<std::string> getKeysList() override;
    /**
     * @brief isEmpty Get if there is no defined variable
     * @return true if empty
     */
    bool isEmpty() override;
    // TODO: decode as BC pos reference (for file reading).
    /**
     * @brief getMultiPartMessagesByName Get Multipart Messages By Name
     * @param varName Variable Name
     * @return list of part message that match the given name
     */
    std::list<MIME_PartMessage *> getMultiPartMessagesByName(const std::string & varName);
    /**
     * @brief getFirstMessageByName Get the first message by name
     * @param varName Variable name
     * @return The first part message for a given variable name
     */
    MIME_PartMessage * getFirstMessageByName(const std::string & varName);


    ////////////////////////////////////////////////////////////////////
    //        ------------- VARIABLE SET FUNCTIONS -------------
    ////////////////////////////////////////////////////////////////////
    /**
     * @brief addString Add String Variable to the multipart message
     * @param varName Variable Name
     * @param varValue Variable Value
     * @return true if inserted, otherwise false (eg. limits)
     */
    bool addStringVar( const std::string & varName, const std::string & varValue );
    /**
     * @brief addReferecedFile Add File as Variable to the multipart message (will be read when transmitted)
     * @param varName Variable Name
     * @param filePath File Path
     * @return true if file exist and inserted, otherwise false (eg. limits)
     */
    bool addReferecedFileVar(const std::string & varName, const std::string & filePath );

    ////////////////////////////////////////////////////////////////////
    //        ------------- EXTENDED OPTIONS -------------
    ////////////////////////////////////////////////////////////////////
    /**
     * @brief writeVarToFS Write specific variable name to filesystem when receiving/parsing
     * @param varName variable name
     * @param fileName assigned file name
     */
    void writeVarToFS(const std::string &varName, const std::string &fileName);

    ////////////////////////////////////////////////////////////////////
    //        ------------- MULTIPART OPTIONS -------------
    ////////////////////////////////////////////////////////////////////
    /**
     * @brief getMultiPartType Get MultiPart Type (eg. multipart/form-data)
     * @return multipart type string
     */
    std::string getMultiPartType() const;
    /**
     * @brief setMultiPartType Set MultiPart Type (eg. multipart/form-data)
     * @param value multipart type string
     */
    void setMultiPartType(const std::string &value);
    /**
     * @brief getMultiPartBoundary Get the boundary for making the multipart message
     * @return boundary text
     */
    std::string getMultiPartBoundary() const;
    /**
     * @brief setMultiPartBoundary Set the boundary for making the multipart message
     * @param value boundary text
     */
    void setMultiPartBoundary(const std::string &value);

    ////////////////////////////////////////////////////////////////////
    //             ------------- SECURITY OPTIONS -------------
    ////////////////////////////////////////////////////////////////////
    /**
     * @brief makeDataSizeExceptionForPart Create a data size exception for an specific mime part
     * @param partName Part Name
     * @param size max size
     */
    void makeDataSizeExceptionForPart(const std::string & partName, const uint64_t & size);
    /**
     * @brief getMaxParts Get Max number of parts allowed to be decoded on this container
     * @return Max number of parts
     */
    size_t getMaxParts() const;
    /**
     * @brief setMaxParts Set Max number of parts allowed to be decoded on this container
     * @param value max number of parts
     */
    void setMaxParts(const size_t &value);
    /**
     * @brief getMaxHeaderSubOptionsCount Get Max number of options that can be included in a part header
     * @return Max number of options that can be included in a part header
     */
    size_t getMaxHeaderSubOptionsCount() const;
    /**
     * @brief setMaxHeaderSubOptionsCount Set Max number of options that can be included in a part header
     * @param value Max number of options that can be included in a part header
     */
    void setMaxHeaderSubOptionsCount(const size_t &value);
    /**
     * @brief getMaxHeaderSubOptionsSize Get the max size that a option from a header can handle
     * @return max size that a option from a header can have
     */
    size_t getMaxHeaderSubOptionsSize() const;
    /**
     * @brief setMaxHeaderSubOptionsSize Set the max size that a option from a header can handle
     * @param value max size that a option from a header can have
     */
    void setMaxHeaderSubOptionsSize(const size_t &value);
    /**
     * @brief getMaxHeaderOptionsCount Get the max number of options that a part header can handle
     * @return max number of options that a part header can handle
     */
    size_t getMaxHeaderOptionsCount() const;
    /**
     * @brief setMaxHeaderOptionsCount Set the max number of options that a part header can handle
     * @param value max number of options that a part header can handle
     */
    void setMaxHeaderOptionsCount(const size_t &value);
    /**
     * @brief getMaxHeaderOptionSize Get the max header option size
     * @return max header option size
     */
    size_t getMaxHeaderOptionSize() const;
    /**
     * @brief setMaxHeaderOptionSize Set the max header option size
     * @param value max header option size
     */
    void setMaxHeaderOptionSize(const size_t &value);

protected:
    bool initProtocol() override;
    void endProtocol() override;
    bool changeToNextParser() override;

    ///////////////////////////////////////
    // Virtuals for Vars...  Security
    void iSetMaxVarContentSize() override;
    ///////////////////////////////////////

private:

    enum eMIME_VarStat
    {
        MP_STATE_FIRST_BOUNDARY,
        MP_STATE_ENDPOINT,
        MP_STATE_HEADERS,
        MP_STATE_CONTENT
    };

    void addMultiPartMessage(MIME_PartMessage * part);
    std::string getMultiPartMessageName(MIME_PartMessage * part);

    void renewCurrentPart();

    size_t maxParts;
    size_t maxHeaderSubOptionsCount, maxHeaderSubOptionsSize;
    size_t maxHeaderOptionsCount, maxHeaderOptionSize;

    std::string multiPartType, multiPartBoundary;

    std::map<std::string,uint64_t> dataSizeExceptions;
    std::map<std::string,std::string> varToFS;
    std::list<MIME_PartMessage *> parts;
    std::multimap<std::string,MIME_PartMessage *> partsByName;

    eMIME_VarStat currentState;
    MIME_PartMessage * currentPart;
    MIME_Sub_FirstBoundary subFirstBoundary;
    MIME_Sub_EndPBoundary subEndPBoundary;
};


}}}

#endif // MIME_MULTIPART_H
