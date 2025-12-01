// bbp_status.hpp
// Shared status / response codes for BBP
#ifndef BBP_STATUS_HPP
#define BBP_STATUS_HPP

#include <array>
#include <string>

namespace bbp
{

    enum class ErrorCode
    {
        MalformedRequest,
        MalformedId,
        MalformedType,
        UnknownType,
        UnknownId,
        NotFound,
        EmptyRequest,
        UnknownRequest
    };

    class ErrorMap
    {
    public:
        static const std::string &toString(ErrorCode code)
        {
            static const std::array<std::string, 8> TABLE = {
                "ERR MALFORMED-REQUEST",
                "ERR MALFORMED-ID",
                "ERR MALFORMED-TYPE",
                "ERR UNKNOWN-TYPE",
                "ERR UNKNOWN-ID",
                "ERR NOT-FOUND",
                "ERR EMPTY-REQUEST",
                "ERR UNKNOWN-REQUEST"};

            static_assert(
                TABLE.size() == static_cast<std::size_t>(ErrorCode::UnknownRequest) + 1,
                "ErrorMap TABLE size must match ErrorCode enum");

            return TABLE[static_cast<std::size_t>(code)];
        }
    };

    enum class OkCode
    {
        Simple,
        Context,
        Outline
    };

    class OkMap
    {
    public:
        static const std::string &toString(OkCode code)
        {
            static const std::array<std::string, 3> TABLE = {
                "OK",
                "OK CONTEXT",
                "OK OUTLINE"};

            static_assert(
                TABLE.size() == static_cast<std::size_t>(OkCode::Outline) + 1,
                "OkMap TABLE size must match OkCode enum");

            return TABLE[static_cast<std::size_t>(code)];
        }
    };

}

#endif
