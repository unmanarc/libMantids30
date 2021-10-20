#ifndef JSON_H
#define JSON_H

#include <json/json.h>

typedef Json::Value json;

// jsoncpp macros:
#define JSON_ASCSTRING(j,x,def) (j[x].isString()?j[x].asCString():def)
#define JSON_ASSTRING(j,x,def) (j[x].isString()?j[x].asString():def)
#define JSON_ASBOOL(j,x,def) (j[x].isBool()?j[x].asBool():def)
#define JSON_ASDOUBLE(j,x,def) (j[x].isDouble()?j[x].asDouble():def)
#define JSON_ASFLOAT(j,x,def) (j[x].isFloat()?j[x].asFloat():def)
#define JSON_ASINT(j,x,def) (j[x].isInt()?j[x].asInt():def)
#define JSON_ASINT64(j,x,def) (j[x].isInt64()?j[x].asInt64():def)
#define JSON_ASUINT(j,x,def) (j[x].isUInt()?j[x].asUInt():def)
#define JSON_ASUINT64(j,x,def) (j[x].isUInt64()?j[x].asUInt64():def)

// deprecated JSONReader replacement:
class JSONReader2 {
public:
    JSONReader2();
    ~JSONReader2();

    bool parse(const std::string& document, Json::Value& root);
    std::string getFormattedErrorMessages();

private:
    Json::CharReader * reader;
    std::string errors;
};

std::string jsonToString(const json &value);


#endif // JSON_H
