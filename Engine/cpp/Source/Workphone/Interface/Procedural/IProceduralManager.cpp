#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{

    WP_CLASS_REGISTER_DERIVED( workphone, IProceduralManager, ISharedObject );

    IProceduralManager::~IProceduralManager() = default;

}  // namespace workphone::procedural
