#ifndef InstanceObject_h__
#define InstanceObject_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

#include <Workphone/WorkphoneTypes.hpp>
#include <OgreVector3.h>

namespace workphone
{
    namespace render
    {

        class CInstanceObjectOld
        {
        public:
            CInstanceObjectOld();
            ~CInstanceObjectOld();

            void add( Ogre::InstancedEntity *entity );

            bool isVisible() const;
            void setVisible( bool visible );

            const Ogre::Vector3 &getPosition() const;
            void setPosition( const Ogre::Vector3 &position );

            Ogre::InstancedEntity *getBillboard() const;
            void setBillboard( Ogre::InstancedEntity *billboard );

            SmartPtr<IStateContext> getStateContext() const;
            void setStateContext( SmartPtr<IStateContext> stateContext );

            class InstanceObjectStateListener : public IStateListener
            {
            public:
                InstanceObjectStateListener( CInstanceObjectOld *instanceObject );
                ~InstanceObjectStateListener();

                bool handleStateMessage( const SmartPtr<IStateMessage> &message );
                bool handleStateChanged( SmartPtr<IState> &state );

                CInstanceObjectOld *m_instanceObject;
            };

            Ogre::Vector3 m_position;
            f32 m_lastDistance;
            u32 m_nextUpdate;
            atomic_bool m_isVisible;
            Array<Ogre::InstancedEntity *> m_instanceEntities;
            Ogre::InstancedEntity *m_billboard;

            SmartPtr<IStateContext> m_stateContext;
            SmartPtr<IStateListener> m_stateListener;
        };

    }  // namespace render
}  // namespace workphone

#endif  // InstanceObject_h__
