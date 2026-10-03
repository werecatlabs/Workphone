#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsLight.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsLight, IGraphicsObject );

    const hash_type IGraphicsLight::VISIBILITY_MASK_HASH = StringUtil::getHash( "visibilityMask" );
    const hash_type IGraphicsLight::LIGHT_TYPE_HASH = StringUtil::getHash( "lightType" );
    const hash_type IGraphicsLight::DIFFUSE_COLOUR_HASH = StringUtil::getHash( "diffuseColour" );
    const hash_type IGraphicsLight::SPECULAR_COLOUR_HASH = StringUtil::getHash( "specularColour" );

    const String IGraphicsLight::lightStr = String( "Light" );

    IGraphicsLight::IGraphicsLight() : IGraphicsObject( IGraphicsLight::typeInfo() )
    {
    }

    IGraphicsLight::~IGraphicsLight() = default;

}  // namespace workphone::render
