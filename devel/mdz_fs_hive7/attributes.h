#ifndef HIVE7_ATTRIBUTES_H
#define HIVE7_ATTRIBUTES_H

#include <string>

namespace Mantids { namespace Files { namespace Hive7 {

class Attributes
{
public:
    Attributes();
    std::string getName() const;
    void setName(const std::string &value);

    std::string getDescription() const;
    void setDescription(const std::string &value);

    timespec getAccessTime() const;
    void setAccessTime(const timespec &value);

    timespec getModificationTime() const;
    void setModificationTime(const timespec &value);

    timespec getCreationTime() const;
    void setCreationTime(const timespec &value);

protected:
    std::string name,description;

    struct timespec accessTime;
    struct timespec modificationTime;
    struct timespec creationTime;
};

}}}

#endif // HIVE7_ATTRIBUTES_H
