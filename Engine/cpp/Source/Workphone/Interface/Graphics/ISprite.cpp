#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ISprite.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ISprite, ISharedObject );

    ISprite::~ISprite() = default;

}  // namespace workphone::render
