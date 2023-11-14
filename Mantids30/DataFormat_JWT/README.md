# Mantids30 DataFormat JWT

Mantids30 DataFormat JWT is a C++ library for creating, signing, and verifying JSON Web Tokens (JWT) with support for different signing algorithms such as HMAC and RSA. The library also includes a caching and token revocation mechanism.

## Features
- Supports HMAC with SHA-256, SHA-384, and SHA-512 signing algorithms
- Supports RSA with SHA-256, SHA-384, and SHA-512 signing algorithms
- Cache for verified tokens
- Token revocation mechanism
- Custom claims support

## Dependencies
- Boost C++ Libraries
- JsonCpp

## Usage

Include the JWT header in your project and use the provided namespace Mantids30::DataFormat.

```cpp
#include "jwt.h"

using namespace Mantids30::DataFormat;
```

### Creating and signing an HS-256 token  

To create a token, first create an instance of the JWT class, passing the desired signing algorithm as a parameter. Then, create an instance of the JWT::Token class and set its claims. Finally, call the signFromToken() function.

```cpp
JWT jwt(JWT::Algorithm::HS256);
jwt.setSharedSecret("YourSharedSecret");

// Example of json array for attributes:
Json::Value attributes;
attributes[0] = "read";
attributes[1] = "write";

// My JWT Token:
JWT::Token token;
token.setIssuer("issuer");
token.setSubject("myUserName");
token.setAudience("audience");
token.setExpirationTime(time(nullptr) + 3600); // Expires in 1 hour
token.setNotBefore(time(nullptr));
token.setIssuedAt(time(nullptr));
token.setJwtId("uniqueJwtId");
token.addClaim("custom_claim", "custom_value");
token.addClaim("attributes", attributes);

// Sign:
std::string signedToken = jwt.signFromToken(token);
```

### Verifying an HS-256 token

To verify a token, call the verify() function with the signed token as the parameter. If you want to retrieve the decoded token, pass a pointer to a JWT::Token object as the second parameter.

```cpp
JWT jwt(JWT::Algorithm::HS256);
jwt.setSharedSecret("YourSharedSecret");

std::string signedToken = "yourSignedToken";
JWT::Token decodedToken;
bool isVerified = jwt.verify(signedToken, &decodedToken);

if (isVerified) {
    std::cout << "Token verified." << std::endl;
} else {
    std::cout << "Token not verified." << std::endl;
}
```

### Using the revocation mechanism

Add tokens to the revocation list with the token expiration date. The signature should be in raw format (not base64 encoded)

```cpp
JWT::Token myToken;
...
jwt.m_revocation.addToRevocationList(signature, myToken.getExpirationTime());
```
