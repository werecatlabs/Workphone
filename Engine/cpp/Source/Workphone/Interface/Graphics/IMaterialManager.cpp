#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IMaterialManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, IMaterialManager, IResourceManager );

    IMaterialManager::~IMaterialManager() = default;

}  // namespace workphone::render
