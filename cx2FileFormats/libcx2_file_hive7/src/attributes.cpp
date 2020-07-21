#include "attributes.h"

using namespace CX2::Files::Hive7;

Attributes::Attributes()
{

}

std::string Attributes::getName() const
{
    return name;
}

void Attributes::setName(const std::string &value)
{
    name = value;
}

std::string Attributes::getDescription() const
{
    return description;
}

void Attributes::setDescription(const std::string &value)
{
    description = value;
}

timespec Attributes::getAccessTime() const
{
    return accessTime;
}

void Attributes::setAccessTime(const timespec &value)
{
    accessTime = value;
}

timespec Attributes::getModificationTime() const
{
    return modificationTime;
}

void Attributes::setModificationTime(const timespec &value)
{
    modificationTime = value;
}

timespec Attributes::getCreationTime() const
{
    return creationTime;
}

void Attributes::setCreationTime(const timespec &value)
{
    creationTime = value;
}
