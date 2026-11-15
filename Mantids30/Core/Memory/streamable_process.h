#pragma once

#ifndef _WIN32

#include "streamable_object.h"
#include <Mantids30/Core/Helpers/appexec.h>

namespace Mantids30::Core::Memory::Streams {

class StreamableProcess : public Mantids30::Core::Memory::Streams::StreamableObject
{
public:
    StreamableProcess(Mantids30::Core::Helpers::AppSpawn *spawner);
    ~StreamableProcess() override;
    /**
     * Retrieve Stream to another Streamable.
     * @param objDst pointer to the destination object.
     * @return false if failed, true otherwise.
     */
    bool streamTo(Mantids30::Core::Memory::Streams::StreamableObject *out) override;
    std::optional<size_t> write(const void *buf, const size_t &count) override;

private:
    bool m_streamStdOut, m_streamStdErr;
    Mantids30::Core::Helpers::AppSpawn *m_spawner;
};

} // namespace Mantids30::Core::Memory::Streams
#endif
