#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IMaterialTexture.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IMaterialTexture, IMaterialNode );

    const String IMaterialTexture::texturePathStr = String( "texture" );
    const String IMaterialTexture::scaleStr = String( "scale" );
    const String IMaterialTexture::tintStr = String( "tint" );
    const String IMaterialTexture::textureTypeStr = String( "textureType" );

    /** Virtual destructor. */
    IMaterialTexture::~IMaterialTexture() = default;

}  // namespace workphone::render
