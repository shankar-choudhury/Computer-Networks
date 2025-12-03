// bbp_status.hpp
// Shared status / response codes for BBP
#ifndef BBP_STATUS_HPP
#define BBP_STATUS_HPP

#include <array>
#include <string>

namespace bbp
{
#define BBP_ERROR_CODES(X)                        \
    X(MalformedRequest, "ERR MALFORMED-REQUEST")  \
    X(MalformedId, "ERR MALFORMED-ID")            \
    X(MalformedType, "ERR MALFORMED-TYPE")        \
    X(UnknownType, "ERR UNKNOWN-TYPE")            \
    X(UnknownId, "ERR UNKNOWN-ID")                \
    X(NotFound, "ERR NOT-FOUND")                  \
    X(EmptyRequest, "ERR EMPTY-REQUEST")          \
    X(CommandNotFound, "ERR COMMAND-NOT-FOUND")   \
    X(TypeNotFound, "ERR TYPE-NOT-FOUND")         \
    X(MissingBody, "ERR MISSING-BODY")            \
    X(MissingTitle, "ERR MISSING-TITLE")          \
    X(ItemExists, "ERR ITEM-EXISTS")              \
    X(LinkExists, "ERR LINK-EXISTS")              \
    X(UnknownRequest, "ERR UNKNOWN-REQUEST")      \
    X(InvalidBookName, "ERR INVALID-BOOK-NAME")   \
    X(BookExists, "ERR BOOK-EXISTS")              \
    X(BookNotFound, "ERR BOOK-NOT-FOUND")         \
    X(BookCreateFailed, "ERR BOOK-CREATE-FAILED") \
    X(BookLoadFailed, "ERR BOOK-LOAD-FAILED")     \
    X(BookDeleteFailed, "ERR BOOK-DELETE-FAILED") \
    X(Unauthorized, "ERR UNAUTHORIZED")           \
    X(CannotDeleteActiveBook, "ERR ACTIVE-BOOK")

    enum class ErrorCode
    {
#define BBP_DEFINE_ERROR_ENUM(name, str) name,
        BBP_ERROR_CODES(BBP_DEFINE_ERROR_ENUM)
#undef BBP_DEFINE_ERROR_ENUM
            Count
    };

    class ErrorMap
    {
    public:
        static const std::string &toString(ErrorCode code)
        {
            static const std::array<std::string,
                                    static_cast<std::size_t>(ErrorCode::Count)>
                TABLE = {
#define BBP_DEFINE_ERROR_STR(name, str) str,
                    BBP_ERROR_CODES(BBP_DEFINE_ERROR_STR)
#undef BBP_DEFINE_ERROR_STR
                };

            static_assert(TABLE.size() == static_cast<std::size_t>(ErrorCode::Count),
                          "ErrorMap TABLE size must match ErrorCode enum");

            return TABLE[static_cast<std::size_t>(code)];
        }
    };

#define BBP_OK_CODES(X)                      \
    X(Simple, "OK")                          \
    X(Context, "OK CONTEXT")                 \
    X(Outline, "OK OUTLINE")                 \
    X(NewBookCreated, "OK NEW-BOOK-CREATED") \
    X(BookLoaded, "OK BOOK-LOADED")          \
    X(BookDeleted, "OK BOOK-DELETED")

    enum class OkCode
    {
#define BBP_DEFINE_OK_ENUM(name, str) name,
        BBP_OK_CODES(BBP_DEFINE_OK_ENUM)
#undef BBP_DEFINE_OK_ENUM
            Count
    };

    class OkMap
    {
    public:
        static const std::string &toString(OkCode code)
        {
            static const std::array<std::string,
                                    static_cast<std::size_t>(OkCode::Count)>
                TABLE = {
#define BBP_DEFINE_OK_STR(name, str) str,
                    BBP_OK_CODES(BBP_DEFINE_OK_STR)
#undef BBP_DEFINE_OK_STR
                };

            static_assert(TABLE.size() == static_cast<std::size_t>(OkCode::Count),
                          "OkMap TABLE size must match OkCode enum");

            return TABLE[static_cast<std::size_t>(code)];
        }
    };

    inline std::string errorCodeToString(ErrorCode code)
    {
        return ErrorMap::toString(code);
    }

    inline std::string okCodeToString(OkCode code)
    {
        return OkMap::toString(code);
    }

#undef BBP_ERROR_CODES
#undef BBP_OK_CODES

}

#endif
