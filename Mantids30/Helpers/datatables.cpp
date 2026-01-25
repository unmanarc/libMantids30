#include "datatables.h"
#include <regex>

static std::string getColumnNameFromColumnPos(const json &dataTablesFilters, const uint32_t & pos)
{
    if (JSON_ISARRAY(dataTablesFilters,"columns"))
    {
        if (pos>=dataTablesFilters["columns"].size())
        {
            return "";
        }
        return JSON_ASSTRING(dataTablesFilters["columns"][pos],"data","");
    }
    return "";
}


std::string Mantids30::Helpers::DataTables::getOrderByStatement(const json &dataTablesFilters)
{
    std::string orderByStatement;
    // Manejo de ordenamiento (order)
    const Json::Value &orderArray = dataTablesFilters["order"];
    if (JSON_ISARRAY_D(orderArray) && orderArray.size() > 0)
    {
        const Json::Value &orderArrayElement = orderArray[0];
        std::string columnName = getColumnNameFromColumnPos(dataTablesFilters, JSON_ASUINT(orderArrayElement, "column", 0));
        std::string dir = JSON_ASSTRING(orderArrayElement, "dir", "desc");

        // Validar que el nombre del campo sea seguro (solo letras, números y guiones bajos)
        static const std::regex fieldRegex("^[a-zA-Z0-9_]+$");
        if (std::regex_match(columnName, fieldRegex))
        {
            orderByStatement = "`" + columnName + "` ";
            orderByStatement += (dir == "desc") ? "DESC" : "ASC";
        }
    }
    return orderByStatement;
}
