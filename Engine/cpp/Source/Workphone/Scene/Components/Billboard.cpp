#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Billboard.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/IGraphicsWindow.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Billboard, Component );

    const String Billboard::TextureStr = String( "Texture" );
    const String Billboard::SizeStr = String( "Size" );
    const String Billboard::ColorStr = String( "Color" );

    Billboard::Billboard()
    {
        m_size = Vector2F( 1.0f, 1.0f );
        m_color = ColourF( 1.0f, 1.0f, 1.0f, 1.0f );
    }

    Billboard::~Billboard() = default;

    void Billboard::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            Component::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto graphicsScene = graphicsSystem->getGraphicsScene();
            WP_ASSERT( graphicsScene );

            auto actor = getActor();
            WP_ASSERT( actor );

            auto transform = actor->getTransform();
            WP_ASSERT( transform );

            auto sceneNode = graphicsScene->addSceneNode();
            WP_ASSERT( sceneNode );

            m_sceneNode = sceneNode;
            sceneNode->setPosition( transform->getPosition() );
            sceneNode->setOrientation( transform->getOrientation() );
            sceneNode->setScale( transform->getScale() );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Billboard::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            if( m_sceneNode )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto graphicsScene = graphicsSystem->getGraphicsScene();
                WP_ASSERT( graphicsScene );

                graphicsScene->removeSceneNode( m_sceneNode );
                m_sceneNode = nullptr;
            }

            Component::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<Properties> Billboard::getProperties() const
    {
        auto properties = Component::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( TextureStr, getTexture() );
        properties->setProperty( SizeStr, getSize() );
        properties->setProperty( ColorStr, getColor() );

        return properties;
    }

    void Billboard::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );
        if( !properties )
        {
            return;
        }

        auto texture = getTexture();
        auto size = getSize();
        auto color = getColor();
        properties->getPropertyValue( TextureStr, texture );
        properties->getPropertyValue( SizeStr, size );
        properties->getPropertyValue( ColorStr, color );

        setTexture( texture );
        setSize( size );
        setColor( color );
    }

    void Billboard::updateBillboard()
    {
        if( m_sceneNode && m_texture )
        {
            // Update the billboard with the current texture, size and color
            // This would typically involve updating the material and geometry
            // of the billboard to reflect the new properties
        }
    }

    SmartPtr<render::ITexture> Billboard::getTexture() const
    {
        return m_texture;
    }

    void Billboard::setTexture( SmartPtr<render::ITexture> texture )
    {
        m_texture = texture;
        updateBillboard();
    }

    Vector2F Billboard::getSize() const
    {
        return m_size;
    }

    void Billboard::setSize( const Vector2F &size )
    {
        m_size = size;
        updateBillboard();
    }

    ColourF Billboard::getColor() const
    {
        return m_color;
    }

    void Billboard::setColor( const ColourF &color )
    {
        m_color = color;
        updateBillboard();
    }

}  // namespace workphone::scene
