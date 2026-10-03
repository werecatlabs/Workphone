#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/SkyboxPlane.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/State/States/SkyStateData.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, SkyboxPlane, Sky<ISkyboxPlane> );

    SkyboxPlane::SkyboxPlane() = default;

    SkyboxPlane::~SkyboxPlane() = default;

    bool SkyboxPlane::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        // Handle custom state messages specific to planar skybox
        if( !message )
            return false;

        // Currently using default handling through state system
        // Can be extended to support custom messages for:
        // - Layer visibility toggling
        // - Parallax scroll factor updates
        // - Animation frame advancement
        // - Transition triggers (fade, dissolve, etc.)
        // - Dynamic weather or time-of-day effects

        return Sky<ISkyboxPlane>::handleStateMessage( message );
    }

    bool SkyboxPlane::handleStateChanged( SmartPtr<IState> &state )
    {
        // Validate state ownership and loading status
        if( !state || state->getOwnerPtr() != this )
            return false;

        if( !isLoaded() )
            return false;

        // Extract sky state data
        auto stateData = state->getData();
        if( !stateData || !stateData->isDerived<SkyStateData>() )
            return false;

        auto skyStateData = workphone::static_pointer_cast<SkyStateData>( stateData );

        // Acquire required system managers
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
            return false;

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( !graphicsSystem )
            return false;

        auto scene = skyStateData->scene;
        if( !scene )
            return false;

        // Extract state parameters
        const f32 distance = skyStateData->distance;
        const bool visible = skyStateData->visible;
        auto material = skyStateData->material;

        // Planar skybox rendering strategy:
        // Unlike cube skyboxes which surround the camera on all sides,
        // planar skyboxes are typically used for:
        // 1. 2D games: Background planes perpendicular to view direction
        // 2. Skydomes: Hemisphere or dome-shaped backgrounds
        // 3. Parallax layers: Multiple planes at different depths
        // 4. Visual novels: Static background images
        //
        // The renderer implementation interprets the state appropriately
        // based on its capabilities and the game's requirements.

        if( visible )
        {
            // Priority 1: Use provided material if available
            // Materials can include complex shaders for:
            // - Day/night cycles
            // - Weather effects (clouds, fog)
            // - Animated water/sky
            // - Multi-pass rendering
            if( material )
            {
                scene->setSkyBox( visible, material, distance, true );
                return true;
            }

            // Priority 2: Check for texture layers
            // Multiple textures enable:
            // - Parallax scrolling backgrounds (2D platformers)
            // - Layered atmospheric effects
            // - Cube map faces (if renderer supports conversion)
            // - Sequential animation frames
            bool hasTextures = false;
            for( const auto &texture : skyStateData->textures )
            {
                if( texture )
                {
                    hasTextures = true;
                    break;
                }
            }

            if( hasTextures )
            {
                // For planar skybox, use the first texture as primary
                // Renderer may use additional textures for:
                // - Normal maps (depth perception)
                // - Specular maps (water reflections)
                // - Emission maps (self-illuminated areas)
                // - Animation sequences
                auto primaryTexture = skyStateData->textures[0];
                if( primaryTexture )
                {
                    scene->setSkyBox( visible, primaryTexture, distance, true );
                    return true;
                }
            }

            // Priority 3: No material or textures - disable skybox
            //scene->setSkyBox( false, nullptr, distance, true );
            return true;
        }
        else
        {
            // Skybox is hidden - disable rendering
            scene->setSkyBox( false, material, distance, true );
            return true;
        }
    }

}  // namespace workphone::render
