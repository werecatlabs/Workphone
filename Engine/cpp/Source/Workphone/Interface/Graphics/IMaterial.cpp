#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterialNode.hpp>
#include <Workphone/Interface/Graphics/IMaterialPass.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IMaterial, IResource );

    const hash_type IMaterial::SET_TEXTURE_HASH = StringUtil::getHash( "setTexture" );
    const hash_type IMaterial::FRAGMENT_FLOAT_HASH = StringUtil::getHash( "fragmentFloat" );
    const hash_type IMaterial::FRAGMENT_VECTOR2F_HASH = StringUtil::getHash( "fragmentVector2f" );
    const hash_type IMaterial::FRAGMENT_VECTOR3F_HASH = StringUtil::getHash( "fragmentVector3f" );
    const hash_type IMaterial::FRAGMENT_VECTOR4F_HASH = StringUtil::getHash( "fragmentVector4f" );
    const hash_type IMaterial::FRAGMENT_COLOUR_HASH = StringUtil::getHash( "fragmentColour" );
    const hash_type IMaterial::LIGHTING_ENABLED_HASH = StringUtil::getHash( "lightingEnabled" );

    const u32 IMaterial::transparentFlag = ( 1 << 1 );
    const u32 IMaterial::cutoutFlag = ( 1 << 2 );
    const u32 IMaterial::lightingEnabledFlag = ( 1 << 3 );
    const u32 IMaterial::depthWriteEnabledFlag = ( 1 << 4 );
    const u32 IMaterial::depthCheckEnabledFlag = ( 1 << 5 );
    const u32 IMaterial::enableEmissionFlag = ( 1 << 6 );
    const u32 IMaterial::enableRefractionFlag = ( 1 << 7 );

    const String IMaterial::materialTypeStr = String( "Material Type" );

    IMaterial::IMaterial() : IResource( IMaterial::typeInfo() )
    {
    }

    IMaterial::~IMaterial() = default;

}  // namespace workphone::render
