#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone::procedural, ITextureForge, ISharedObject );

    ITextureForge::~ITextureForge() = default;
}  // namespace workphone::procedural
