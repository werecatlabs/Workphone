#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/ShadowMaskMaterial.hpp>

namespace workphone
{
    namespace render
    {

        Ogre::HlmsPbsDatablock *ShadowMaskMaterial::create(
            Ogre::HlmsManager *hlmsManager, const Ogre::String &materialName,
            const Ogre::String &albedoTex, const Ogre::String &shadowMaskTex )
        {
            auto *pbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );

            auto *datablock = static_cast<Ogre::HlmsPbsDatablock *>(
                pbs->createDatablock( materialName, materialName, Ogre::HlmsMacroblock(),
                                      Ogre::HlmsBlendblock(), Ogre::HlmsParamVec() ) );

            datablock->setWorkflow( Ogre::HlmsPbsDatablock::MetallicWorkflow );
            datablock->setDiffuse( Ogre::Vector3( 1.0f, 1.0f, 1.0f ) );
            datablock->setRoughness( 0.7f );
            datablock->setMetalness( 0.0f );

            datablock->setTexture( Ogre::PBSM_DIFFUSE, albedoTex );

            // Practical shortcut:
            // Use emissive texture as a baked light/shadow mask.
            // Ogre-Next can treat emissive as lightmap and multiply it into diffuse.
            datablock->setTexture( Ogre::PBSM_EMISSIVE, shadowMaskTex );
            datablock->setEmissive( Ogre::Vector3( 1.0f, 1.0f, 1.0f ) );
            datablock->setUseEmissiveAsLightmap( true );

            // Usually baked masks/lightmaps use UV1, not UV0.
            datablock->setTextureUvSource( Ogre::PBSM_EMISSIVE, 1 );

            return datablock;
        }

        /*
                auto* shadowMaskMat = ShadowMaskMaterial::create(
                    root->getHlmsManager(),
                    "Materials/Wall_ShadowMasked",
                    "wall_albedo.dds",
                    "wall_shadowmask.dds");

                Ogre::Item* item = sceneManager->createItem("wall.mesh",
                                                            Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                                            Ogre::SCENE_DYNAMIC);

                item->setDatablock(shadowMaskMat);
                sceneNode->attachObject(item);
                */

    }  // namespace render
}  // namespace workphone
