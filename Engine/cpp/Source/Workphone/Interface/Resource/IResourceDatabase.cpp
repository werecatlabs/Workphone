#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, IResourceDatabase, ISharedObject );

    IResourceDatabase::~IResourceDatabase() = default;

}  // namespace workphone
