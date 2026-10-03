#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IDatabase, ISharedObject );

    IDatabase::~IDatabase() = default;

}  // namespace workphone
