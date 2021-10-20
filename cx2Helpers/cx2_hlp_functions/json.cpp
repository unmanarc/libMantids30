#include "json.h"

std::string jsonToString(const json &value)
{
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::string xstrValue = Json::writeString(builder, value);

    if (!xstrValue.empty() && xstrValue[xstrValue.length()-1] == '\n')
    {
        xstrValue.erase(xstrValue.length()-1);
    }
    return xstrValue;
}

JSONReader2::JSONReader2()
{
    Json::CharReaderBuilder builder;
    reader = builder.newCharReader();
}

JSONReader2::~JSONReader2()
{
    delete reader;
}

bool JSONReader2::parse(const std::string &document, Json::Value &root)
{
    return reader->parse(document.c_str(),document.c_str()+document.size(),&root,&errors);
}

std::string JSONReader2::getFormattedErrorMessages()
{
    return errors;
}
