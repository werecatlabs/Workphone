#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IVideoTexture, render::ITexture );

    IVideoTexture::IVideoTexture() : ITexture( IVideoTexture::typeInfo() )
    {
    }

    IVideoTexture::IVideoTexture( u32 poolTypeInfo ) : ITexture( poolTypeInfo )
    {
    }

    IVideoTexture::~IVideoTexture() = default;

}  // namespace workphone::render
