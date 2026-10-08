#ifndef WORKPHONE_PARAMETERIZED_DATABASE_HPP
#define WORKPHONE_PARAMETERIZED_DATABASE_HPP
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>

namespace workphone
{
    /** Optional database extension. Leaves the existing IDatabase ABI unchanged.
     * SQL uses positional ? parameters. A null result denotes failure, including
     * for mutations; a successful mutation returns an empty result set. */
    class IParameterizedDatabase
    {
    public:
        virtual ~IParameterizedDatabase() = default;
        virtual SmartPtr<IDatabaseQuery> queryBound( const String &sql,
                                                     const Array<String> &values ) = 0;
    };
}  // namespace workphone
#endif
