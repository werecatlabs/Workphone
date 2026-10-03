#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IFontManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, IFontManager, IResourceManager );

    IFontManager::IFontManager( u32 poolTypeId ) : IResourceManager( poolTypeId )
    {
    }

    IFontManager::IFontManager() : IResourceManager( IFontManager::typeInfo() )
    {
    }

    IFontManager::~IFontManager() = default;

}  // namespace workphone::render
