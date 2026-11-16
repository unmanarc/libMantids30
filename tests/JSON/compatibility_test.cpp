#include <Mantids30/DataFormat/JSON/json.h>
#include <Mantids30/DataFormat/JWT/jwt.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

static void require(bool condition, const char *message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

int main()
{
    try
    {
        using namespace Mantids30::DataFormat::JSON;
        const std::string storage = "__name__";
        const std::string_view key(storage.data() + 2, 4);
        const std::string binary("a\0b", 3);
        boost::json::value object;
        setMember(object, key, binary);
        require(HAS_MEMBER(object, key), "substring key lookup failed");
        require(ASSTRING(object, key) == std::string_view(binary), "object string length changed");
        require(MEMBER_OR_NULL(object, key).is_string(), "substring key find failed");
        require(MEMBER_OR_NULL(object, "absent").is_null(), "missing member did not return null");
        setMember(object, key, 42);
        require(ASINT(object, key, 0) == 42, "scalar setter failed");
        const boost::json::value array = boost::json::array{boost::json::string(binary.data(), binary.size())};
        setMember(object, key, array);
        require(ASARRAY_OR_EMPTY(object, key).size() == 1, "array member lookup failed");
        require(ARRAY_ASSTRING(array, 0, "fallback") == std::string_view(binary), "array string length changed");
        require(ARRAY_ASSTRING(array, 1, "fallback") == "fallback", "array fallback changed");
        require(toStringList(array).front() == binary, "string list conversion lost bytes");
        const std::string binaryKey("x\0y", 3);
        setMember(object, binaryKey, std::string("value"));
        require(HAS_MEMBER(object, binaryKey), "embedded-null key lookup failed");
        require(toMap(object).at(binaryKey) == "value", "map conversion lost key bytes");
        const std::string document = "__{\"k\":1}__";
        boost::json::value parsed;
        std::string errors;
        require(parse(std::string_view(document.data() + 2, 7), parsed, &errors), "bounded JSON document parse failed");
        require(ASINT(parsed, "k", 0) == 1 && errors.empty(), "parsed document mismatch");
        Mantids30::DataFormat::JWT::JWT::Token token;
        token.setClaim(key, std::string_view(binary));
        require(ASSTRING_D(token.getClaim(key)) == std::string_view(binary), "JWT string-view claim mismatch");
        require(token.getAllClaims().at(std::string(key)).is_string(), "JWT claim map conversion failed");
        token.addRole(key);
        token.addScope(std::string_view(binary));
        require(token.hasRole(key) && token.hasScope(std::string_view(binary)), "JWT role/scope string-view conversion failed");
        std::cout << "JSON string-view compatibility checks passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
