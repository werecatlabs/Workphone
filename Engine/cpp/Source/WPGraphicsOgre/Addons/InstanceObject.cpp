#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include "WPGraphicsOgre/Addons/InstanceObject.hpp"
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        CInstanceObjectOld::CInstanceObjectOld() :
            m_position( Ogre::Vector3::ZERO ),
            m_lastDistance( 1e10 ),
            m_nextUpdate( 0 ),
            m_isVisible( true )
        {
            auto engine = core::IApplicationManager::instance();

            // m_stateContext = platformMgr->createStateObject();

            m_stateListener = SmartPtr<IStateListener>( new InstanceObjectStateListener( this ) );
            m_stateContext->addStateListener( m_stateListener );
        }

        CInstanceObjectOld::~CInstanceObjectOld()
        {
        }

        void CInstanceObjectOld::add( Ogre::InstancedEntity *entity )
        {
            m_instanceEntities.push_back( entity );
        }

        bool CInstanceObjectOld::isVisible() const
        {
            return m_isVisible;
        }

        void CInstanceObjectOld::setVisible( bool visible )
        {
            auto currentTaskId = Thread::getCurrentTask();

            if( currentTaskId == TaskId::Render )
            {
                if( m_isVisible != visible )
                {
                    for( u32 i = 0; i < m_instanceEntities.size(); ++i )
                    {
                        Ogre::InstancedEntity *instancedEntity = m_instanceEntities[i];
                        WP_ASSERT( instancedEntity );
                        instancedEntity->setVisible( visible );
                    }

                    m_isVisible = visible;
                }
            }
            else
            {
                if( m_isVisible != visible )
                {
                    auto engine = core::IApplicationManager::instance();
                    // auto& pool = engine->getPool();

                    SmartPtr<StateMessageVisible> visibleMessage( new StateMessageVisible );
                    visibleMessage->setVisible( visible );
                    m_stateContext->addMessage( TaskId::Render, visibleMessage );
                }
            }
        }

        const Ogre::Vector3 &CInstanceObjectOld::getPosition() const
        {
            return m_position;
        }

        void CInstanceObjectOld::setPosition( const Ogre::Vector3 &position )
        {
            m_position = position;
        }

        Ogre::InstancedEntity *CInstanceObjectOld::getBillboard() const
        {
            return m_billboard;
        }

        void CInstanceObjectOld::setBillboard( Ogre::InstancedEntity *billboard )
        {
            m_billboard = billboard;
        }

        workphone::SmartPtr<workphone::IStateContext> CInstanceObjectOld::getStateContext() const
        {
            return m_stateContext;
        }

        void CInstanceObjectOld::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        CInstanceObjectOld::InstanceObjectStateListener::InstanceObjectStateListener(
            CInstanceObjectOld *instanceObject ) :
            m_instanceObject( instanceObject )
        {
        }

        CInstanceObjectOld::InstanceObjectStateListener::~InstanceObjectStateListener()
        {
        }

        bool CInstanceObjectOld::InstanceObjectStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            // if(message->isExactly(StateMessageVisible::TYPE_INFO))
            //{
            //	StateMessageVisiblePtr visibleMessage = message;
            //	m_instanceObject->setVisible(visibleMessage->isVisible());
            // }

            return false;
        }

        bool CInstanceObjectOld::InstanceObjectStateListener::handleStateChanged(
            SmartPtr<IState> &state )
        {
            return false;
        }

    }  // namespace render
}  // namespace workphone
