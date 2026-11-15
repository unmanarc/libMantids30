#pragma once

/**
 * Utility functions for working with JSON data using Boost.JSON.
 *
 * This is the public entry point for the JSON helpers. It is kept as a thin
 * shim that pulls in the individual, single-purpose headers so existing
 * consumers can keep using this include unchanged:
 *
 *   - json_detail.h            internal detail helpers (safeGet, *_Safe)
 *   - json_accessors.h         object member accessors (AS*, setMember, ...)
 *   - json_accessors_direct.h  direct value accessors (*_D)
 *   - json_accessors_array.h   array index accessors (ARRAY_*)
 *   - json_path.h              jsoncpp-style Path resolver
 *   - json_convert.h           parse/serialization and Reader class
 */

#include "json_detail.h"
#include "json_accessors.h"
#include "json_accessors_direct.h"
#include "json_accessors_array.h"
#include "json_path.h"
#include "json_convert.h"
