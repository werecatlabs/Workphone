#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CViewportOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCameraOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreViewport.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CViewportOgreNext, Viewport );

    CViewportOgreNext::CViewportOgreNext()
    {
        try
        {
            static const String name = "CViewportOgreNext";
            setName( name );
            setId( StringUtil::getHash( name ) + m_idExt++ );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManagerPtr();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = graphicsSystem->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto stateContext = graphicsSystem->getStateContext();
            WP_ASSERT( stateContext );
            setStateContext( stateContext );

            auto state = factoryManager->make_ptr<State>();
            state->setId( getId() );
            state->setOwner( this );
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<ViewportStateData>();
            state->setData( stateData );

            //auto mask = 0xFFFFFFFF & ( ~Ogre::VisibilityFlags::LAYER_VISIBILITY );
            auto mask = Ogre::VisibilityFlags::RESERVED_VISIBILITY_FLAGS;
            stateData->visibilityMask = mask;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    CViewportOgreNext::~CViewportOgreNext()
    {
        unload( nullptr );
        destroyStateContext();
    }

    void CViewportOgreNext::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Viewport::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void CViewportOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        Viewport::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void CViewportOgreNext::initialise( Ogre::Viewport *viewport )
    {
        m_viewport = viewport;
    }

    void CViewportOgreNext::_getObject( void **ppObject ) const
    {
        *ppObject = m_viewport;
    }

#ifdef _DEBUG
    s32 CViewportOgreNext::addWeakReference()
    {
        return ISharedObject::addWeakReference();
    }

    bool CViewportOgreNext::removeWeakReference()
    {
        return ISharedObject::removeWeakReference();
    }
#endif

}  // namespace workphone::render
