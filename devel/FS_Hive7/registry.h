#ifndef HIVE7_REGISTRY_H
#define HIVE7_REGISTRY_H

#include "attributes.h"
#include "permissions.h"

namespace Mantids { namespace Files { namespace Hive7 {

enum eH7RegistryType
{
    H7_REG_BOOL,
    H7_REG_INT8,
    H7_REG_INT16,
    H7_REG_INT32,
    H7_REG_INT64,
    H7_REG_UINT8,
    H7_REG_UINT16,
    H7_REG_UINT32,
    H7_REG_UINT64,
    H7_REG_STRING8,
    H7_REG_STRING16,
    H7_REG_BINARY,
    H7_REG_EMPTY,
    H7_REG_NOTFOUND,
};

class Registry : public Attributes, public Permissions
{
public:
    Registry();

    void appendBytes(const char * bytes, const uint64_t size);
    void setBytes(const char * bytes, const uint64_t size);

    void appendString16(const std::string & value);
    void setString16(const std::string & value);

    void appendString8(const std::string & value);
    void setString8(const std::string & value);

    void setUInt8( const uint8_t & value );
    void setUInt16( const uint16_t & value );
    void setUInt32( const uint32_t & value );
    void setUInt64( const uint16_t & value );

    void setInt8( const int8_t & value );
    void setInt16( const int16_t & value );
    void setInt32( const int32_t & value );
    void setInt64( const int16_t & value );

    std::string asString();

    uint8_t asUInt8();
    uint16_t asUInt16();
    uint32_t asUInt32();
    uint64_t asUInt64();

    int8_t asInt8();
    int16_t asInt16();
    int32_t asInt32();
    int64_t asInt64();

    eH7RegistryType getType() const;

private:
    std::string encryption,compression;
    std::string signatureVersion, signatureHash;

    // TODO: linked file tagging...
    // std::list<std::string> tags;
    eH7RegistryType type;
};

}}}

#endif // HIVE7_REGISTRY_H
