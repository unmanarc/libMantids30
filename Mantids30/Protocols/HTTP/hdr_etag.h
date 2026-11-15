#pragma once

#include <string>
#include <utility>

namespace Mantids30::Network::Protocol::HTTP::Headers {

class ETag
{
public:

    ETag() = default;
    ETag(std::string val, bool isWeak = false) : value(std::move(val)), weak(isWeak) {}

    [[nodiscard]] bool isEmpty() const { return value.empty(); }
    [[nodiscard]] std::string toString() const;
    void fromString(const std::string &str);

    /**
     * @brief Compare this ETag with another (supports weak comparison for validation)
     */
    [[nodiscard]] bool matches(const ETag &other) const;
    [[nodiscard]] std::string getValue() const;
    void setValue(const std::string &newValue);

    [[nodiscard]] bool getWeak() const;
    void setWeak(bool newWeak);

    void clear();


private:
    std::string value;
    bool weak = false;

};

} // namespace Mantids30::Network::Protocol::HTTP::Headers