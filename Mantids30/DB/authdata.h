#pragma once

#include <string>

namespace Mantids30 {
namespace Database {

/**
 * \brief The AuthData struct represents authentication data, including a username and password.
 */
struct AuthData {
    std::string username; // The stored username.
    std::string password; // The stored password.
};

}  // namespace Database
}  // namespace Mantids

