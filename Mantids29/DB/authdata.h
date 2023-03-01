#ifndef AUTHINFO_H
#define AUTHINFO_H

#include <string>

namespace Mantids29 {
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

#endif // AUTHINFO_H
