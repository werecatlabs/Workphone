#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IDatabaseQuery, ISharedObject );

    IDatabaseQuery::IDatabaseQuery() : ISharedObject( IDatabaseQuery::typeInfo() )
    {
    }

    IDatabaseQuery::~IDatabaseQuery() = default;
}  // namespace workphone
