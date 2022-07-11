#ifndef CACHECONTROL_H
#define CACHECONTROL_H

#include <stdint.h>
#include <string>

namespace Mantids { namespace Protocols { namespace HTTP { namespace Headers {

class CacheControl
{
public:
    CacheControl();

    void setDefaults();

    std::string toString();
    void fromString(const std::string & str);

    bool getOptionNoStore() const;
    void setOptionNoStore(bool newOptionNoStore);

    bool getOptionNoCache() const;
    void setOptionNoCache(bool newOptionNoCache);

    bool getOptionMustRevalidate() const;
    void setOptionMustRevalidate(bool newOptionMustRevalidate);

    bool getOptionPrivate() const;
    void setOptionPrivate(bool newOptionPrivate);

    bool getOptionPublic() const;
    void setOptionPublic(bool newOptionPublic);

    uint32_t getMaxAge() const;
    void setMaxAge(uint32_t newMaxAge);

    bool getOptionImmutable() const;
    void setOptionImmutable(bool newOptionImmutable);

    bool getOptionNoTransform() const;
    void setOptionNoTransform(bool newOptionNoTransform);

    uint32_t getSMaxAge() const;
    void setSMaxAge(uint32_t newSMaxAge);

private:
    bool optionNoStore, optionNoCache, optionMustRevalidate, optionPrivate, optionPublic, optionImmutable, optionNoTransform,optionProxyRevalidate;
    uint32_t maxAge,sMaxAge;
};

}}}}
#endif // CACHECONTROL_H
