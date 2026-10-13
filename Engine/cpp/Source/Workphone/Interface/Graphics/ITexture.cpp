#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ITexture, IResource );

    const hash_type ITexture::STATE_MESSAGE_TEXTURE_SIZE = StringUtil::getHash( "textureSize" );

    ITexture::ITexture() : IResource( ITexture::typeInfo() )
    {
    }

    ITexture::ITexture( u32 poolTypeId ) : IResource( poolTypeId )
    {
    }

    ITexture::~ITexture() = default;

    bool ITexture::copyLinearFloatData( const Array<f32> &, const Vector2I &, String &error )
    {
        error = "Linear float texture upload is unsupported by this backend.";
        return false;
    }

    Array<SmartPtr<ITexture>> ITexture::getCubemapFaces() const
    {
        return {};
    }

}  // namespace workphone::render
