#include "test_support.h"
#include <Mantids30/Core/Memory/a_allvars.h>
#include <Mantids30/Core/Memory/vars.h>
#include <cmath>
#include <map>

using namespace Mantids30::Core::Memory::Abstract;
using namespace MemoryTests;

namespace {
// Apply the same bounds, parsing, copy, null-state, and JSON checks to every integer width.
template <class Variable, class Value>
void integerContract(Var::Type type, const std::string &below, const std::string &above)
{
    Variable variable;
    CHECK(variable.getVarType() == type);
    CHECK(variable.getValue() == 0);
    for (Value value : {std::numeric_limits<Value>::min(), Value(0), Value(1), std::numeric_limits<Value>::max()})
    {
        const std::string text = std::to_string(value);
        CHECK(variable.fromString(text));
        CHECK(variable.getValue() == value);
        CHECK(variable.toString() == text);
        Variable constructed(value);
        CHECK(constructed.getVarType() == type);
        CHECK(constructed.getValue() == value);
        auto copy = variable.copy();
        CHECK(copy && copy->getVarType() == type);
        CHECK(copy->toString() == text);
        CHECK(variable.setValue(0));
        CHECK(copy->toString() == text);
        Variable json;
        CHECK(json.fromJSON(constructed.toJSON()));
        CHECK(json.getValue() == value);
    }
    for (const auto &invalid : std::vector<std::string>{"", " ", " 1", "1 ", "+1", "1x", "1.0", "0xff", "1e2", below, above, std::string("1\0x", 3)})
    {
        CHECK(variable.setValue(42));
        CHECK(!variable.fromString(invalid));
        CHECK(variable.getValue() == 0); // Integer parsing explicitly resets on failure.
    }
    std::string sliced = "!17?";
    CHECK(variable.fromString(std::string_view(sliced.data() + 1, 2)));
    CHECK(variable.getValue() == 17);
    variable.setIsNull(true);
    CHECK(variable.isNull());
    CHECK(variable.toJSON().is_null());
    auto nullCopy = variable.copy();
    CHECK(nullCopy && nullCopy->isNull());
    CHECK(nullCopy->toJSON().is_null());
}

// Minimal concrete implementation exercises the generic Vars conversion/JSON code.
class Values : public Vars
{
public:
    std::map<std::string, std::list<std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject>>> data;
    uint32_t varCount(const std::string &key) override
    {
        auto it = data.find(key);
        return it == data.end() ? 0 : it->second.size();
    }
    std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject> getValue(const std::string &key) override
    {
        auto it = data.find(key);
        return it == data.end() || it->second.empty() ? nullptr : it->second.front();
    }
    std::list<std::shared_ptr<Mantids30::Core::Memory::Streams::StreamableObject>> getValues(const std::string &key) override
    {
        auto it = data.find(key);
        return it == data.end() ? decltype(it->second){} : it->second;
    }
    std::set<std::string> getKeysList() override
    {
        std::set<std::string> keys;
        for (const auto &entry : data)
        {
            keys.insert(entry.first);
        }
        return keys;
    }
    bool isEmpty() override
    {
        return data.empty();
    }
    void clear() override
    {
        data.clear();
    }
    bool addVar(const std::string &key, const std::shared_ptr<Mantids30::Core::Memory::Containers::B_Chunks> &value) override
    {
        if (key.size() > m_maxVarNameSize || value->size() > m_maxVarContentSize || data.size() >= m_maxVarsCount)
        {
            return false;
        }
        data[key].push_back(value);
        return true;
    }
};
}

TEST("variables", "int8_range_parsing_copy_json_and_type")
{
    integerContract<INT8, int8_t>(Var::Type::INT8, "-129", "128");
}
TEST("variables", "int16_range_parsing_copy_json_and_type")
{
    integerContract<INT16, int16_t>(Var::Type::INT16, "-32769", "32768");
}
TEST("variables", "int32_range_parsing_copy_json_and_type")
{
    integerContract<INT32, int32_t>(Var::Type::INT32, "-2147483649", "2147483648");
}
TEST("variables", "int64_range_parsing_copy_json_and_type")
{
    integerContract<INT64, int64_t>(Var::Type::INT64, "-9223372036854775809", "9223372036854775808");
}
TEST("variables", "uint8_range_parsing_copy_json_and_type")
{
    integerContract<UINT8, uint8_t>(Var::Type::UINT8, "-1", "256");
}
TEST("variables", "uint16_range_parsing_copy_json_and_type")
{
    integerContract<UINT16, uint16_t>(Var::Type::UINT16, "-1", "65536");
}
TEST("variables", "uint32_range_parsing_copy_json_and_type")
{
    integerContract<UINT32, uint32_t>(Var::Type::UINT32, "-1", "4294967296");
}
TEST("variables", "uint64_range_parsing_copy_json_and_type")
{
    integerContract<UINT64, uint64_t>(Var::Type::UINT64, "-1", "18446744073709551616");
}

TEST("variables", "uint64_signed_projection")
{
    UINT64 variable(uint64_t(INT64_MAX));
    CHECK(variable.getIValueTruncatedOrZero() == INT64_MAX);
    CHECK(variable.setValue(uint64_t(INT64_MAX) + 1));
    CHECK(variable.getIValueTruncatedOrZero() == 0);
}

TEST("variables", "double_strict_parsing_and_json")
{
    DOUBLE variable;
    for (const std::string text : {"0", "-1.5", "1.25e2", "2.2250738585072014e-308", "1.7976931348623157e308"})
    {
        CHECK(variable.fromString(text));
        CHECK(variable.getValue() == std::stod(text));
        DOUBLE copy;
        CHECK(copy.fromJSON(variable.toJSON()));
        CHECK(copy.getValue() == variable.getValue());
    }
    for (const std::string invalid : {"", " 1", "1 ", "1x", "1e9999"})
    {
        variable.setValue(7.5);
        CHECK(!variable.fromString(invalid));
        CHECK(variable.getValue() == 7.5);
    }
}

TEST("variables", "boolean_supported_literals_and_copy")
{
    BOOL variable;
    for (const std::string text : {"true", "TRUE", "1", "t", "T"})
    {
        CHECK(variable.fromString(text));
        CHECK(variable.getValue());
        CHECK(variable.toString() == "true");
        CHECK(variable.toJSON().as_bool());
    }
    for (const std::string text : {"false", "FALSE", "0", "", "unknown"})
    {
        CHECK(variable.fromString(text));
        CHECK(!variable.getValue());
    }
}

TEST("variables", "string_binary_json_and_independent_copy")
{
    const std::string data("hello\0\xffworld", 12);
    STRING variable(data);
    CHECK(variable.getValue() == data);
    CHECK(variable.toString() == data);
    auto copy = variable.copy();
    CHECK(variable.setValue("changed"));
    CHECK(copy->toString() == data);
    STRING decoded;
    CHECK(decoded.fromJSON(copy->toJSON()));
    CHECK(decoded.getValue() == data);
    CHECK(decoded.setValue(static_cast<const char *>(nullptr)));
    CHECK(decoded.getValue().empty());
}

TEST("variables", "varchar_initial_empty_exact_capacity_and_truncation")
{
    VARCHAR variable(4);
    CHECK(variable.toString().empty());
    CHECK(variable.getVarSize() == 4);
    CHECK(variable.setValue("abcd"));
    CHECK(!variable.getWasTruncated());
    CHECK(variable.toString() == "abcd");
    CHECK(variable.getFillSize() == 4);
    CHECK(!variable.setValue("abcde"));
    CHECK(variable.getWasTruncated());
    CHECK(variable.toString() == "abcd");
    CHECK(variable.fromString("xy"));
    CHECK(!variable.getWasTruncated());
    CHECK(variable.toString() == "xy");
    CHECK(variable.fromString(""));
    CHECK(variable.toString().empty());
    VARCHAR zero(0);
    CHECK(zero.fromString(""));
    CHECK(!zero.fromString("x"));
    CHECK(zero.toString().empty());
}

TEST("variables", "binary_size_base64_json_copy_and_replacement")
{
    BINARY variable;
    CHECK(variable.toString().empty());
    const std::string data("a\0\xffz", 4);
    CHECK(variable.fromString(data));
    CHECK(variable.toString() == data);
    CHECK(variable.toJSON().as_string() == "YQD/eg==");
    auto bytes = variable.getValue();
    const auto size = bytes->dataSize;
    bytes->mutex.unlock();
    CHECK(size == data.size());
    auto copy = variable.copy();
    CHECK(copy->toString() == data);
    BINARY decoded;
    CHECK(decoded.fromJSON(variable.toJSON()));
    CHECK(decoded.toString() == data);
    CHECK(variable.fromString("x"));
    CHECK(variable.toString() == "x");
    CHECK(copy->toString() == data);
    CHECK(variable.fromString(""));
    CHECK(variable.toString().empty());
    BINARY::ByteArray empty(0);
    CHECK(empty.dataSize == 0);
}

TEST("variables", "stringlist_csv_quotes_empty_fields_and_json")
{
    for (const auto &values : std::vector<std::list<std::string>>{{}, {"simple", "two"}, {"a,b", "a\"b", "last"}, {"", "middle", ""}, {""}, {"a\nline", "tail"}})
    {
        STRINGLIST original(values), decoded;
        CHECK(decoded.fromString(original.toString()));
        CHECK(decoded.getValue() == values);
        CHECK(decoded.fromJSON(original.toJSON()));
        CHECK(decoded.getValue() == values);
        CHECK(original.copy()->toString() == original.toString());
    }
    STRINGLIST variable;
    CHECK(!variable.fromJSON(boost::json::object{}));
}

TEST("variables", "network_addresses_valid_invalid_and_roundtrips")
{
    IPV4 ipv4;
    CHECK(ipv4.fromString("192.0.2.1/24"));
    CHECK(ipv4.toString() == "192.0.2.1/24");
    CHECK(ipv4.matchRange("192.0.2.255"));
    CHECK(!ipv4.matchRange("192.0.3.1"));
    for (const std::string invalid : {"999.1.2.3", "a.b.c.d", "192.0.2.1/33"})
    {
        CHECK(!ipv4.fromString(invalid));
    }
    IPV6 ipv6;
    CHECK(ipv6.fromString("2001:db8::1"));
    CHECK(ipv6.toString() == "2001:db8::1");
    CHECK(ipv6.copy()->toString() == ipv6.toString());
    CHECK(!ipv6.fromString("2001:::bad"));
    MACADDR mac;
    CHECK(mac.fromString("ab:cd:ef:01:23:45"));
    CHECK(mac.toString() == "AB:CD:EF:01:23:45");
    bool ok = false;
    auto hash = MACADDR::_fromStringToHASH(mac.toString(), &ok);
    CHECK(ok);
    CHECK(MACADDR::_fromHASHToString(hash) == mac.toString());
    for (const std::string invalid : {"", "gg:00:00:00:00:00", "00:11:22:33:44", "00:11:22:33:44:55junk"})
    {
        CHECK(!mac.fromString(invalid));
    }
}

TEST("variables", "datetime_utc_offsets_leap_dates_and_json")
{
    for (const std::string text : {"1970-01-01T00:00:00Z", "1970-01-01T02:00:00+02:00", "1969-12-31T19:00:00-05:00"})
    {
        DATETIME epoch;
        CHECK(epoch.fromString(text));
        CHECK(epoch.getValue() == 0);
    }
    DATETIME value(time_t(951782400));
    CHECK(value.toString() == "2000-02-29T00:00:00Z");
    DATETIME decoded;
    CHECK(decoded.fromJSON(value.toJSON()));
    CHECK(decoded.getValue() == value.getValue());
    CHECK(value.copy()->toString() == value.toString());
    for (const std::string invalid : {"garbage", "2023-02-29T00:00:00Z", "2020-13-01T00:00:00Z", "2020-01-01T24:00:00Z", "2020-01-01T00:00:00Zjunk"})
    {
        DATETIME invalidDate;
        invalidDate.fromString(invalid);
        CHECK(invalidDate.getValue() == 0);
    }
}

TEST("variables", "pointer_hex_parsing_and_copy_without_dereferencing")
{
    PTR value;
    CHECK(value.fromString("0x1234"));
    CHECK(value.getValue() == reinterpret_cast<void *>(uintptr_t(0x1234)));
    CHECK(value.copy()->toString() == value.toString());
    CHECK(!value.fromString("0xnothex"));
    CHECK(value.getValue() == nullptr);
    CHECK(value.fromString(""));
}

TEST("variables", "factory_type_and_copy_for_supported_types")
{
    for (auto type : {Var::Type::BOOL, Var::Type::INT8, Var::Type::INT16, Var::Type::INT32, Var::Type::INT64,
                      Var::Type::UINT8, Var::Type::UINT16, Var::Type::UINT32, Var::Type::UINT64, Var::Type::DOUBLE,
                      Var::Type::BIN, Var::Type::STRING, Var::Type::VARCHAR, Var::Type::STRINGLIST,
                      Var::Type::IPV4, Var::Type::IPV6, Var::Type::MACADDR, Var::Type::DATETIME, Var::Type::VOID})
    {
        auto value = Var::makeAbstract(type);
        CHECK(value);
        CHECK(value->getVarType() == type);
        auto copy = value->copy();
        CHECK(copy && copy->getVarType() == type);
    }
    CHECK(!Var::makeAbstract(static_cast<Var::Type>(255)));
}

TEST("variables", "variable_map_replacement_submaps_removal_and_keys")
{
    VariableMap map;
    CHECK(!map.getVariable("missing"));
    CHECK(map.getVariableAsString("missing").empty());
    map.setVariableFromString("count", Var::Type::INT32, "42");
    CHECK(map.getVariableAsString("count") == "42");
    CHECK(map.listVariableKeys() == std::list<std::string>{"count"});
    auto submap = std::make_shared<VariableMap>();
    map.insertOrUpdateSubmap("count", submap);
    CHECK(!map.getVariable("count"));
    CHECK(map.getSubmap("count") == submap);
    CHECK(map.listSubmapKeys() == std::list<std::string>{"count"});
    map.insertOrUpdateVariable("count", std::make_shared<STRING>("changed"));
    CHECK(!map.getSubmap("count"));
    CHECK(map.getVariableAsString("count") == "changed");
    map.removeVariable("count");
    map.removeVariable("missing");
    CHECK(map.listVariableKeys().empty());
}

TEST("variables", "vars_json_multiple_values_and_strict_numeric_conversion")
{
    Values values;
    const boost::json::value json = boost::json::object{
        {"single", "42"}, {"multiple", boost::json::array{"1", "2", "3"}}, {"empty", ""}};
    CHECK(values.fromJSON(json));
    CHECK(values.toJSON() == json);
    CHECK(values.exist("single"));
    CHECK(!values.exist("missing"));
    CHECK(values.getTValue<int>("single") == 42);
    CHECK(values.getTValue<int>("missing") == 0);
    CHECK(values.getTValue<int>("missing", 17) == 17);
    CHECK(values.getTValue<int>("empty", 17) == 17);
    CHECK(values.getTValues<int>("multiple") == std::list<int>({1, 2, 3}));
    CHECK(values.fromJSON(boost::json::object{{"mixed", boost::json::array{"1", "bad", "2x", "", "3"}}}));
    CHECK(values.getTValues<int>("mixed") == std::list<int>({1, 3}));
    values.setMaxVarNameSize(1);
    CHECK(values.getMaxVarNameSize() == 1);
    CHECK(!values.fromJSON(json));
    CHECK(!values.fromJSON(boost::json::array{}));
}

TEST("variables", "binary_set_value_deep_copy_replacement_and_empty_array")
{
    BINARY variable;
    char input[] = {'a', '\0', 'b'};
    BINARY::ByteArray bytes(input, sizeof(input));
    CHECK(variable.setValue(&bytes));
    input[0] = 'x';
    bytes.ptr[0] = 'z';
    CHECK(variable.toString() == std::string("a\0b", 3));
    BINARY::ByteArray replacement(2);
    replacement.ptr[0] = 'O';
    replacement.ptr[1] = 'K';
    CHECK(variable.setValue(&replacement));
    CHECK(variable.toString() == "OK");
    BINARY::ByteArray empty(0);
    CHECK(variable.setValue(&empty));
    CHECK(variable.toString().empty());
    CHECK(variable.toJSON().as_string().empty());
}

TEST("variables", "stringlist_malformed_csv_rejected_and_json_binary_preserved")
{
    STRINGLIST variable(std::list<std::string>{"preserve"});
    CHECK(!variable.fromString("\"unterminated"));
    CHECK(variable.getValue() == std::list<std::string>{"preserve"});
    CHECK(!variable.fromString("\"closed\"junk"));
    const std::list<std::string> data = {std::string("a\0b", 3), "\"", ","};
    CHECK(variable.setValue(data));
    STRINGLIST copy;
    CHECK(copy.fromJSON(variable.toJSON()));
    CHECK(copy.getValue() == data);
}
