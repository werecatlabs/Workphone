#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Skybox.hpp>
#include <Workphone/State/States/SkyStateData.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, Skybox, SharedGraphicsObject<ISkybox> );

    Skybox::Skybox() = default;

    Skybox::~Skybox() = default;

    void Skybox::setTexture( SmartPtr<ITexture> texture, u32 layerIdx )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<SkyStateData>( getId() ) )
            {
                state->textures[layerIdx] = texture;
            }
        }
    }

    void Skybox::setTexture( const String &fileName, u32 layerIdx )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabasePtr();
        WP_ASSERT( resourceDatabase );

        auto texture = resourceDatabase->loadResourceByType<ITexture>( fileName );
        if( texture )
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateDataById<SkyStateData>( getId() ) )
                {
                    state->textures[layerIdx] = texture;
                }
            }
        }
    }

    Array<SmartPtr<ITexture>> Skybox::getTextures() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<SkyStateData>( getId() ) )
            {
                return { state->textures.begin(), state->textures.end() };
            }
        }

        return {};
    }

    void Skybox::setTextures( const Array<SmartPtr<ITexture>> &textures )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<SkyStateData>( getId() ) )
            {
                // Initialize all entries to null then copy up to the fixed size
                state->textures.fill( nullptr );

                const size_t srcSize = textures.size();
                const size_t dstSize = state->textures.size();
                const size_t count = srcSize < dstSize ? srcSize : dstSize;

                for( size_t i = 0; i < count; ++i )
                {
                    state->textures[i] = textures[i];
                }
            }
        }
    }

    String Skybox::getTextureName( u32 layerIdx ) const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<SkyStateData>( getId() ) )
            {
                auto texture = state->textures[layerIdx];
                return texture->getName();
            }
        }

        return {};
    }

    SmartPtr<ITexture> Skybox::getTexture( u32 layerIdx ) const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<SkyStateData>( getId() ) )
            {
                return state->textures[layerIdx];
            }
        }

        return nullptr;
    }

    SmartPtr<IGraphicsScene> Skybox::getScene() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<SkyStateData>( getId() ) )
            {
                return state->scene;
            }
        }

        return nullptr;
    }

    void Skybox::setScene( SmartPtr<IGraphicsScene> scene )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<SkyStateData>( getId() ) )
            {
                state->scene = scene;
            }
        }
    }

    bool Skybox::isVisible() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<SkyStateData>( getId() ) )
            {
                return state->visible;
            }
        }

        return false;
    }

    void Skybox::setVisible( bool visible )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<SkyStateData>( getId() ) )
            {
                state->visible = visible;
            }
        }
    }

    f32 Skybox::getDistance() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateDataById<SkyStateData>( getId() ) )
            {
                return state->distance;
            }
        }

        return 0.0f;
    }

    void Skybox::setDistance( f32 distance )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->invalidateStateDataById<SkyStateData>( getId() ) )
            {
                state->distance = distance;
            }
        }
    }
}  // namespace workphone::render
