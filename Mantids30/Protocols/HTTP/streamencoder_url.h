#pragma once

#include <Mantids30/Core/Memory/streamable_object.h>
#include <Mantids30/Core/Memory/streamable_transformer.h>

namespace Mantids30::Protocols::HTTP::Encoders {

class URL : public Mantids30::Core::Memory::Streams::StreamableTransformer
{
public:
    URL() = default;
    static std::string encodeURLStr(const std::string &url);

protected:
    size_t writeTo(Mantids30::Core::Memory::Streams::StreamableObject *dst, const void *buf, const size_t &count) override;

private:
    size_t getPlainBytesSize(const unsigned char *buf, size_t count);
    inline bool shouldEncodeThisByte(const unsigned char &byte) const;
};

} // namespace Mantids30::Protocols::HTTP::Encoders
