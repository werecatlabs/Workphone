#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IMaterialPass.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IMaterialPass, IMaterialNode );

    IMaterialPass::~IMaterialPass() = default;

    const hash_type IMaterialPass::DIFFUSE_HASH = StringUtil::getHash( "diffuse" );
    const hash_type IMaterialPass::EMISSION_HASH = StringUtil::getHash( "emmision" );

    const String IMaterialPass::ambientStr = "ambient";
    const String IMaterialPass::diffuseStr = "diffuse";
    const String IMaterialPass::specularStr = "specular";
    const String IMaterialPass::emissiveStr = "emissive";
    const String IMaterialPass::tintStr = "tint";
    const String IMaterialPass::metalnessStr = "metalness";
    const String IMaterialPass::roughnessStr = "roughness";
    const String IMaterialPass::lightingEnabledStr = "lightingEnabled";
    const String IMaterialPass::transparentStr = "transparent";
    const String IMaterialPass::cutoutStr = "cutout";
    const String IMaterialPass::enableEmissionStr = "enableEmission";
    const String IMaterialPass::enableRefractionStr = "enableRefraction";

}  // namespace workphone::render
