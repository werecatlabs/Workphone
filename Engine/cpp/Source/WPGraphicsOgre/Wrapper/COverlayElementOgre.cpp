#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayElementOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreOverlayElement.h>
#include <OgreOverlaySystem.h>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, COverlayElementOgre, T, T );

        template <class T>
        void COverlayElementOgre<T>::createStateContext()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateContext = stateManager->addStateContext();

            auto listener = factoryManager->make_ptr<StateListenerOgre>();
            listener->setOwner( this );
            stateContext->addStateListener( listener );

            auto state = factoryManager->make_ptr<State>();
            stateContext->addState( state );

            auto stateData = factoryManager->make_ptr<OverlayElementState>();
            state->setData( stateData );

            stateContext->setOwner( this );
            OverlayElement<T>::setStateContext( stateContext );

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );
        }

        template <class T>
        bool COverlayElementOgre<T>::StateListenerOgre::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        template <class T>
        bool COverlayElementOgre<T>::StateListenerOgre::handleStateChanged( SmartPtr<IState> &state )
        {
            auto stateData = state->getData();
            if( stateData )
            {
                if( stateData->isDerived<OverlayElementState>() )
                {
                    auto overlayElementState =
                        workphone::static_pointer_cast<OverlayElementState>( stateData );

                    auto position = overlayElementState->getPosition();
                    auto size = overlayElementState->getSize();

                    auto pOwner = OverlayElement<T>::ElementStateListener::getOwner();
                    if( auto owner = workphone::static_pointer_cast<COverlayElementOgre<T>>( pOwner ) )
                    {
                        if( auto element = owner->getElement() )
                        {
                            auto gva = overlayElementState->getVerticalAlignment();
                            element->setVerticalAlignment(
                                static_cast<Ogre::GuiVerticalAlignment>( gva ) );

                            auto gha = overlayElementState->getHorizontalAlignment();
                            element->setHorizontalAlignment(
                                static_cast<Ogre::GuiHorizontalAlignment>( gha ) );

                            auto metricsMode = overlayElementState->getMetricsMode();
                            element->setMetricsMode( static_cast<Ogre::GuiMetricsMode>( metricsMode ) );

                            element->_setLeft( position.X() );
                            element->_setTop( position.Y() );

                            element->_setWidth( size.X() );
                            element->_setHeight( size.Y() );

                            if( overlayElementState->isVisible() )
                            {
                                element->show();
                            }
                            else
                            {
                                element->hide();
                            }

                            auto text = overlayElementState->getCaption();
                            element->setCaption( text.c_str() );

                            auto colour = overlayElementState->getColour();
                            element->setColour(
                                Ogre::ColourValue( colour.r, colour.g, colour.b, colour.a ) );

                            auto material = overlayElementState->getMaterial();
                            owner->setupMaterial( material );

                            auto zOrder = overlayElementState->getZOrder();
                            element->_notifyZOrder( zOrder );

                            state->setDirty( false );
                        }
                    }
                }
            }

            return false;
        }

        template class COverlayElementOgre<IOverlayElementText>;
        template class COverlayElementOgre<IOverlayElementContainer>;
    }  // end namespace render
}  // namespace workphone
