#pragma once
#include <string>

namespace Mantids30 {
namespace Database {

/**
* \brief Represents user credentials for database authentication.
* \details Stores essential information such as username and password needed to access the database securely.
*/
struct DatabaseCredentials {
    std::string userName; ///< The username required for database authentication.
    std::string userPassword; ///< The password associated with the username for database access.
};

}  // namespace Database
}  // namespace Mantids
