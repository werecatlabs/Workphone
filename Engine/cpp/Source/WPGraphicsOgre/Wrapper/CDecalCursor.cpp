#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CDecalCursor.hpp>
#include <WPGraphicsOgre/Addons/DecalCursorOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CDecalCursor, IDecalCursor );

        CDecalCursor::CDecalCursor() : m_decalCursor( nullptr )
        {
            auto engine = core::IApplicationManager::instance();
        }

        CDecalCursor::~CDecalCursor()
        {
        }

        void CDecalCursor::initialise( SmartPtr<IGraphicsScene> sceneMgr, const String &terrainMaterial,
                                       const String &decalTextureName, const Vector2F &size )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::SceneManager *smgr = nullptr;
            sceneMgr->_getObject( (void **)&smgr );

            m_sceneMgr = sceneMgr.get();

            Ogre::MaterialManager *materialMgr = Ogre::MaterialManager::getSingletonPtr();
            Ogre::MaterialPtr terrainMat = materialMgr->getByName( terrainMaterial.c_str() );

            auto s = Ogre::Vector2( size.X(), size.Y() );
            m_decalCursor = new DecalCursorOgre( smgr, terrainMat, s, decalTextureName.c_str() );
        }

        bool CDecalCursor::isVisible() const
        {
            return m_isVisible;
        }

        void CDecalCursor::setVisible( bool visible )
        {
            m_isVisible = visible;
        }

        Vector3F CDecalCursor::getPosition() const
        {
            return m_position;
        }

        void CDecalCursor::setPosition( const Vector3F &position )
        {
            auto currentTaskId = Thread::getCurrentTask();

            //if( currentTaskId != TaskId::Render )
            //{
            //    SmartPtr<StateMessageVector3> message( new StateMessageVector3( position ) );
            //    m_stateContext->addMessage( TaskId::Render, message );
           // }
            //else
            {
                m_decalCursor->setPosition( Ogre::Vector3( position.X(), position.Y(), position.Z() ) );
            }

            if( m_node )
            {
                m_node->setPosition( position );
            }
        }

        Vector2F CDecalCursor::getSize() const
        {
            return m_size;
        }

        void CDecalCursor::setSize( const Vector2F &size )
        {
            m_size = size;
        }

        void CDecalCursor::addDebugEntity( const String &entityName, const Vector3F &scale )
        {
            m_entity = m_sceneMgr->addGraphicsObjectByType<render::IGraphicsMesh>();
            m_entity->setName( entityName );

            m_node = m_sceneMgr->getRootSceneNode()->addChildSceneNode();
            m_node->attachObject( m_entity );
            m_node->setScale( scale );
            // m_node->getStateContext()->add();
        }

        void CDecalCursor::removeDebugEntity()
        {
        }

        String CDecalCursor::getTextureName() const
        {
            return m_textureName;
        }

        void CDecalCursor::setTextureName( const String &textureName )
        {
            m_textureName = textureName;
        }

        bool CDecalCursor::CDecalCursorStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            // if(message->isExactly(StateMessageVector3::TYPE_INFO))
            //{
            //	SmartPtr<StateMessageVector3> positionMessage = message;

            //	Vector3F position = positionMessage->getValue();
            //	m_owner->setPosition(position);
            //}

            return false;
        }

        bool CDecalCursor::CDecalCursorStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        CDecalCursor::CDecalCursorStateListener::~CDecalCursorStateListener()
        {
        }

        CDecalCursor::CDecalCursorStateListener::CDecalCursorStateListener( CDecalCursor *owner ) :
            m_owner( owner )
        {
        }
    }  // end namespace render
}  // namespace workphone
