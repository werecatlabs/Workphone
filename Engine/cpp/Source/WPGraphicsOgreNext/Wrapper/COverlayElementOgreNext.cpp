#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreOverlaySystem.h>
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, COverlayElementOgreNext, T, T );

    template <class T>
    COverlayElementOgreNext<T>::COverlayElementOgreNext() : OverlayElement<T>()
    {
    }

    template <class T>
    COverlayElementOgreNext<T>::~COverlayElementOgreNext()
    {
    }

    template <class T>
    void COverlayElementOgreNext<T>::_getObject( void **ppObject ) const
    {
        WP_ASSERT( ppObject );
        if( !ppObject )
        {
            return;
        }

        *ppObject = nullptr;

        *ppObject = m_element;
        WP_ASSERT( *ppObject || !T::isLoaded() );
    }

    template <class T>
    bool COverlayElementOgreNext<T>::isValid() const
    {
        if( T::isLoaded() )
        {
            if( !m_element )
            {
                return false;
            }

            auto children = OverlayElement<T>::getChildren();
            for( auto child : children )
            {
                WP_ASSERT( child );
                if( !child || !child->isValid() )
                {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    template <class T>
    Ogre::v1::OverlayElement *COverlayElementOgreNext<T>::getElement() const
    {
        return m_element;
    }

    template <class T>
    void COverlayElementOgreNext<T>::setElement( Ogre::v1::OverlayElement *element )
    {
        m_element = element;
    }

    template <class T>
    void COverlayElementOgreNext<T>::setupMaterial( SmartPtr<IMaterial> material )
    {
    }

    template <class T>
    void COverlayElementOgreNext<T>::createStateContext()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return;
        }

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );
        if( !stateManager )
        {
            return;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );
        if( !factoryManager )
        {
            return;
        }

        auto stateContext = stateManager->addStateContext();
        WP_ASSERT( stateContext );
        if( !stateContext )
        {
            return;
        }

        auto listener = factoryManager->make_ptr<StateListenerOgre>();
        WP_ASSERT( listener );
        listener->setOwner( this );
        stateContext->addStateListener( listener );

        auto state = factoryManager->make_ptr<State>();
        WP_ASSERT( state );
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<OverlayElementState>();
        WP_ASSERT( stateData );
        state->setData( stateData );

        stateContext->setOwner( this );
        OverlayElement<T>::setStateContext( stateContext );

        stateContext->setTaskId( TaskId::Render );
    }

    template <class T>
    bool COverlayElementOgreNext<T>::StateListenerOgre::handleStateMessage(
        const SmartPtr<IStateMessage> &message )
    {
        WP_ASSERT( message );
        return false;
    }

    template <class T>
    bool COverlayElementOgreNext<T>::StateListenerOgre::handleStateChanged( SmartPtr<IState> &state )
    {
        WP_ASSERT( state );
        if( !state )
        {
            return false;
        }

        auto overlayElementState =
            workphone::static_pointer_cast<OverlayElementState>( state->getData() );
        WP_ASSERT( overlayElementState );
        if( !overlayElementState )
        {
            return false;
        }

        auto position = overlayElementState->getPosition();
        auto size = overlayElementState->getSize();

        auto pOwner = OverlayElement<T>::ElementStateListener::getOwner();
        WP_ASSERT( pOwner );
        if( auto owner = workphone::static_pointer_cast<COverlayElementOgreNext<T>>( pOwner ) )
        {
            if( auto element = owner->getElement() )
            {
                auto gva = overlayElementState->getVerticalAlignment();
                element->setVerticalAlignment( static_cast<Ogre::v1::GuiVerticalAlignment>( gva ) );

                auto gha = overlayElementState->getHorizontalAlignment();
                element->setHorizontalAlignment( static_cast<Ogre::v1::GuiHorizontalAlignment>( gha ) );

                auto metricsMode = overlayElementState->getMetricsMode();
                element->setMetricsMode( static_cast<Ogre::v1::GuiMetricsMode>( metricsMode ) );

                element->_setLeft( position.X() );
                element->_setTop( position.Y() );

                element->_setWidth( size.X() );
                element->_setHeight( size.Y() );

                auto visible = overlayElementState->isVisible();
                if( visible != element->isVisible() )
                {
                    if( visible )
                    {
                        auto ownerOverlay = owner->getOverlay();
                        if( !ownerOverlay )
                        {
                            if( auto parent = owner->getParent() )
                            {
                                ownerOverlay = parent->getOverlay();
                                WP_ASSERT( ownerOverlay );
                            }
                        }

                        Ogre::v1::Overlay *ogreOverlay = nullptr;
                        if( auto overlay =
                                workphone::static_pointer_cast<COverlayOgreNext>( ownerOverlay ) )
                        {
                            overlay->_getObject( reinterpret_cast<void **>( &ogreOverlay ) );
                            WP_ASSERT( ogreOverlay );
                        }

                        if( !element->getParent() )
                        {
                            if( auto parent = owner->getParent() )
                            {
                                WP_ASSERT( parent->isContainer() );
                                if( parent->isContainer() )
                                {
                                    Ogre::v1::OverlayContainer *ogreParent = nullptr;
                                    parent->_getObject( reinterpret_cast<void **>( &ogreParent ) );
                                    WP_ASSERT( ogreParent );

                                    if( ogreParent )
                                    {
                                        ogreParent->addChild( element );
                                    }
                                }
                            }
                        }

                        element->_notifyParent( element->getParent(), ogreOverlay );
                        element->show();
                    }
                    else
                    {
                        element->hide();
                    }
                }

                auto text = overlayElementState->getCaption();
                element->setCaption( text.c_str() );

                auto colour = overlayElementState->getColour();
                element->setColour( Ogre::ColourValue( colour.r, colour.g, colour.b, colour.a ) );

                auto material = overlayElementState->getMaterial();
                owner->setupMaterial( material );

                auto zOrder = overlayElementState->getZOrder();
                auto ratio = static_cast<f32>( zOrder ) / 6.0f;
                ratio = MathF::max( ratio, 1.0f );

                constexpr int SubRqIdBits = 3;
                constexpr auto max_value = ( 1u << SubRqIdBits ) - 1u;
                //element->_notifyZOrder( zOrder );

                auto order = static_cast<u32>( ratio * static_cast<f32>( max_value ) );
                //element->setRenderQueueSubGroup( zOrder );

                element->_positionsOutOfDate();
                element->_notifyViewport();

                //element->updatePositionGeometry();
                //element->updateTextureGeometry();
            }
            else
            {
                WP_ASSERT( false );
            }
        }

        return false;
    }

    template <class T>
    auto COverlayElementOgreNext<T>::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = OverlayElement<T>::getProperties();
        if( auto element = getElement() )
        {
            auto text = element->getCaption();
            if( !text.empty() )
            {
                properties->setProperty( "Text", text.c_str() );
            }

            auto colour = element->getColour();
            properties->setProperty( "Colour", ColourF( colour.r, colour.g, colour.b, colour.a ) );

            auto visible = element->isVisible();
            properties->setProperty( "visible", visible );
        }

        return properties;
    }

    template <class T>
    void COverlayElementOgreNext<T>::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            return;
        }

        OverlayElement<T>::setProperties( properties );

        if( auto element = getElement() )
        {
            auto text = properties->getProperty( "Text" );
            element->setCaption( text.c_str() );

            ColourF colour;
            properties->getPropertyValue( "Colour", colour );
            auto colourValue = Ogre::ColourValue( colour.r, colour.g, colour.b, colour.a );
            element->setColour( colourValue );

            //auto materialName = properties->getProperty( "Material" );
            //if( material )
            //{
            //    setupMaterial( materialName );
            //}

            //auto zOrder = properties->getProperty( "ZOrder" );
            //if( zOrder )
            //{
            //    auto value = zOrder->getValue();
            //    auto ratio = (f32)value / 6.0f;
            //    ratio = MathF::max( ratio, 1.0f );

            //    constexpr int SubRqIdBits = 3;
            //    constexpr auto max_value = ( 1u << SubRqIdBits ) - 1u;
            //    //element->_notifyZOrder( zOrder );

            //    auto order = (u32)( ratio * (f32)max_value );
            //    element->setRenderQueueSubGroup( order );
            //}

            auto visible = element->isVisible();

            properties->getPropertyValue( "visible", visible );

            if( element->isVisible() != visible )
            {
                if( visible )
                {
                    element->show();
                }
                else
                {
                    element->hide();
                }
            }
        }
    }

    template class COverlayElementOgreNext<IOverlayElementText>;
    template class COverlayElementOgreNext<IOverlayElementContainer>;

}  // namespace workphone::render
