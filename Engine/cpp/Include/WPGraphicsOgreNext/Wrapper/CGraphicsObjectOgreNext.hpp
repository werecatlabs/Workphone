#ifndef CGraphicsObjectOgreNext_h__
#define CGraphicsObjectOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/State/States/GraphicsObjectData.hpp>
#include <OgreMovableObject.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

namespace workphone
{
    namespace render
    {

        template <class T>
        class CGraphicsObjectOgreNext : public T
        {
        public:
            CGraphicsObjectOgreNext();
            ~CGraphicsObjectOgreNext() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void makeDirty() override;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_TEMPLATE_DECL( CGraphicsObjectOgreNext, T );

        protected:
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, CGraphicsObjectOgreNext, T, T );

        template <class T>
        CGraphicsObjectOgreNext<T>::CGraphicsObjectOgreNext() = default;

        template <class T>
        CGraphicsObjectOgreNext<T>::~CGraphicsObjectOgreNext() = default;

        template <class T>
        void CGraphicsObjectOgreNext<T>::load( SmartPtr<ISharedObject> data )
        {
        }

        template <class T>
        void CGraphicsObjectOgreNext<T>::unload( SmartPtr<ISharedObject> data )
        {
            Ogre::SceneManager *smgr = nullptr;

            if( auto creator = T::getCreator() )
            {
                creator->_getObject( reinterpret_cast<void **>( &smgr ) );
            }

            if( auto movableObject = this->template getGraphicsObjectByType<Ogre::MovableObject>() )
            {
                movableObject->detachFromParent();

                if( smgr )
                {
                    smgr->destroyMovableObject( movableObject );
                }
            }

            T::unload( data );
        }

        template <class T>
        void CGraphicsObjectOgreNext<T>::makeDirty()
        {
            if( auto stateContext = T::getStateContext() )
            {
                stateContext->setDirty( true );
            }
        }

        template <class T>
        bool CGraphicsObjectOgreNext<T>::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        template <class T>
        bool CGraphicsObjectOgreNext<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            if( !state )
            {
                return false;
            }

            if( this->isLoaded() )
            {
                if( state->getOwnerPtr() == this )
                {
                    if( auto stateData = state->getData() )
                    {
                        if( stateData->isDerived<GraphicsObjectData>() )
                        {
                            auto graphicsObjectData = SafeReadPtr<GraphicsObjectData>( stateData );
                            auto visible = BitUtil::getFlagValue( graphicsObjectData->flags,
                                                                  IGraphicsObject::visibleFlag );

                            auto castShadows = BitUtil::getFlagValue( graphicsObjectData->flags,
                                                                      IGraphicsObject::castShadowsFlag );

                            if( auto movableObject =
                                    this->template getGraphicsObjectByType<Ogre::MovableObject>() )
                            {
                                if( movableObject->isAttached() )
                                {
                                    movableObject->setVisible( visible );
                                    movableObject->setRenderQueueGroup(
                                        graphicsObjectData->renderQueueGroup );
                                    movableObject->setVisibilityFlags(
                                        graphicsObjectData->visibilityMask );
                                    movableObject->setCastShadows( castShadows );

                                    return true;
                                }
                            }
                        }
                    }
                }
            }

            return false;
        }

    }  // end namespace render
}  // namespace workphone

#endif  // CGraphicsObject_h__
