#pragma once

/**
 * @brief Sets a boolean response passed by pointer, guarding against nullptr.
 *
 * Replaces the repeated pattern:  if (ok) { *ok = value; }
 * @param ptr   Pointer to the boolean out-parameter (may be nullptr).
 * @param value Value to assign through the pointer.
 */
#define SET_OK_BOOLEAN(ptr, value) do { if ((ptr) != nullptr) { *(ptr) = (value); } } while (0)
#define SV_FMT "%.*s"
#define SV_ARG(sv) static_cast<int>(std::min((sv).size(), static_cast<size_t>(INT_MAX))), (sv).data()