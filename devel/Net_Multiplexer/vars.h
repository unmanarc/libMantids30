#pragma once

#include <Mantids30/Helpers/json.h>
#include <stdint.h>

namespace Mantids {
namespace Network {
namespace Multiplexor {

// JSON value type used by the multiplexer (boost::json based).
using json = boost::json::value;

typedef uint32_t LineID;

#define SOCKET_MULTIPLEXER_VERSION 3

// Sendbuf should be under 65535.
#define MUX_LINE_SENDBUF 8192          // 8Kb read.
#define MUX_LINE_HEAPSIZE (512 * 1024) // 512Kb.

#define PLUGIN_MAX_DATA 512 * 1024;

#define JSON_MAX_DATA 8 * 1024 * 1024

} // namespace Multiplexor
} // namespace Network
} // namespace Mantids
