#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IResourceManager, ISharedObject );

    IResourceManager::IResourceManager( u32 poolTypeId ) : ISharedObject( poolTypeId )
    {
    }

    IResourceManager::IResourceManager() : ISharedObject( IResourceManager::typeInfo() )
    {
    }

    IResourceManager::~IResourceManager() = default;

}  // namespace workphone
