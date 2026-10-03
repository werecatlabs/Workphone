#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsObjectOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        GraphicsObjectOgreStateListener::GraphicsObjectOgreStateListener() = default;

        GraphicsObjectOgreStateListener::~GraphicsObjectOgreStateListener() = default;

        bool GraphicsObjectOgreStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool GraphicsObjectOgreStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            auto stateData = state->getData();
            if( stateData->isDerived<GraphicsObjectData>() )
            {
                auto graphicsObjectData =
                    workphone::static_pointer_cast<GraphicsObjectData>( stateData );
                auto visible =
                    BitUtil::getFlagValue( graphicsObjectData->flags, IGraphicsObject::visibleFlag );

                if( auto owner = getOwner() )
                {
                    if( owner->isLoaded() )
                    {
                        Ogre::MovableObject *movable = nullptr;
                        owner->_getObject( (void **)&movable );

                        if( movable )
                        {
                            movable->setVisibilityFlags( graphicsObjectData->visibilityMask );
                            movable->setVisible( visible );
                        }

                        return true;
                    }
                }
            }

            return false;
        }

        SmartPtr<IGraphicsObject> GraphicsObjectOgreStateListener::getOwner() const
        {
            return m_owner.load();
        }

        void GraphicsObjectOgreStateListener::setOwner( SmartPtr<IGraphicsObject> owner )
        {
            m_owner = owner;
        }

    }  // namespace render
}  // namespace workphone
