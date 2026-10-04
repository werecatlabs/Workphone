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

    Array<SmartPtr<ITexture>> ITexture::getCubemapFaces() const
    {
        return {};
    }

}  // namespace workphone::render
