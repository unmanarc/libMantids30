#include "json.h"


std::string Mantids::Helpers::jsonToString(const json &value)
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

Mantids::Helpers::JSONReader2::JSONReader2()
{
    Json::CharReaderBuilder builder;
    reader = builder.newCharReader();
}

Mantids::Helpers::JSONReader2::~JSONReader2()
{
    delete reader;
}

bool Mantids::Helpers::JSONReader2::parse(const std::string &document, Json::Value &root)
{
    return reader->parse(document.c_str(),document.c_str()+document.size(),&root,&errors);
}

std::string Mantids::Helpers::JSONReader2::getFormattedErrorMessages()
{
    return errors;
}

std::list<std::string> Mantids::Helpers::jsonToStringList(const json &value)
{
    std::list<std::string> r;
    if (value.isArray())
    {
        for ( size_t x = 0; x< value.size(); x++)
        {
            if (value[(int)x].isString())
                r.push_back(value[(int)x].asString());
        }
    }
    return r;
}
