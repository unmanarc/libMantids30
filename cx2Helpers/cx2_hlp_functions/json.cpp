#include "json.h"


std::string CX2::Helpers::jsonToString(const json &value)
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

CX2::Helpers::JSONReader2::JSONReader2()
{
    Json::CharReaderBuilder builder;
    reader = builder.newCharReader();
}

CX2::Helpers::JSONReader2::~JSONReader2()
{
    delete reader;
}

bool CX2::Helpers::JSONReader2::parse(const std::string &document, Json::Value &root)
{
    return reader->parse(document.c_str(),document.c_str()+document.size(),&root,&errors);
}

std::string CX2::Helpers::JSONReader2::getFormattedErrorMessages()
{
    return errors;
}
