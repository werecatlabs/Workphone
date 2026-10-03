#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTechniqueOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialPassOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreHlmsPbsDatablock.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreHlms.h>
#include <OgreHlmsPbs.h>
#include <OgreHlmsManager.h>
#include <OgreTextureGpuManager.h>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CMaterialTextureOgreNext, MaterialTexture );
    WP_CLASS_REGISTER_DERIVED( workphone::render, CMaterialTextureOgreNext::TextureListener,
                               MaterialTexture );

    CMaterialTextureOgreNext::CMaterialTextureOgreNext()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = graphicsSystem->getFactoryManager();

        auto materialManager = graphicsSystem->getMaterialManagerPtr();
        WP_ASSERT( materialManager );

        auto stateContext = materialManager->getStateContext();
        WP_ASSERT( stateContext );

        auto state = factoryManager->make_ptr<State>();
        state->setId( getId() );
        state->setOwner( this );
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<MaterialPassStateData>();
        state->setData( stateData );

        auto listener = factoryManager->make_ptr<TextureListener>();
        listener->setOwner( this );
        m_textureListener = listener;
    }

    CMaterialTextureOgreNext::~CMaterialTextureOgreNext()
    {
        unload( nullptr );
    }

    void CMaterialTextureOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            MaterialTexture::load( data );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CMaterialTextureOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                m_animator = nullptr;
                m_texture = nullptr;

                if( m_textureListener )
                {
                    m_textureListener->unload( nullptr );
                    m_textureListener = nullptr;
                }

                auto applicationManager = core::IApplicationManager::instance();
                if( applicationManager )
                {
                    auto graphicsSystem = applicationManager->getGraphicsSystem();
                    if( graphicsSystem )
                    {
                        auto materialManager = graphicsSystem->getMaterialManagerPtr();
                        if( materialManager )
                        {
                            auto stateContext = materialManager->getStateContext();
                            if( stateContext )
                            {
                                stateContext->removeStatesById( getId() );
                            }
                        }
                    }
                }

                MaterialTexture::unload( data );

                auto objectListeners = getObjectListeners();
                for( auto &listener : objectListeners )
                {
                    if( listener )
                    {
                        listener->unload( nullptr );
                    }
                }

                this->removeObjectListeners();

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CMaterialTextureOgreNext::loadImage( Ogre::TextureGpu *texture, const String &filePath )
    {
        using namespace Ogre;

        auto imagePtr = new Image2();
        imagePtr->load( filePath.c_str(), ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

        texture->scheduleTransitionTo( GpuResidency::Resident, imagePtr, true );
        // texture->scheduleReupload(imagePtr, true);
    }

    void CMaterialTextureOgreNext::setTexture( SmartPtr<ITexture> texture )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        ScopedLock lock( graphicsSystem );

        if( m_texture != texture )
        {
            if( m_texture )
            {
                m_texture->removeObjectListener( m_textureListener );
            }

            m_texture = texture;

            if( m_texture )
            {
                m_texture->addObjectListener( m_textureListener );
            }
        }
    }

    void CMaterialTextureOgreNext::setScale( const Vector3F &scale )
    {
        m_scale = scale;
    }

    auto CMaterialTextureOgreNext::getAnimator() const -> SmartPtr<IAnimator>
    {
        return m_animator;
    }

    void CMaterialTextureOgreNext::setAnimator( SmartPtr<IAnimator> animator )
    {
        m_animator = animator;
    }

    void CMaterialTextureOgreNext::_getObject( void **ppObject )
    {
        *ppObject = nullptr;
    }

    auto CMaterialTextureOgreNext::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        try
        {
            auto objects = MaterialTexture::getChildObjects();
            objects.emplace_back( m_animator );
            objects.emplace_back( m_texture.load() );
            return objects;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    auto CMaterialTextureOgreNext::getTextureType() const -> u32
    {
        return m_textureType;
    }

    void CMaterialTextureOgreNext::setTextureType( u32 textureType )
    {
        m_textureType = textureType;
    }

    auto CMaterialTextureOgreNext::getTextureUnitState() const -> Ogre::TextureUnitState *
    {
        return m_textureUnitState;
    }

    void CMaterialTextureOgreNext::setTextureUnitState( Ogre::TextureUnitState *textureUnitState )
    {
        m_textureUnitState = textureUnitState;
    }

    auto CMaterialTextureOgreNext::TextureListener::handleEvent( EventType eventType,
                                                                 hash_type eventValue,
                                                                 const Array<Parameter> &arguments,
                                                                 SmartPtr<ISharedObject> sender,
                                                                 SmartPtr<ISharedObject> object,
                                                                 SmartPtr<IEvent> event ) -> Parameter
    {
        if( eventValue == IEvent::loadingStateChanged && arguments.size() > 1 )
        {
            auto newState = static_cast<LoadingState>( arguments[1].getS32() );
            if( newState == LoadingState::Loaded )
            {
                if( auto owner = getOwner() )
                {
                    if( auto material = owner->getMaterial() )
                    {
                        material->makeDirty();
                    }

                    if( auto stateContext = owner->getStateContext() )
                    {
                        stateContext->setDirty( true );
                    }
                }
            }
        }

        return {};
    }

    auto CMaterialTextureOgreNext::TextureListener::getOwner() const
        -> SmartPtr<CMaterialTextureOgreNext>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void CMaterialTextureOgreNext::TextureListener::setOwner( SmartPtr<CMaterialTextureOgreNext> owner )
    {
        m_owner = owner;
    }

    CMaterialTextureOgreNext::TextureListener::TextureListener() = default;

    CMaterialTextureOgreNext::TextureListener::~TextureListener() = default;

}  // namespace workphone::render
